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

#include "LMI_Account.h"
#include "LMI_Group.h"
#include "LMI_Identity.h"

typedef struct {
    int wd;
    int inotify_fd;
    struct timespec last_pwd, last_grp;
} AccountIndication;

static const char* account_allowed_classes[] = {
    LMI_Account_ClassName,
    LMI_Group_ClassName,
    LMI_Identity_ClassName,
    NULL};

bool watcher_init(AccountIndication *ind);
bool watcher(AccountIndication *ind, void **data);
void watcher_destroy(AccountIndication *ind);
