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

#ifndef PROCFS_H_
#define PROCFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"


/* Processor from /proc/cpuinfo file. */
typedef struct _CpuinfoProcessor {
    unsigned flags_nb;          /* Number of CPU Flags */
    char **flags;               /* CPU Flags */
    unsigned address_size;      /* Address Size / Address Width */
    char *model_name;           /* Model Name */
} CpuinfoProcessor;


/*
 * Get processor structure according to the /proc/cpuinfo file.
 * @param cpu
 * @return 0 if success, negative value otherwise
 */
short cpuinfo_get_processor(CpuinfoProcessor *cpu);

/*
 * Free attributes in the cpuinfo structure.
 * @param cpu structure
 */
void cpuinfo_free_processor(CpuinfoProcessor *cpu);

/*
 * Get total memory from /proc/meminfo file.
 * @return memory size or 0 in case of a problem
 */
unsigned long meminfo_get_memory_size();


#endif /* PROCFS_H_ */
