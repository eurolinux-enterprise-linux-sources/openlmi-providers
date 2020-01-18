/*
 * Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Roman Rakus <rrakus@redhat.com>
 */

/*
 * Common functions for indications used in Account provider
 */

#include <cmpimacs.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <limits.h>
#include <errno.h>

#include "indication_common.h"
#include <globals.h>

#include "LMI_Account.h"
#include "LMI_Group.h"
#include "LMI_Identity.h"
static const char* allowed_classes[] = {
    LMI_Account_ClassName,
    LMI_Group_ClassName,
    LMI_Identity_ClassName,
    NULL };

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (10 * EVENT_SIZE + NAME_MAX + 1)
#define PASSWD_FILE "/etc/passwd"
#define GROUP_FILE "/etc/group"

bool filter_checker(const CMPISelectExp *filter)
{
    /*
     * Support only simple conditions and only on allowed_classes
     * and type of `sourceinstance ISA allowed_class'
     */
    CMPIStatus st;
    CMPISelectCond *sec = CMGetDoc(filter, &st);
    if (!sec) return false;
    CMPICount count = CMGetSubCondCountAndType(sec, NULL, &st);
    if (count != 1) return false;
    CMPISubCond *sub = CMGetSubCondAt(sec, 0, &st);
    if (!sub) return false;
    count = CMGetPredicateCount(sub, &st);
    if (count != 1) return false;
    CMPIPredicate *pred = CMGetPredicateAt(sub, 0, &st);
    if (!pred) return false;
    CMPIType type;
    CMPIPredOp op;
    CMPIString *lhs = NULL;
    CMPIString *rhs = NULL;
    st = CMGetPredicateData(pred, &type, &op, &lhs, &rhs);
    if (st.rc != CMPI_RC_OK || op != CMPI_PredOp_Isa) return false;
    const char *rhs_str = CMGetCharsPtr(rhs, &st);
    if (!rhs_str) return false;
    unsigned i = 0;
    while (allowed_classes[i]) {
        if (strcasecmp(rhs_str, allowed_classes[i++]) == 0) return true;
    }
    return false;
}

/*
 * Returns last modification time for specified file name
 */
static struct timespec get_last_mod(const char* file)
{
    struct stat buf = {0};
    stat (file, &buf);
    return buf.st_mtim;
}

/*
 * Compares 2 timespecs
 * return value is same like in strcmp
 */
static int timecmp(struct timespec a, struct timespec b)
{
    if (a.tv_sec == b.tv_sec) {
        if (a.tv_nsec == b.tv_nsec) {
            return 0;
        } else {
            return (a.tv_nsec > b.tv_nsec ? 1 : -1);
        }
    } else {
        return (a.tv_sec > b.tv_sec ? 1 : -1);
    }
}

#define ADD_WATCH(fd, wd, file)\
    (wd) = inotify_add_watch((fd), (file), IN_CLOSE_WRITE | IN_MODIFY |\
                                           IN_DELETE | IN_DELETE_SELF);\
    if ((wd) < 0) {\
        goto bail;\
    }\

bool watcher_init(AccountIndication *ind)
{
    ind->wd_pwd = 0;
    ind->wd_grp = 0;
    ind->inotify_fd = inotify_init();

    if (ind->inotify_fd < 0)
        return false;

    /* Get initial timestamps, at the beginning of watching. */
    ind->last_pwd = get_last_mod(PASSWD_FILE);
    ind->last_grp = get_last_mod(GROUP_FILE);

    ADD_WATCH(ind->inotify_fd, ind->wd_pwd, PASSWD_FILE);
    ADD_WATCH(ind->inotify_fd, ind->wd_grp, GROUP_FILE);

    return true;

bail:
    watcher_destroy(ind);
    return false;
}

void watcher_destroy(AccountIndication *ind)
{
    if (ind->inotify_fd >= 0) {
        if (ind->wd_pwd > 0)
            inotify_rm_watch(ind->inotify_fd, ind->wd_pwd);
        if (ind->wd_grp > 0)
            inotify_rm_watch(ind->inotify_fd, ind->wd_grp);
        close(ind->inotify_fd);
        ind->inotify_fd = -1;
    }
}

bool watcher(AccountIndication *ind, void **data)
{
    struct timespec curr_pwd, curr_grp;
    char buffer[BUF_LEN];

    if (ind->inotify_fd < 0)
        return false;

    do {
        int len = 0, i = 0;
        if ((len = read(ind->inotify_fd, buffer, BUF_LEN)) < 0 || len > (int) BUF_LEN) {
            warn("account watcher: error reading from inotify fd: %s", strerror(errno));
            watcher_destroy(ind);
            watcher_init(ind);
            return false;
        }
        while (i + (ssize_t) EVENT_SIZE < len) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (i + (ssize_t) EVENT_SIZE + event->len >= len) {
                error("Unable to create watcher, inotify initialization failed");
                watcher_destroy(ind);
                watcher_init(ind);
                return false;
            }
            switch (event->mask) {
                case IN_MODIFY:
                case IN_CLOSE_WRITE:
                case IN_DELETE:
                case IN_DELETE_SELF:
                    if (event->wd == ind->wd_grp) {
                        curr_grp = get_last_mod(GROUP_FILE);
                        if (timecmp(ind->last_grp, curr_grp) == -1) {
                            ind->last_grp = curr_grp;
                            return true;
                        }
                    } else {
                        curr_pwd = get_last_mod(GROUP_FILE);
                        if (timecmp(ind->last_pwd, curr_pwd) == -1) {
                            ind->last_pwd = curr_pwd;
                            return true;
                        }
                    }
                    break;
                case IN_IGNORED:
                    if (event->wd == ind->wd_grp) {
                        ADD_WATCH(ind->inotify_fd, ind->wd_grp, GROUP_FILE);
                        curr_grp = get_last_mod(GROUP_FILE);
                        if (timecmp(ind->last_grp, curr_grp) == -1) {
                            ind->last_grp = curr_grp;
                            return true;
                        }
                    } else {
                        ADD_WATCH(ind->inotify_fd, ind->wd_pwd, PASSWD_FILE);
                        curr_pwd = get_last_mod(GROUP_FILE);
                        if (timecmp(ind->last_pwd, curr_pwd) == -1) {
                            ind->last_pwd = curr_pwd;
                            return true;
                        }
                    }
                    break;
            }
            i += EVENT_SIZE + event->len;
        }
    } while (1);

bail:
    return false;
}
