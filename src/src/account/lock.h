/*
 * Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
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
 * Author: Robin Hack <rhack@redhat.com>
 */
#ifndef _LOCK_H
#define _LOCK_H

#include <pthread.h>
#include <glib.h>

/* Global */
#define USERNAME_LEN_MAX 33

typedef struct lock {
    char id [USERNAME_LEN_MAX];
    pthread_mutex_t mutex;
    unsigned int instances;
} lock_t;

typedef struct giant_lock {
    pthread_mutex_t mutex;
} giant_lock_t;

giant_lock_t giant_lock;

typedef struct lock_pool {
    GHashTable *hash_table;
    pthread_mutex_t csec;
} lock_pool_t;

typedef struct lock_pools {
    lock_pool_t user_pool;
    lock_pool_t group_pool;
    int initialized;
    pthread_mutex_t csec;
} lock_pools_t;

int init_lock_pools (void) __attribute__((warn_unused_result));
void destroy_lock_pools (void);

int get_user_lock (const char *const username) __attribute__((nonnull));
int get_group_lock (const char *const groupname) __attribute__((nonnull));
int release_user_lock (const char *const username) __attribute__((nonnull));
int release_group_lock (const char *const username) __attribute__((nonnull));
void get_giant_lock (void);
void release_giant_lock (void);

#endif /* _LOCK_H */
