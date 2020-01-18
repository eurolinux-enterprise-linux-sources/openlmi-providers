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

#include <ctype.h>
#include <string.h>
#include <libudev.h>
#include <assert.h>
#include <stdint.h>
#include "openlmi.h"
#include "openlmi-utils.h"

struct _TreeKeyValueContainer {
    gpointer key;
    gpointer value;
};

static gboolean _do_tree_peek_first_item(gpointer key,
                                         gpointer value,
                                         gpointer data)
{
    struct _TreeKeyValueContainer *cnt = data;

    cnt->key = key;
    cnt->value = value;
    return true;
}

guint lmi_str_lcase_hash_func(gconstpointer key) {
    const gchar *strkey = key;
    int len = strlen(strkey);
    gchar tmp[BUFLEN];

    for (int i=0; i < len; ++i) {
        tmp[i] = tolower(strkey[i]);
    }
    tmp[len] = '\0';
    return g_str_hash(tmp);
}

gboolean lmi_str_equal(gconstpointer a, gconstpointer b)
{
    return !g_strcmp0((gchar const *) a, (gchar const *) b);
}

gboolean lmi_str_icase_equal(gconstpointer a, gconstpointer b)
{
    return !g_ascii_strcasecmp((gchar const *) a, (gchar const *) b);
}

gpointer lmi_tree_peek_first(GTree *tree, gpointer *value)
{
    struct _TreeKeyValueContainer data = {NULL, NULL};

    g_tree_foreach(tree, _do_tree_peek_first_item, &data);
    if (value)
        *value = data.value;
    return data.key;
}

gint lmi_tree_strcmp_func(gconstpointer a,
                          gconstpointer b,
                          gpointer user_data)
{
    const gchar *fst = (const gchar *) a;
    const gchar *snd = (const gchar *) b;

    return g_strcmp0(fst, snd);
}

gint lmi_tree_ptrcmp_func(gconstpointer a,
                          gconstpointer b,
                          gpointer unused)
{
    uintptr_t fst = (uintptr_t) a;
    uintptr_t snd = (uintptr_t) b;
    if (fst < snd)
        return -1;
    if (fst > snd)
        return 1;
    return 0;
}

void get_logfile_class_from_stat(const struct stat *sb, char *fileclass)
{
    (S_ISREG(sb->st_mode)) ? strcpy(fileclass, "LMI_DataFile") :
    (S_ISDIR(sb->st_mode)) ? strcpy(fileclass, "LMI_UnixDirectory") :
    (S_ISCHR(sb->st_mode)) ? strcpy(fileclass, "LMI_UnixDeviceFile") :
    (S_ISBLK(sb->st_mode)) ? strcpy(fileclass, "LMI_UnixDeviceFile") :
    (S_ISLNK(sb->st_mode)) ? strcpy(fileclass, "LMI_SymbolicLink") :
    (S_ISFIFO(sb->st_mode)) ? strcpy(fileclass, "LMI_FIFOPipeFile") :
    (S_ISSOCK(sb->st_mode)) ? strcpy(fileclass, "LMI_UnixSocket") :
    strcpy(fileclass, "Unknown");
    assert(strcmp(fileclass, "Unknown") != 0);
}

int get_logfile_class_from_path(const char *path, char *fileclass)
{
    int rc = 0;
    struct stat sb;

    if (lstat(path, &sb) < 0) {
        rc = 1;
    } else {
        get_logfile_class_from_stat(&sb, fileclass);
    }

    return rc;
}

CMPIStatus get_fsinfo_from_stat(const CMPIBroker *b, const struct stat *sb,
        const char *path, char **fsclassname, char **fsname)
{
    struct udev *udev_ctx;
    struct udev_device *udev_dev;
    const char *dev_name;
    CMPIStatus st = {.rc = CMPI_RC_OK};

    udev_ctx = udev_new();
    if (!udev_ctx) {
        lmi_return_with_status(b, &st, ERR_FAILED, "Could not create udev context");
    }

    udev_dev = udev_device_new_from_devnum(udev_ctx, 'b', sb->st_dev);
    if ((dev_name = udev_device_get_property_value(udev_dev, "ID_FS_UUID_ENC"))) {
        if (!*fsname) {
            if (asprintf(fsname, "UUID=%s", dev_name) < 0) {
                lmi_return_with_status(b, &st, ERR_FAILED, "asprintf failed");
            }
        }
        if (!*fsclassname) {
            *fsclassname = FSCREATIONCLASSNAME_LOCAL;
        }
    } else if ((dev_name = udev_device_get_property_value(udev_dev, "DEVNAME"))) {
        if (!*fsname) {
            if (asprintf(fsname, "DEVICE=%s", dev_name) < 0) {
                lmi_return_with_status(b, &st, ERR_FAILED, "asprintf failed");
            }
        }
        if (!*fsclassname) {
            *fsclassname = FSCREATIONCLASSNAME_LOCAL;
        }
    } else {
        if (!*fsname) {
            if (asprintf(fsname, "PATH=%s", path) < 0) {
                lmi_return_with_status(b, &st, ERR_FAILED, "asprintf failed");
            }
        }
        if (!*fsclassname) {
            *fsclassname = FSCREATIONCLASSNAME_TRANSIENT;
        }
    }
    udev_device_unref(udev_dev);
    udev_unref(udev_ctx);

    return st;
}

CMPIStatus get_fsinfo_from_path(const CMPIBroker *b, const char *path,
        char **fsclassname, char **fsname)
{
    CMPIStatus st = {.rc = CMPI_RC_OK};
    struct stat sb;

    if (lstat(path, &sb) < 0) {
        lmi_return_with_status(b, &st, ERR_FAILED, "lstat(2) failed");
    }

    return get_fsinfo_from_stat(b, &sb, path, fsclassname, fsname);
}
