/*
 * Copyright (C) 2013-2014 Red Hat, Inc. All rights reserved.
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
 * Authors: Peter Schiffer <pschiffe@redhat.com>
 */

#ifndef LSBLK_H_
#define LSBLK_H_

#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "utils.h"

/* Disk by lsblk program. */
typedef struct _LsblkHdd {
    char *name;                 /* /dev/path as name */
    char *basename;             /* Basename of /dev/path */
    char *type;                 /* Disk type; disk for disk, rom for cdrom */
    char *model;                /* Model */
    char *serial;               /* Serial number */
    char *revision;             /* Revision */
    char *vendor;               /* Vendor */
    char *tran;                 /* Device transport type */
    unsigned long capacity;     /* Drive's capacity in bytes */
} LsblkHdd;

/*
 * Get array of disks from sysfs by lsblk program.
 * @param hdds array of disks, this function will allocate necessary
 *      memory, but caller is responsible for freeing it
 * @param hdds_nb number of disks in hdds
 * @return 0 if success, negative value otherwise
 */
short lsblk_get_hdds(LsblkHdd **hdds, unsigned *hdds_nb);

/*
 * Free array of disks.
 * @param hdds array of disks
 * @param hdds_nb number of disks in hdds
 */
void lsblk_free_hdds(LsblkHdd **hdds, unsigned *hdds_nb);

#endif /* LSBLK_H_ */
