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

#include "virt_what.h"

short virt_what_get_virtual_type(char **virt)
{
    short ret = -1;
    unsigned buffer_size = 0;
    char **buffer = NULL;

    *virt = NULL;

    /* get virt-what output */
    if (run_command("virt-what", &buffer, &buffer_size) != 0) {
        goto done;
    }

    if (buffer_size > 0) {
        if (!(*virt = strdup(buffer[0]))) {
            goto done;
        }
    } else {
        if (!(*virt = strdup(""))) {
            goto done;
        }
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    return ret;
}
