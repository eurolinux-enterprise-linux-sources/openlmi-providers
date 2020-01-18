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

#include "lscpu.h"

/*
 * Initialize LscpuProcessor attributes.
 * @param cpu
 */
void init_lscpuprocessor_struct(LscpuProcessor *cpu)
{
    cpu->data_width = 0;
    cpu->processors = 0;
    cpu->cores = 1;
    cpu->threads_per_core = 1;
    cpu->stepping = NULL;
    cpu->current_speed = 0;
}

/*
 * Check attributes of cpu structure and fill in defaults if needed.
 * @param cpu
 * @return 0 if success, negative value otherwise
 */
short check_lscpuprocessor_attributes(LscpuProcessor *cpu)
{
    short ret = -1;

    if (!cpu->stepping) {
        if (!(cpu->stepping = strdup(""))) {
            goto done;
        }
    }

    ret = 0;

done:
    if (ret != 0) {
        lmi_warn("Failed to allocate memory.");
    }

    return ret;
}

short lscpu_get_processor(LscpuProcessor *cpu)
{
    short ret = -1;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf;

    /* get lscpu output */
    if (run_command("lscpu", &buffer, &buffer_size) != 0) {
        goto done;
    }

    init_lscpuprocessor_struct(cpu);

    /* parse information about processor */
    for (i = 0; i < buffer_size; i++) {
        /* Data Width */
        buf = copy_string_part_after_delim(buffer[i], "CPU op-mode(s):");
        if (buf) {
            if (strstr(buf, "64")) {
                cpu->data_width = 64;
            } else if (strstr(buf, "32")) {
                cpu->data_width = 32;
            }
            free(buf);
            buf = NULL;
            continue;
        }
        /* Threads per core */
        buf = copy_string_part_after_delim(buffer[i], "Thread(s) per core:");
        if (buf) {
            sscanf(buf, "%u", &cpu->threads_per_core);
            free(buf);
            buf = NULL;
            continue;
        }
        /* Cores per processor */
        buf = copy_string_part_after_delim(buffer[i], "Core(s) per socket:");
        if (buf) {
            sscanf(buf, "%u", &cpu->cores);
            free(buf);
            buf = NULL;
            continue;
        }
        /* Number of processors */
        buf = copy_string_part_after_delim(buffer[i], "Socket(s):");
        if (buf) {
            sscanf(buf, "%u", &cpu->processors);
            free(buf);
            buf = NULL;
            continue;
        }
        /* Stepping */
        buf = copy_string_part_after_delim(buffer[i], "Stepping:");
        if (buf) {
            cpu->stepping = buf;
            buf = NULL;
            continue;
        }
        /* Current speed in MHz */
        buf = copy_string_part_after_delim(buffer[i], "CPU MHz:");
        if (buf) {
            sscanf(buf, "%u", &cpu->current_speed);
            free(buf);
            buf = NULL;
            continue;
        }
    }

    if (check_lscpuprocessor_attributes(cpu) != 0) {
        goto done;
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        lscpu_free_processor(cpu);
    }

    return ret;
}

void lscpu_free_processor(LscpuProcessor *cpus)
{
    if (!cpus) {
        return;
    }

    free(cpus->stepping);
    cpus->stepping = NULL;
}
