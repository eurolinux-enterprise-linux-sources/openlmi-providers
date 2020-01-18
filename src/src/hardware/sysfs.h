/*
 * Copyright (C) 2013 Red Hat, Inc. All rights reserved.
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

#ifndef SYSFS_H_
#define SYSFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "utils.h"
#include "dmidecode.h"
#include "lscpu.h"

#define SYSFS_PATH "/sys/devices/system"
#define SYSFS_CPU_PATH SYSFS_PATH "/cpu"
#define SYSFS_KERNEL_MM "/sys/kernel/mm"


/* Transparent memory huge pages statuses. */
typedef enum _ThpStatus {
    thp_unsupported = 0,
    thp_never,
    thp_madvise,
    thp_always
} ThpStatus;


/* Processor cache from sysfs. */
typedef struct _SysfsCpuCache {
    char *id;                   /* ID */
    unsigned size;              /* Cache Size */
    char *name;                 /* Cache Name */
    unsigned level;             /* Cache Level */
    char *type;                 /* Cache Type (Data, Instruction, Unified..) */
    unsigned ways_of_assoc;     /* Number of ways of associativity */
    unsigned line_size;         /* Cache Line Size */
} SysfsCpuCache;


/*
 * Get array of processor caches from sysfs.
 * @param caches array of cpu caches, this function will allocate necessary
 *      memory, but caller is responsible for freeing it
 * @param caches_nb number of caches in caches
 * @return 0 if success, negative value otherwise
 */
short sysfs_get_cpu_caches(SysfsCpuCache **caches, unsigned *caches_nb);

/*
 * Free array of cpu cache structures.
 * @param caches array of caches
 * @param caches_nb number of caches
 */
void sysfs_free_cpu_caches(SysfsCpuCache **caches, unsigned *caches_nb);

/*
 * Detects whether system has NUMA memory layout.
 * @return 0 if no, 1 if yes
 */
short sysfs_has_numa();

/*
 * Get all supported sizes of memory huge pages in kB
 * @param sizes array of all supported sizes if huge pages
 * @param sizes_nb number of items in array
 * @return 0 if success, negative value otherwise
 */
short sysfs_get_sizes_of_hugepages(unsigned **sizes, unsigned *sizes_nb);

/*
 * Get current status of transparent memory huge pages.
 * @return ThpStatus
 */
ThpStatus sysfs_get_transparent_hugepages_status();


#endif /* SYSFS_H_ */
