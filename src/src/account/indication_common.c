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

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (10 * EVENT_SIZE + NAME_MAX + 1)
#define WATCH_PATH "/etc/"
#define PASSWD_FILE_NAME "passwd"
#define GROUP_FILE_NAME "group"

#define SETTLE_DELAY 1000 * 250  /* usec */

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

bool watcher_init(AccountIndication *ind)
{
    ind->wd = -1;
    ind->inotify_fd = inotify_init();

    if (ind->inotify_fd < 0)
        return false;

    /* Get initial timestamps, at the beginning of watching. */
    ind->last_pwd = get_last_mod(WATCH_PATH PASSWD_FILE_NAME);
    ind->last_grp = get_last_mod(WATCH_PATH GROUP_FILE_NAME);

    ind->wd = inotify_add_watch(ind->inotify_fd, WATCH_PATH,
                                IN_CLOSE_WRITE | IN_CREATE | IN_MODIFY | IN_MOVED_TO);
    if (ind->wd < 0) {
        watcher_destroy(ind);
        return false;
    }
    return true;
}

void watcher_destroy(AccountIndication *ind)
{
    if (ind->inotify_fd >= 0) {
        if (ind->wd >= 0)
            inotify_rm_watch(ind->inotify_fd, ind->wd);
        close(ind->inotify_fd);
        ind->wd = -1;
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
            if (i + (ssize_t) EVENT_SIZE + event->len > len) {
                error("Unable to create watcher, inotify initialization failed");
                watcher_destroy(ind);
                watcher_init(ind);
                return false;
            }
            if (event->len > 1) {
                if (strcmp(&event->name[0], PASSWD_FILE_NAME) == 0) {
                    curr_pwd = get_last_mod(WATCH_PATH PASSWD_FILE_NAME);
                    if (timecmp(ind->last_pwd, curr_pwd) == -1) {
                        ind->last_pwd = curr_pwd;
                        usleep(SETTLE_DELAY);
                        return true;
                    }
                } else
                if (strcmp(&event->name[0], GROUP_FILE_NAME) == 0) {
                    curr_grp = get_last_mod(WATCH_PATH GROUP_FILE_NAME);
                    if (timecmp(ind->last_grp, curr_grp) == -1) {
                        ind->last_grp = curr_grp;
                        usleep(SETTLE_DELAY);
                        return true;
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }
    } while (1);
}
