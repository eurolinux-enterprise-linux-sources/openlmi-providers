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

#ifndef SMARTCTL_H_
#define SMARTCTL_H_

#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "utils.h"
#include "sysfs.h"

/* HDD from smartctl. */
typedef struct _SmartctlHdd {
    char *dev_path;                 /* /dev/path */
    char *dev_basename;             /* basename of /dev/path */
    char *manufacturer;             /* Manufacturer */
    char *model;                    /* Model */
    char *serial_number;            /* Serial Number */
    char *name;                     /* Name */
    char *smart_status;             /* SMART status */
    char *firmware;                 /* Firmware version */
    char *port_type;                /* Port Type: ATA, SATA or SATA 2 */
    unsigned short form_factor;     /* Form Factor */
    unsigned long port_speed;       /* Current Port Speed in b/s */
    unsigned long max_port_speed;   /* Max Port Speed in b/s */
    unsigned rpm;                   /* RPM of drive */
    unsigned long capacity;         /* Drive's capacity in bytes */
    short int curr_temp;            /* Current disk temperature in Celsius */
} SmartctlHdd;

/*
 * Get array of hard drives according to the smartclt program.
 * @param hdds array of hdds, this function will allocate necessary memory,
 *      but caller is responsible for freeing it
 * @param hdds_nb number of hard drives in hdds
 * @return 0 if success, negative value otherwise
 */
short smartctl_get_hdds(SmartctlHdd **hdds, unsigned *hdds_nb);

/*
 * Free array of hard drive structures.
 * @param hdds array of hdds
 * @param hdds_nb number of hdds
 */
void smartctl_free_hdds(SmartctlHdd **hdds, unsigned *hdds_nb);

#endif /* SMARTCTL_H_ */
