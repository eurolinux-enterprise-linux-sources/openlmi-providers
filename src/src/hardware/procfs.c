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

#include "procfs.h"


/*
 * Initialize CpuinfoProcessor attributes.
 * @param cpu
 */
void init_cpuinfoprocessor_struct(CpuinfoProcessor *cpu)
{
    cpu->flags_nb = 0;
    cpu->flags = NULL;
    cpu->address_size = 0;
    cpu->model_name = NULL;
}

/*
 * Check attributes of cpu structure and fill in defaults if needed.
 * @param cpu
 * @return 0 if success, negative value otherwise
 */
short check_cpuinfoprocessor_attributes(CpuinfoProcessor *cpu)
{
    short ret = -1;

    if (!cpu->model_name) {
        if (!(cpu->model_name = strdup(""))) {
            goto done;
        }
    }

    ret = 0;

done:
    if (ret != 0) {
        warn("Failed to allocate memory.");
    }

    return ret;
}

short cpuinfo_get_processor(CpuinfoProcessor *cpu)
{
    short ret = -1;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf = NULL;

    /* read /proc/cpuinfo file */
    if (read_file("/proc/cpuinfo", &buffer, &buffer_size) != 0) {
        goto done;
    }

    init_cpuinfoprocessor_struct(cpu);

    /* parse information about processor */
    for (i = 0; i < buffer_size; i++) {
        /* CPU Flags */
        buf = copy_string_part_after_delim(buffer[i], "flags\t\t: ");
        if (buf) {
            if (explode(buf, NULL, &cpu->flags, &cpu->flags_nb) != 0) {
                goto done;
            }
            free(buf);
            buf = NULL;
            continue;
        }
        /* Address Size */
        buf = copy_string_part_after_delim(buffer[i], " bits physical, ");
        if (buf) {
            sscanf(buf, "%u", &cpu->address_size);
            free(buf);
            buf = NULL;
            continue;
        }
        /* Model Name */
        buf = copy_string_part_after_delim(buffer[i], "model name\t: ");
        if (buf) {
            cpu->model_name = buf;
            buf = NULL;
            continue;
        }
    }

    if (check_cpuinfoprocessor_attributes(cpu) != 0) {
        goto done;
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);
    free(buf);

    if (ret != 0) {
        cpuinfo_free_processor(cpu);
    }

    return ret;
}

void cpuinfo_free_processor(CpuinfoProcessor *cpu)
{
    unsigned i;

    if (!cpu) {
        return;
    }

    if (cpu->flags && cpu->flags_nb > 0) {
        for (i = 0; i < cpu->flags_nb; i++) {
            free(cpu->flags[i]);
            cpu->flags[i] = NULL;
        }
        free(cpu->flags);
    }
    cpu->flags_nb = 0;
    cpu->flags = NULL;

    free(cpu->model_name);
    cpu->model_name = NULL;
}

unsigned long meminfo_get_memory_size()
{
    unsigned long ret = 0;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf;

    /* read /proc/meminfo file */
    if (read_file("/proc/meminfo", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* read memory size */
    for (i = 0; i < buffer_size; i++) {
        /* memory size */
        buf = copy_string_part_after_delim(buffer[i], "MemTotal:");
        if (buf) {
            sscanf(buf, "%lu", &ret);
            ret *= 1024; /* it's in kB, we want B */
            free(buf);
            buf = NULL;
            break;
        }
    }

done:
    free_2d_buffer(&buffer, &buffer_size);

    return ret;
}
