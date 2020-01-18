/*
 * Copyright (C) 2012-2013 Red Hat, Inc.  All rights reserved.
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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <error.h>
#include <limits.h>
#include <glib.h>

#include "lock.h"

static lock_pools_t pools;

/* Critical sections lock */
static inline void LOCK_CSEC_POOLS (void)   { pthread_mutex_lock (&pools.csec); }
static inline void UNLOCK_CSEC_POOLS (void) { pthread_mutex_unlock (&pools.csec); }
static inline void LOCK_CSEC_USER (void)    { pthread_mutex_lock (&pools.user_pool.csec); }
static inline void UNLOCK_CSEC_USER (void)  { pthread_mutex_unlock (&pools.user_pool.csec); }
static inline void LOCK_CSEC_GROUP (void)   { pthread_mutex_lock (&pools.group_pool.csec); }
static inline void UNLOCK_CSEC_GROUP (void) { pthread_mutex_unlock (&pools.group_pool.csec); }


static pthread_once_t pools_are_initialized = PTHREAD_ONCE_INIT;
static unsigned int ref_count = 0;

/* Callbacks */
static void new_pools (void);
static void free_lock (gpointer lock) __attribute__((nonnull));

/* Internal functions */
static int search_key (lock_pool_t *const pool, const char *const key, lock_t **lck) __attribute__((nonnull));
static int add_key (lock_pool_t *const pool, const char *const key) __attribute__((nonnull));
static int release_lock (lock_pool_t *const pool, const char *const key) __attribute__((nonnull));
static int get_lock (lock_pool_t *const pool, const char *const key) __attribute__((nonnull));


int init_lock_pools (void)
{
    pthread_once (&pools_are_initialized, new_pools);
    if ( pools.initialized == 0 ) { return 0; }

    LOCK_CSEC_POOLS ();

    ++ref_count;
    if (ref_count >= UINT_MAX) { UNLOCK_CSEC_POOLS(); return 0; }

    UNLOCK_CSEC_POOLS ();
    return 1;
}

static void new_pools (void)
{
    memset (&pools, 0, sizeof (lock_pools_t));
    pools.user_pool.hash_table = g_hash_table_new_full (&g_str_hash, &g_str_equal, NULL, &free_lock);
    if ( pools.user_pool.hash_table == NULL ) {
        /* paranoia? */
        memset (&pools, 0, sizeof (lock_pools_t));
        return;
    }

    pools.group_pool.hash_table = g_hash_table_new_full (&g_str_hash, &g_str_equal, NULL, &free_lock);
    if ( pools.group_pool.hash_table == NULL ) {
        g_hash_table_destroy (pools.user_pool.hash_table);
        memset (&pools, 0, sizeof (lock_pools_t));
        return;
    }

    pthread_mutex_init (&pools.user_pool.csec, NULL);
    pthread_mutex_init (&pools.group_pool.csec, NULL);
    pthread_mutex_init (&pools.csec, NULL);

    pools.initialized = 1;
}

static void free_lock (gpointer lock)
{
    lock_t *const lck = (lock_t *) lock;

    pthread_mutex_unlock (&lck->mutex);
    pthread_mutex_destroy (&lck->mutex);

    free (lock);
}

void destroy_lock_pools (void)
{
    assert (pools.initialized == 1);

    LOCK_CSEC_POOLS ();

    --ref_count;

    if ( ref_count > 0 ) { UNLOCK_CSEC_POOLS(); return; }

    assert (pools.user_pool.hash_table != NULL);
    assert (pools.group_pool.hash_table != NULL);
    /* Remove all pairs. Memory is deallocated by free_lock callback. */
    g_hash_table_destroy (pools.user_pool.hash_table);
    g_hash_table_destroy (pools.group_pool.hash_table);

    /* What if mutex is locked? */
    pthread_mutex_destroy (&pools.user_pool.csec);
    pthread_mutex_destroy (&pools.group_pool.csec);

    UNLOCK_CSEC_POOLS ();

    pthread_mutex_destroy (&pools.csec);

    memset (&pools, 0, sizeof (lock_pools_t));

    pools_are_initialized = PTHREAD_ONCE_INIT;
}

/* search for keys */
static int search_key (lock_pool_t *const pool, const char *const username, lock_t **lck)
{
    assert (pool != NULL);

    *lck = (lock_t *) g_hash_table_lookup (pool->hash_table, username);
    if ( *lck != NULL ) { return 1; }

    return 0;
}

static int add_key (lock_pool_t *const pool, const char *const key)
{
    assert (pool != NULL);

    lock_t *const new_lock = calloc (1, sizeof (lock_t));
    if ( new_lock == NULL ) { return 0; }

    pthread_mutex_init (&new_lock->mutex, NULL);
    pthread_mutex_lock (&new_lock->mutex);
    new_lock->instances = 1;

    /* -1 for null char */
    strncpy (new_lock->id, key, sizeof (new_lock->id) - 1);
    g_hash_table_insert (pool->hash_table, (gpointer) new_lock->id, new_lock);
    return 1;
}

int get_user_lock (const char *const username)
{
    assert (pools.initialized == 1);

    LOCK_CSEC_USER ();
    lock_pool_t *const pool = &pools.user_pool;
    const int ret = get_lock (pool, username);
    UNLOCK_CSEC_USER ();
    return ret;
}

int get_group_lock (const char *const groupname)
{
    assert (pools.initialized == 1);

    LOCK_CSEC_GROUP ();
    lock_pool_t *const pool = &pools.group_pool;
    const int ret = get_lock (pool, groupname);
    UNLOCK_CSEC_GROUP ();
    return ret;
}

static int get_lock (lock_pool_t *const pool, const char *const key)
{
    assert (pool != NULL);

    lock_t *lck = NULL;
    int ret = search_key (pool, key, &lck);
    if ( ret == 1 ) {
        assert (lck != NULL);

        pthread_mutex_t *const mutex = &lck->mutex;
        if ( lck->instances >= UINT_MAX ) { return 0; }
        ++lck->instances;

        /* This will raise warning in covscan.
           But I just want return locked mutex. */
        pthread_mutex_lock (mutex);
        return 1;
    }
    /* no keys found - add new key to list */
    return (add_key (pool, key));
}

int release_user_lock (const char *const username)
{
    assert (pools.initialized == 1);
    LOCK_CSEC_USER ();
    lock_pool_t *const pool = &pools.user_pool;
    const int ret = release_lock (pool, username);
    UNLOCK_CSEC_USER ();
    return ret;
}

int release_group_lock (const char *const groupname)
{
    assert (pools.initialized == 1);
    LOCK_CSEC_GROUP ();
    lock_pool_t *const pool = &pools.group_pool;
    const int ret = release_lock (pool, groupname);
    UNLOCK_CSEC_GROUP ();
    return ret;
}

static int release_lock (lock_pool_t *const pool, const char *const key)
{
    assert (pool != NULL);
    lock_t *lck = NULL;
    int ret = search_key (pool, key, &lck);
    if ( ret == 0 ) { return ret; }

    assert (lck != NULL);

    --lck->instances;
    if ( lck->instances > 0 ) {
        pthread_mutex_unlock (&lck->mutex);
        return ret;
    }

    g_hash_table_remove (pool->hash_table, (gpointer) key);

    return ret;
}
