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

#ifndef LSCPU_H_
#define LSCPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"


/* Processor from lscpu program. */
typedef struct _LscpuProcessor {
    unsigned data_width;        /* Data width */
    unsigned processors;        /* Number of processors */
    unsigned cores;             /* Cores per processor */
    unsigned threads_per_core;  /* Threads per core */
    char *stepping;             /* Stepping */
    unsigned current_speed;     /* Current speed in MHz */
} LscpuProcessor;


/*
 * Get processor structure according to the lscpu program.
 * @param cpu
 * @return 0 if success, negative value otherwise
 */
short lscpu_get_processor(LscpuProcessor *cpu);

/*
 * Free attributes in the lscpu structure.
 * @param cpu
 */
void lscpu_free_processor(LscpuProcessor *cpu);


#endif /* LSCPU_H_ */
