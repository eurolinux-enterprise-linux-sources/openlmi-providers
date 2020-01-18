/*
 * Copyright (C) 2014 Red Hat, Inc.  All rights reserved.
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
 * Authors: Michal Minar <miminar@redhat.com>
 */

#ifndef OPENLMI_UTILS_H
#define OPENLMI_UTILS_H

#include <glib.h>
#include <sys/stat.h>
#include <cmpi/cmpidt.h>

#define FSCREATIONCLASSNAME_LOCAL "LMI_LocalFileSystem"
#define FSCREATIONCLASSNAME_TRANSIENT "LMI_TransientFileSystem"

/**
 * Hash function used in hash table treating its keys in case-insensitive way.
 */
guint lmi_str_lcase_hash_func(gconstpointer key);

/**
 * Comparison function for use within hash table treating its keys in
 * case-sensitive way. Can be used with `GHashTable`.
 */
gboolean lmi_str_equal(gconstpointer a, gconstpointer b);

/**
 * Comparison function for use within hash table treating its keys in
 * case-insensitive way.
 */
gboolean lmi_str_icase_equal(gconstpointer a, gconstpointer b);

/**
 * Get the first key/value pair from balanced binary tree.
 *
 * @param value If supplied, it will be filled with a pointer
 *      to associated value.
 * @return Pointer to key.
 */
gpointer lmi_tree_peek_first(GTree* tree, gpointer *value);

/**
 * Comparison function for use with glib's binary tree having c-strings as
 * keys.
 *
 * @param user_data Is ignored.
 * @return The result of `strcmp()` applied to the first two arguments.
 */
gint lmi_tree_strcmp_func(gconstpointer a,
                              gconstpointer b,
                              gpointer user_data);

/**
 * Use this as a comparison functions for binary tree where keys are strings.
 *
 * @returns Result of `g_strcmp0()` applied on first two arguments.
 */
gint lmi_tree_ptrcmp_func(gconstpointer a,
                          gconstpointer b,
                          gpointer unused);

/**
 * Determine logicalfile class from st_mode.
 *
 * @param sb Stat structure of file.
 * @param fileclass Will be filled with logicalfile class.
 */
void get_logfile_class_from_stat(const struct stat *sb, char *fileclass);

/**
 * Determine logicalfile class from file name.
 *
 * @param path File name.
 * @param Will be filled with logicalfile class.
 * @return 0 on success, positive value on failure.
 */
int get_logfile_class_from_path(const char *path, char *fileclass);

/**
 * Get file system info from stat struct.
 *
 * @param b Broker.
 * @param sb Stat structure.
 * @param path File name.
 * @param fsclassname File system class name.
 * @param fsname File system name.
 * @return CMPIStatus.
 */
CMPIStatus get_fsinfo_from_stat(const CMPIBroker *b, const struct stat *sb,
        const char *path, char **fsclassname, char **fsname);

/**
 * Get file system info from file name.
 *
 * @param b Broker.
 * @param path File name.
 * @param fsclassname File system class name.
 * @param fsname File system name.
 * @return CMPIStatus.
 */
CMPIStatus get_fsinfo_from_path(const CMPIBroker *b, const char *path,
        char **fsclassname, char **fsname);

#endif /* end of include guard: OPENLMI_UTILS_H */
