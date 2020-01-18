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

#include "dmidecode.h"

/******************************************************************************
 * DmiProcessor
 */

/*
 * Initialize DmiProcessor attributes.
 * @param cpu
 */
void init_dmiprocessor_struct(DmiProcessor *cpu)
{
    cpu->id = NULL;
    cpu->family = NULL;
    cpu->status = NULL;
    cpu->current_speed = 0;
    cpu->max_speed = 0;
    cpu->external_clock = 0;
    cpu->name = NULL;
    cpu->cores = 1;
    cpu->enabled_cores = 1;
    cpu->threads = 1;
    cpu->type = NULL;
    cpu->stepping = NULL;
    cpu->upgrade = NULL;
    cpu->charact_nb = 0;
    cpu->characteristics = NULL;
    cpu->l1_cache_handle = NULL;
    cpu->l2_cache_handle = NULL;
    cpu->l3_cache_handle = NULL;
    cpu->manufacturer = NULL;
    cpu->serial_number = NULL;
    cpu->part_number = NULL;
}

/*
 * Check attributes of cpu structure and fill in defaults if needed.
 * @param cpu
 * @return 0 if success, negative value otherwise
 */
short check_dmiprocessor_attributes(DmiProcessor *cpu)
{
    short ret = -1;

    if (!cpu->id) {
        if (!(cpu->id = strdup(""))) {
            goto done;
        }
    }
    if (!cpu->family) {
        if (!(cpu->family = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!cpu->status) {
        if (!(cpu->status = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!cpu->name) {
        if (!(cpu->name = strdup(""))) {
            goto done;
        }
    }
    if (!cpu->type) {
        if (!(cpu->type = strdup(""))) {
            goto done;
        }
    }
    if (!cpu->stepping) {
        if (!(cpu->stepping = strdup(""))) {
            goto done;
        }
    }
    if (!cpu->upgrade) {
        if (!(cpu->upgrade = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!cpu->l1_cache_handle) {
        if (!(cpu->l1_cache_handle = strdup(""))) {
            goto done;
        }
    }
    if (!cpu->l2_cache_handle) {
        if (!(cpu->l2_cache_handle = strdup(""))) {
            goto done;
        }
    }
    if (!cpu->l3_cache_handle) {
        if (!(cpu->l3_cache_handle = strdup(""))) {
            goto done;
        }
    }
    if (!cpu->manufacturer) {
        if (!(cpu->manufacturer = strdup(""))) {
            goto done;
        }
    }
    if (!cpu->serial_number) {
        if (!(cpu->serial_number = strdup(""))) {
            goto done;
        }
    }
    if (!cpu->part_number) {
        if (!(cpu->part_number = strdup(""))) {
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

short dmi_get_processors(DmiProcessor **cpus, unsigned *cpus_nb)
{
    short ret = -1;
    int curr_cpu = -1;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf;

    dmi_free_processors(cpus, cpus_nb);

    /* get dmidecode output */
    if (run_command("dmidecode -t 4", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* count processors */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            (*cpus_nb)++;
        }
    }

    /* if no processor was found */
    if (*cpus_nb < 1) {
        lmi_warn("Dmidecode didn't recognize any processor.");
        goto done;
    }

    /* allocate memory for processors */
    *cpus = (DmiProcessor *)calloc(*cpus_nb, sizeof(DmiProcessor));
    if (!(*cpus)) {
        lmi_warn("Failed to allocate memory.");
        *cpus_nb = 0;
        goto done;
    }

    /* parse information about processors */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            curr_cpu++;
            init_dmiprocessor_struct(&(*cpus)[curr_cpu]);
            continue;
        }
        /* ignore first useless lines */
        if (curr_cpu < 0) {
            continue;
        }
        /* ID */
        buf = copy_string_part_after_delim(buffer[i], "ID: ");
        if (buf) {
            char curr_cpu_str[LONG_INT_LEN];
            snprintf(curr_cpu_str, LONG_INT_LEN, "-%u", curr_cpu);
            (*cpus)[curr_cpu].id = append_str(buf, curr_cpu_str, NULL);
            if (!(*cpus)[curr_cpu].id) {
                goto done;
            }
            buf = NULL;
            continue;
        }
        /* Family */
        buf = copy_string_part_after_delim(buffer[i], "Family: ");
        if (buf) {
            (*cpus)[curr_cpu].family = buf;
            buf = NULL;
            continue;
        }
        /* Manufacturer */
        buf = copy_string_part_after_delim(buffer[i], "Manufacturer: ");
        if (buf) {
            (*cpus)[curr_cpu].manufacturer = buf;
            buf = NULL;
            continue;
        }
        /* Status */
        buf = copy_string_part_after_delim(buffer[i], "Status: Populated, ");
        if (buf) {
            (*cpus)[curr_cpu].status = buf;
            buf = NULL;
            continue;
        }
        /* Current Speed */
        buf = copy_string_part_after_delim(buffer[i], "Current Speed: ");
        if (buf) {
            if (strcmp(buf, "Unknown") != 0) {
                sscanf(buf, "%u", &(*cpus)[curr_cpu].current_speed);
            }
            free(buf);
            buf = NULL;
            continue;
        }
        /* Max Speed */
        buf = copy_string_part_after_delim(buffer[i], "Max Speed: ");
        if (buf) {
            if (strcmp(buf, "Unknown") != 0) {
                sscanf(buf, "%u", &(*cpus)[curr_cpu].max_speed);
            }
            free(buf);
            buf = NULL;
            continue;
        }
        /* External Clock Speed */
        buf = copy_string_part_after_delim(buffer[i], "External Clock: ");
        if (buf) {
            if (strcmp(buf, "Unknown") != 0) {
                sscanf(buf, "%u", &(*cpus)[curr_cpu].external_clock);
            }
            free(buf);
            buf = NULL;
            continue;
        }
        /* CPU Name */
        buf = copy_string_part_after_delim(buffer[i], "Version: ");
        if (buf) {
            (*cpus)[curr_cpu].name = buf;
            buf = NULL;
            continue;
        }
        /* Cores */
        buf = copy_string_part_after_delim(buffer[i], "Core Count: ");
        if (buf) {
            sscanf(buf, "%u", &(*cpus)[curr_cpu].cores);
            free(buf);
            buf = NULL;
            continue;
        }
        /* Enabled Cores */
        buf = copy_string_part_after_delim(buffer[i], "Core Enabled: ");
        if (buf) {
            sscanf(buf, "%u", &(*cpus)[curr_cpu].enabled_cores);
            free(buf);
            buf = NULL;
            continue;
        }
        /* Threads */
        buf = copy_string_part_after_delim(buffer[i], "Thread Count: ");
        if (buf) {
            sscanf(buf, "%u", &(*cpus)[curr_cpu].threads);
            free(buf);
            buf = NULL;
            continue;
        }
        /* CPU Type/Role */
        buf = copy_string_part_after_delim(buffer[i], "Type: ");
        if (buf) {
            (*cpus)[curr_cpu].type = buf;
            buf = NULL;
            continue;
        }
        /* Stepping */
        buf = copy_string_part_after_delim(buffer[i], ", Stepping ");
        if (buf) {
            (*cpus)[curr_cpu].stepping = buf;
            buf = NULL;
            continue;
        }
        /* Upgrade */
        buf = copy_string_part_after_delim(buffer[i], "Upgrade: ");
        if (buf) {
            (*cpus)[curr_cpu].upgrade = buf;
            buf = NULL;
            continue;
        }
        /* Level 1 Cache Handle */
        buf = copy_string_part_after_delim(buffer[i], "L1 Cache Handle: ");
        if (buf) {
            (*cpus)[curr_cpu].l1_cache_handle = buf;
            buf = NULL;
            continue;
        }
        /* Level 2 Cache Handle */
        buf = copy_string_part_after_delim(buffer[i], "L2 Cache Handle: ");
        if (buf) {
            (*cpus)[curr_cpu].l2_cache_handle = buf;
            buf = NULL;
            continue;
        }
        /* Level 3 Cache Handle */
        buf = copy_string_part_after_delim(buffer[i], "L3 Cache Handle: ");
        if (buf) {
            (*cpus)[curr_cpu].l3_cache_handle = buf;
            buf = NULL;
            continue;
        }
        /* Serial Number */
        buf = copy_string_part_after_delim(buffer[i], "Serial Number: ");
        if (buf) {
            (*cpus)[curr_cpu].serial_number = buf;
            buf = NULL;
            continue;
        }
        /* Part Number */
        buf = copy_string_part_after_delim(buffer[i], "Part Number: ");
        if (buf) {
            (*cpus)[curr_cpu].part_number = buf;
            buf = NULL;
            continue;
        }
        /* CPU Characteristics */
        if (strstr(buffer[i], "Characteristics:")
                && !strstr(buffer[i], "Characteristics: ")) {
            /* count characteristics */
            (*cpus)[curr_cpu].charact_nb = 0;
            while (strlen(buffer[i + (*cpus)[curr_cpu].charact_nb + 1])) {
                (*cpus)[curr_cpu].charact_nb += 1;
            }
            /* allocate memory */
            (*cpus)[curr_cpu].characteristics =
                    (char **)calloc((*cpus)[curr_cpu].charact_nb, sizeof(char *));
            if (!(*cpus)[curr_cpu].characteristics) {
                lmi_warn("Failed to allocate memory.");
                (*cpus)[curr_cpu].charact_nb = 0;
                goto done;
            }
            unsigned j;
            char *tmp_line;
            for (j = 0; j < (*cpus)[curr_cpu].charact_nb; j++) {
                tmp_line = trim(buffer[i + j + 1], NULL);
                if (tmp_line) {
                    (*cpus)[curr_cpu].characteristics[j] = tmp_line;
                } else {
                    (*cpus)[curr_cpu].characteristics[j] = strdup("");
                    if (!(*cpus)[curr_cpu].characteristics[j]) {
                        lmi_warn("Failed to allocate memory.");
                        goto done;
                    }
                }
            }
            /* skip characteristics and newline after them */
            i += (*cpus)[curr_cpu].charact_nb + 1;
        }
    }

    /* fill in default attributes if needed */
    for (i = 0; i < *cpus_nb; i++) {
        if (check_dmiprocessor_attributes(&(*cpus)[i]) != 0) {
            goto done;
        }
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        dmi_free_processors(cpus, cpus_nb);
    }

    return ret;
}

void dmi_free_processors(DmiProcessor **cpus, unsigned *cpus_nb)
{
    unsigned i, j;

    if (*cpus && *cpus_nb > 0) {
        for (i = 0; i < *cpus_nb; i++) {
            free((*cpus)[i].id);
            (*cpus)[i].id = NULL;
            free((*cpus)[i].family);
            (*cpus)[i].family = NULL;
            free((*cpus)[i].status);
            (*cpus)[i].status = NULL;
            free((*cpus)[i].name);
            (*cpus)[i].name = NULL;
            free((*cpus)[i].type);
            (*cpus)[i].type = NULL;
            free((*cpus)[i].stepping);
            (*cpus)[i].stepping = NULL;
            free((*cpus)[i].upgrade);
            (*cpus)[i].upgrade = NULL;

            if ((*cpus)[i].characteristics && (*cpus)[i].charact_nb > 0) {
                for (j = 0; j < (*cpus)[i].charact_nb; j++) {
                    free((*cpus)[i].characteristics[j]);
                    (*cpus)[i].characteristics[j] = NULL;
                }
                free((*cpus)[i].characteristics);
            }
            (*cpus)[i].charact_nb = 0;
            (*cpus)[i].characteristics = NULL;

            free((*cpus)[i].l1_cache_handle);
            (*cpus)[i].l1_cache_handle = NULL;
            free((*cpus)[i].l2_cache_handle);
            (*cpus)[i].l2_cache_handle = NULL;
            free((*cpus)[i].l3_cache_handle);
            (*cpus)[i].l3_cache_handle = NULL;
            free((*cpus)[i].manufacturer);
            (*cpus)[i].manufacturer = NULL;
            free((*cpus)[i].serial_number);
            (*cpus)[i].serial_number = NULL;
            free((*cpus)[i].part_number);
            (*cpus)[i].part_number = NULL;
        }
        free(*cpus);
    }

    *cpus_nb = 0;
    *cpus = NULL;
}


/******************************************************************************
 * DmiCpuCache
 */

/*
 * Initialize DmiCpuCache attributes.
 * @param cache
 */
void init_dmi_cpu_cache_struct(DmiCpuCache *cache)
{
    cache->id = NULL;
    cache->size = 0;
    cache->name = NULL;
    cache->status = NULL;
    cache->level = 0;
    cache->op_mode = NULL;
    cache->type = NULL;
    cache->associativity = NULL;
}

/*
 * Check attributes of cache structure and fill in defaults if needed.
 * @param cache
 * @return 0 if success, negative value otherwise
 */
short check_dmi_cpu_cache_attributes(DmiCpuCache *cache)
{
    short ret = -1;

    if (!cache->id) {
        if (!(cache->id = strdup(""))) {
            goto done;
        }
    }
    if (!cache->name) {
        if (!(cache->name = strdup(""))) {
            goto done;
        }
    }
    if (!cache->status) {
        if (!(cache->status = strdup(""))) {
            goto done;
        }
    }
    if (!cache->op_mode) {
        if (!(cache->op_mode = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!cache->type) {
        if (!(cache->type = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!cache->associativity) {
        if (!(cache->associativity = strdup("Unknown"))) {
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

short dmi_get_cpu_caches(DmiCpuCache **caches, unsigned *caches_nb)
{
    short ret = -1;
    int curr_cache = -1;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf;

    dmi_free_cpu_caches(caches, caches_nb);

    /* get dmidecode output */
    if (run_command("dmidecode -t 7", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* count caches */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            (*caches_nb)++;
        }
    }

    /* if no cache was found */
    if (*caches_nb < 1) {
        lmi_warn("Dmidecode didn't recognize any processor cache memory.");
        goto done;
    }

    /* allocate memory for caches */
    *caches = (DmiCpuCache *)calloc(*caches_nb, sizeof(DmiCpuCache));
    if (!(*caches)) {
        lmi_warn("Failed to allocate memory.");
        *caches_nb = 0;
        goto done;
    }

    /* parse information about cache */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            curr_cache++;
            init_dmi_cpu_cache_struct(&(*caches)[curr_cache]);

            /* Cache ID is it's handle */
            char *id_start = buffer[i] + 7;
            char *id_end = strchr(buffer[i], ',');
            if (!id_end) {
                lmi_warn("Unrecognized output from dmidecode program.");
                goto done;
            }
            (*caches)[curr_cache].id = strndup(id_start, id_end - id_start);
            if (!(*caches)[curr_cache].id) {
                lmi_warn("Failed to allocate memory.");
                goto done;
            }

            continue;
        }
        /* ignore first useless lines */
        if (curr_cache < 0) {
            continue;
        }
        /* Cache Name */
        buf = copy_string_part_after_delim(buffer[i], "Socket Designation: ");
        if (buf) {
            (*caches)[curr_cache].name = buf;
            buf = NULL;
            continue;
        }
        /* Cache Status and Level */
        buf = copy_string_part_after_delim(buffer[i], "Configuration: ");
        if (buf) {
            char **confs = NULL;
            unsigned confs_len = 0;
            if (explode(buf, ",", &confs, &confs_len) != 0 ||
                    confs_len < 3) {
                free_2d_buffer(&confs, &confs_len);
                free(buf);
                buf = NULL;
                continue;
            }

            (*caches)[curr_cache].status = trim(confs[0], NULL);
            sscanf(confs[2], "%*s %u", &(*caches)[curr_cache].level);

            free_2d_buffer(&confs, &confs_len);
            free(buf);
            buf = NULL;
            continue;
        }
        /* Cache Operational Mode */
        buf = copy_string_part_after_delim(buffer[i], "Operational Mode: ");
        if (buf) {
            (*caches)[curr_cache].op_mode = buf;
            buf = NULL;
            continue;
        }
        /* Cache Size */
        buf = copy_string_part_after_delim(buffer[i], "Installed Size: ");
        if (buf) {
            sscanf(buf, "%u", &(*caches)[curr_cache].size);
            (*caches)[curr_cache].size *= 1024;     /* It's in kB, we want B */
            free(buf);
            buf = NULL;
            continue;
        }
        /* Cache Type */
        buf = copy_string_part_after_delim(buffer[i], "System Type: ");
        if (buf) {
            (*caches)[curr_cache].type = buf;
            buf = NULL;
            continue;
        }
        /* Cache Associativity */
        buf = copy_string_part_after_delim(buffer[i], "Associativity: ");
        if (buf) {
            (*caches)[curr_cache].associativity = buf;
            buf = NULL;
            continue;
        }
    }

    /* fill in default attributes if needed */
    for (i = 0; i < *caches_nb; i++) {
        if (check_dmi_cpu_cache_attributes(&(*caches)[i]) != 0) {
            goto done;
        }
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        dmi_free_cpu_caches(caches, caches_nb);
    }

    return ret;
}

void dmi_free_cpu_caches(DmiCpuCache **caches, unsigned *caches_nb)
{
    unsigned i;

    if (*caches && *caches_nb > 0) {
        for (i = 0; i < *caches_nb; i++) {
            free((*caches)[i].id);
            (*caches)[i].id = NULL;
            free((*caches)[i].name);
            (*caches)[i].name = NULL;
            free((*caches)[i].status);
            (*caches)[i].status = NULL;
            free((*caches)[i].op_mode);
            (*caches)[i].op_mode = NULL;
            free((*caches)[i].type);
            (*caches)[i].type = NULL;
            free((*caches)[i].associativity);
            (*caches)[i].associativity = NULL;
        }
        free(*caches);
    }

    *caches_nb = 0;
    *caches = NULL;
}


/******************************************************************************
 * DmiMemory
 */

/*
 * Initialize DmiMemory attributes.
 * @param memory
 */
void init_dmi_memory_struct(DmiMemory *memory)
{
    memory->physical_size = 0;
    memory->available_size = 0;
    memory->start_addr = 0;
    memory->end_addr = 0;
    memory->modules = NULL;
    memory->modules_nb = 0;
    memory->slots = NULL;
    memory->slots_nb = 0;
}

/*
 * Initialize DmiMemoryModule attributes.
 * @param mem
 */
void init_dmi_memory_module_struct(DmiMemoryModule *mem)
{
    mem->size = 0;
    mem->serial_number = NULL;
    mem->form_factor = NULL;
    mem->type = NULL;
    mem->slot = -1;
    mem->speed_time = 0;
    mem->bank_label = NULL;
    mem->name = NULL;
    mem->manufacturer = NULL;
    mem->part_number = NULL;
    mem->speed_clock = 0;
    mem->data_width = 0;
    mem->total_width = 0;
}

/*
 * Initialize DmiMemorySlot attributes.
 * @param slot
 */
void init_dmi_memory_slot_struct(DmiMemorySlot *slot)
{
    slot->slot_number = -1;
    slot->name = NULL;
}

/*
 * Check attributes of memory structure and fill in defaults if needed.
 * @param memory
 * @return 0 if success, negative value otherwise
 */
short check_dmi_memory_attributes(DmiMemory *memory)
{
    short ret = -1;
    unsigned i;

    for (i = 0; i < memory->modules_nb; i++) {
        if (!memory->modules[i].serial_number) {
            if (asprintf(&memory->modules[i].serial_number, "%u", i) < 0) {
                memory->modules[i].serial_number = NULL;
                goto done;
            }
        }
        if (!memory->modules[i].form_factor) {
            if (!(memory->modules[i].form_factor = strdup("Unknown"))) {
                goto done;
            }
        }
        if (!memory->modules[i].type) {
            if (!(memory->modules[i].type = strdup("Unknown"))) {
                goto done;
            }
        }
        if (!memory->modules[i].bank_label) {
            if (!(memory->modules[i].bank_label = strdup(""))) {
                goto done;
            }
        }
        if (!memory->modules[i].name) {
            if (!(memory->modules[i].name = strdup("Memory Module"))) {
                goto done;
            }
        }
        if (!memory->modules[i].manufacturer) {
            if (!(memory->modules[i].manufacturer = strdup(""))) {
                goto done;
            }
        }
        if (!memory->modules[i].part_number) {
            if (!(memory->modules[i].part_number = strdup(""))) {
                goto done;
            }
        }
    }

    for (i = 0; i < memory->slots_nb; i++) {
        if (memory->slots[i].slot_number < 0) {
            memory->slots[i].slot_number = i;
        }
        if (!memory->slots[i].name) {
            if (asprintf(&memory->slots[i].name, "Memory Slot %d",
                    memory->slots[i].slot_number) < 0) {
                memory->slots[i].name = NULL;
                goto done;
            }
        }
    }

    ret = 0;

done:
    if (ret != 0) {
        lmi_warn("Failed to allocate memory.");
    }

    return ret;
}

short dmi_get_memory(DmiMemory *memory)
{
    short ret = -1;
    int curr_mem = -1, last_mem = -1, slot = -1;
    unsigned i, j, buffer_size = 0, data_width = 0, total_width = 0,
            current_slot = 0;
    unsigned long memory_size;
    char **buffer = NULL, *buf, *str_format;

    init_dmi_memory_struct(memory);

    /* get dmidecode output for memory modules */
    if (run_command("dmidecode -t 17", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* count modules */
    for (i = 0; i < buffer_size; i++) {
        if (strstr(buffer[i], "Size: ") &&
                !strstr(buffer[i], "Size: No Module Installed")) {
            memory->modules_nb++;
        }
    }

    /* if no module was found */
    if (memory->modules_nb < 1) {
        lmi_warn("Dmidecode didn't recognize any memory module.");
        goto done;
    }

    /* allocate memory for modules */
    memory->modules = (DmiMemoryModule *)calloc(memory->modules_nb, sizeof(DmiMemoryModule));
    if (!memory->modules) {
        lmi_warn("Failed to allocate memory.");
        memory->modules_nb = 0;
        goto done;
    }

    /* parse information about modules */
    for (i = 0; i < buffer_size; i++) {
        /* Total Width */
        buf = copy_string_part_after_delim(buffer[i], "Total Width: ");
        if (buf) {
            sscanf(buf, "%u", &total_width);
            free(buf);
            buf = NULL;
            continue;
        }
        /* Memory Module Data Width */
        buf = copy_string_part_after_delim(buffer[i], "Data Width: ");
        if (buf) {
            sscanf(buf, "%u", &data_width);
            free(buf);
            buf = NULL;
            continue;
        }
        if (strstr(buffer[i], "Size: ")) {
            if (strstr(buffer[i], "Size: No Module Installed")) {
                curr_mem = -1;
                continue;
            } else {
                curr_mem = ++last_mem;
                init_dmi_memory_module_struct(&memory->modules[curr_mem]);
                /* Module Size */
                buf = copy_string_part_after_delim(buffer[i], "Size: ");
                if (buf) {
                    sscanf(buf, "%lu", &memory->modules[curr_mem].size);
                    memory->modules[curr_mem].size *= 1048576; /* It's in MB, we want B */
                    memory->physical_size += memory->modules[curr_mem].size;
                    free(buf);
                    buf = NULL;
                }
                /*Pretty, human readable module name */
                if (memory->modules[curr_mem].size >= 1073741824) {
                    memory_size = memory->modules[curr_mem].size / 1073741824;
                    str_format = "%lu GB Memory Module";
                } else {
                    memory_size = memory->modules[curr_mem].size / 1048576;
                    str_format = "%lu MB Memory Module";
                }
                if (asprintf(&memory->modules[curr_mem].name, str_format, memory_size) < 0) {
                    memory->modules[curr_mem].name = NULL;
                    lmi_warn("Failed to allocate memory.");
                    goto done;
                }

                memory->modules[curr_mem].total_width = total_width;
                memory->modules[curr_mem].data_width = data_width;
                total_width = 0;
                data_width = 0;

                lmi_debug("Found %s at index %d",
                        memory->modules[curr_mem].name, curr_mem);

                continue;
            }
        }
        /* ignore useless lines */
        if (curr_mem < 0) {
            continue;
        }
        /* Serial Number */
        buf = copy_string_part_after_delim(buffer[i], "Serial Number: ");
        if (buf) {
            if (strcmp(buf, "Not Specified") != 0) {
                memory->modules[curr_mem].serial_number = buf;
                buf = NULL;
            } else {
                free(buf);
                buf = NULL;
                if (asprintf(&memory->modules[curr_mem].serial_number, "%u", curr_mem) < 0) {
                    memory->modules[curr_mem].serial_number = NULL;
                    lmi_warn("Failed to allocate memory.");
                    goto done;
                }
            }
            continue;
        }
        /* Memory Module Bank Label and Slot ID */
        buf = copy_string_part_after_delim(buffer[i], "Locator: ");
        if (buf) {
            if (memory->modules[curr_mem].slot != -1
                    || strncasecmp(buf, "bank ", 5) != 0) {
                free(buf);
                buf = NULL;
                continue;
            }
            sscanf(buf, "%*s %d", &memory->modules[curr_mem].slot);
            memory->modules[curr_mem].bank_label = buf;
            buf = NULL;
            continue;
        }
        /* Form Factor */
        buf = copy_string_part_after_delim(buffer[i], "Form Factor: ");
        if (buf) {
            memory->modules[curr_mem].form_factor = buf;
            buf = NULL;
            continue;
        }
        /* Memory Module Type */
        buf = copy_string_part_after_delim(buffer[i], "Type: ");
        if (buf) {
            memory->modules[curr_mem].type = buf;
            buf = NULL;
            continue;
        }
        /* Manufacturer */
        buf = copy_string_part_after_delim(buffer[i], "Manufacturer: ");
        if (buf) {
            memory->modules[curr_mem].manufacturer = buf;
            buf = NULL;
            continue;
        }
        /* Part Number */
        buf = copy_string_part_after_delim(buffer[i], "Part Number: ");
        if (buf) {
            memory->modules[curr_mem].part_number = buf;
            buf = NULL;
            continue;
        }
        /* Speed in MHz */
        buf = copy_string_part_after_delim(buffer[i], "Speed: ");
        if (buf) {
            if (strstr(buffer[i], "Configured Clock Speed: ")) {
                sscanf(buf, "%u", &memory->modules[curr_mem].speed_clock);
            } else if (memory->modules[curr_mem].speed_clock == 0) {
                sscanf(buf, "%u", &memory->modules[curr_mem].speed_clock);
            }
            free(buf);
            buf = NULL;
            continue;
        }
    }

    free_2d_buffer(&buffer, &buffer_size);

    /* get additional dmidecode output for memory modules */
    if (run_command("dmidecode -t 6", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* count slots */
    for (i = 0; i < buffer_size; i++) {
        if (strstr(buffer[i], "Socket Designation: ")) {
            memory->slots_nb++;
        }
    }

    /* allocate memory for slots */
    memory->slots = (DmiMemorySlot *)calloc(memory->slots_nb, sizeof(DmiMemorySlot));
    if (!memory->slots) {
        lmi_warn("Failed to allocate memory.");
        memory->slots_nb = 0;
        goto done;
    }

    /* parse additional information about modules */
    for (i = 0; i < buffer_size; i++) {
        /* Slot */
        buf = copy_string_part_after_delim(buffer[i], "Socket Designation: ");
        if (buf) {
            init_dmi_memory_slot_struct(&memory->slots[current_slot]);
            if (sscanf(buf, "%*s %*s %d", &slot) == 1) {
                memory->slots[current_slot].slot_number = slot;
            }
            memory->slots[current_slot].name = buf;
            current_slot++;
            buf = NULL;
            continue;
        }
        /* ignore useless lines */
        if (slot < 0) {
            continue;
        }
        /* Memory Speed */
        buf = copy_string_part_after_delim(buffer[i], "Current Speed: ");
        if (buf) {
            if (slot != -1) {
                for (j = 0; j < memory->modules_nb; j++) {
                    if (memory->modules[j].slot == slot) {
                        sscanf(buf, "%u", &memory->modules[j].speed_time);
                        break;
                    }
                }
            }
            free(buf);
            buf = NULL;
            slot = -1;
        }
    }

    free_2d_buffer(&buffer, &buffer_size);

    /* get dmidecode output for memory array */
    if (run_command("dmidecode -t 19", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* parse information about memory array */
    short first_array = 1;
    unsigned long start_addr, end_addr;
    for (i = 0; i < buffer_size; i++) {
        /* Starting Address */
        buf = copy_string_part_after_delim(buffer[i], "Starting Address: ");
        if (buf) {
            sscanf(buf, "%lx", &start_addr);
            if (first_array || start_addr < memory->start_addr) {
                memory->start_addr = start_addr / 1024; /* in Bytes, we want KB */
                first_array = 0;
            }
            free(buf);
            buf = NULL;
            continue;
        }
        /* Ending Address */
        buf = copy_string_part_after_delim(buffer[i], "Ending Address: ");
        if (buf) {
            sscanf(buf, "%lx", &end_addr);
            if (end_addr > memory->end_addr) {
                memory->end_addr = end_addr / 1024; /* in Bytes, we want KB */
            }
            memory->available_size += end_addr - start_addr;
            free(buf);
            buf = NULL;
            continue;
        }
    }

    /* fill in default attributes if needed */
    if (check_dmi_memory_attributes(memory) != 0) {
        goto done;
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        dmi_free_memory(memory);
    }

    return ret;
}

void dmi_free_memory(DmiMemory *memory)
{
    unsigned i;

    if (!memory) {
        return;
    }

    if (memory->modules && memory->modules_nb > 0) {
        for (i = 0; i < memory->modules_nb; i++) {
            free(memory->modules[i].serial_number);
            memory->modules[i].serial_number = NULL;
            free(memory->modules[i].form_factor);
            memory->modules[i].form_factor = NULL;
            free(memory->modules[i].type);
            memory->modules[i].type = NULL;
            free(memory->modules[i].bank_label);
            memory->modules[i].bank_label = NULL;
            free(memory->modules[i].name);
            memory->modules[i].name = NULL;
            free(memory->modules[i].manufacturer);
            memory->modules[i].manufacturer = NULL;
            free(memory->modules[i].part_number);
            memory->modules[i].part_number = NULL;
        }
        free(memory->modules);
    }
    if (memory->slots && memory->slots_nb > 0) {
        for (i = 0; i < memory->slots_nb; i++) {
            free(memory->slots[i].name);
            memory->slots[i].name = NULL;
        }
        free(memory->slots);
    }
    memory->modules_nb = 0;
    memory->modules = NULL;
    memory->slots_nb = 0;
    memory->slots = NULL;
}


/******************************************************************************
 * DmiChassis
 */

/*
 * Initialize DmiChassis attributes.
 * @param chassis
 */
void init_dmi_chassis_struct(DmiChassis *chassis)
{
    chassis->serial_number = NULL;
    chassis->type = NULL;
    chassis->manufacturer = NULL;
    chassis->sku_number = NULL;
    chassis->version = NULL;
    chassis->has_lock = 0;
    chassis->power_cords = 0;
    chassis->asset_tag = NULL;
    chassis->model = NULL;
    chassis->product_name = NULL;
    chassis->uuid = NULL;
}

/*
 * Check attributes of chassis structure and fill in defaults if needed.
 * @param chassis
 * @return 0 if success, negative value otherwise
 */
short check_dmi_chassis_attributes(DmiChassis *chassis)
{
    short ret = -1;

    if (!chassis->serial_number) {
        if (!(chassis->serial_number = strdup("Not Specified"))) {
            goto done;
        }
    }
    if (!chassis->type) {
        if (!(chassis->type = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!chassis->manufacturer) {
        if (!(chassis->manufacturer = strdup(""))) {
            goto done;
        }
    }
    if (!chassis->sku_number) {
        if (!(chassis->sku_number = strdup(""))) {
            goto done;
        }
    }
    if (!chassis->version) {
        if (!(chassis->version = strdup(""))) {
            goto done;
        }
    }
    if (!chassis->asset_tag) {
        if (!(chassis->asset_tag = strdup(""))) {
            goto done;
        }
    }
    if (!chassis->model) {
        if (!(chassis->model = strdup(""))) {
            goto done;
        }
    }
    if (!chassis->product_name) {
        if (!(chassis->product_name = strdup(""))) {
            goto done;
        }
    }
    if (!chassis->uuid) {
        if (!(chassis->uuid = strdup(""))) {
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

short dmi_get_chassis(DmiChassis *chassis)
{
    short ret = -1;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf;

    init_dmi_chassis_struct(chassis);

    /* get dmidecode output for chassis */
    if (run_command("dmidecode -t 3", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* empty output from dmidecode */
    if (buffer_size < 5) {
        lmi_warn("Dmidecode has no information about chassis.");
        goto done;
    }

    /* parse information about chassis */
    for (i = 0; i < buffer_size; i++) {
        /* Serial Number */
        buf = copy_string_part_after_delim(buffer[i], "Serial Number: ");
        if (buf) {
            chassis->serial_number = buf;
            buf = NULL;
            continue;
        }
        /* Asset Tag */
        buf = copy_string_part_after_delim(buffer[i], "Asset Tag: ");
        if (buf) {
            chassis->asset_tag = buf;
            buf = NULL;
            continue;
        }
        /* Type */
        buf = copy_string_part_after_delim(buffer[i], "Type: ");
        if (buf) {
            chassis->type = buf;
            buf = NULL;
            continue;
        }
        /* Manufacturer */
        buf = copy_string_part_after_delim(buffer[i], "Manufacturer: ");
        if (buf) {
            chassis->manufacturer = buf;
            buf = NULL;
            continue;
        }
        /* SKU Number */
        buf = copy_string_part_after_delim(buffer[i], "SKU Number: ");
        if (buf) {
            chassis->sku_number = buf;
            buf = NULL;
            continue;
        }
        /* Version */
        buf = copy_string_part_after_delim(buffer[i], "Version: ");
        if (buf) {
            chassis->version = buf;
            buf = NULL;
            continue;
        }
        /* Has Lock */
        buf = copy_string_part_after_delim(buffer[i], "Lock: ");
        if (buf) {
            if (strcmp(buf, "Present") == 0) {
                chassis->has_lock = 1;
            }
            free(buf);
            buf = NULL;
            continue;
        }
        /* Number of power cords */
        buf = copy_string_part_after_delim(buffer[i], "Number Of Power Cords: ");
        if (buf) {
            if (strcmp(buf, "Unspecified") != 0) {
                sscanf(buf, "%u", &chassis->power_cords);
            }
            free(buf);
            buf = NULL;
            continue;
        }
    }

    free_2d_buffer(&buffer, &buffer_size);

    /* get additional dmidecode output for chassis */
    if (run_command("dmidecode -t 1", &buffer, &buffer_size) == 0
            && buffer_size > 4) {
        for (i = 0; i < buffer_size; i++) {
            /* Model */
            buf = copy_string_part_after_delim(buffer[i], "Version: ");
            if (buf) {
                chassis->model = buf;
                buf = NULL;
                continue;
            }
            /* Product Name */
            buf = copy_string_part_after_delim(buffer[i], "Product Name: ");
            if (buf) {
                chassis->product_name = buf;
                buf = NULL;
                continue;
            }
            /* UUID */
            buf = copy_string_part_after_delim(buffer[i], "UUID: ");
            if (buf) {
                chassis->uuid = buf;
                buf = NULL;
                continue;
            }
        }
    }

    /* fill in default attributes if needed */
    if (check_dmi_chassis_attributes(chassis) != 0) {
        goto done;
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        dmi_free_chassis(chassis);
    }

    return ret;
}

void dmi_free_chassis(DmiChassis *chassis)
{
    if (!chassis) {
        return;
    }

    free(chassis->serial_number);
    chassis->serial_number = NULL;
    free(chassis->type);
    chassis->type = NULL;
    free(chassis->manufacturer);
    chassis->manufacturer = NULL;
    free(chassis->sku_number);
    chassis->sku_number = NULL;
    free(chassis->version);
    chassis->version = NULL;
    free(chassis->asset_tag);
    chassis->asset_tag = NULL;
    free(chassis->model);
    chassis->model = NULL;
    free(chassis->product_name);
    chassis->product_name = NULL;
    free(chassis->uuid);
    chassis->uuid = NULL;
}

char *dmi_get_chassis_tag(DmiChassis *chassis)
{
    char *ret;

    if (strlen(chassis->asset_tag) > 0
            && strcmp(chassis->asset_tag, "Not Specified") != 0) {
        ret = chassis->asset_tag;
    } else if (strlen(chassis->serial_number) > 0
            && strcmp(chassis->serial_number, "Not Specified") != 0) {
        ret = chassis->serial_number;
    } else {
        ret = "0";
    }

    return ret;
}


/******************************************************************************
 * DmiBaseboard
 */

/*
 * Initialize DmiBaseboard attributes.
 * @param baseboard
 */
void init_dmi_baseboard_struct(DmiBaseboard *baseboard)
{
    baseboard->serial_number = NULL;
    baseboard->manufacturer = NULL;
    baseboard->product_name = NULL;
    baseboard->version = NULL;
}

/*
 * Check attributes of baseboard structure and fill in defaults if needed.
 * @param baseboard
 * @return 0 if success, negative value otherwise
 */
short check_dmi_baseboard_attributes(DmiBaseboard *baseboard)
{
    short ret = -1;

    if (!baseboard->serial_number) {
        if (!(baseboard->serial_number = strdup("Not Specified"))) {
            goto done;
        }
    }
    if (!baseboard->manufacturer) {
        if (!(baseboard->manufacturer = strdup(""))) {
            goto done;
        }
    }
    if (!baseboard->product_name) {
        if (!(baseboard->product_name = strdup(""))) {
            goto done;
        }
    }
    if (!baseboard->version) {
        if (!(baseboard->version = strdup(""))) {
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

short dmi_get_baseboard(DmiBaseboard *baseboard)
{
    short ret = -1;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf;

    init_dmi_baseboard_struct(baseboard);

    /* get dmidecode output for baseboard */
    if (run_command("dmidecode -t 2", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* empty output from dmidecode */
    if (buffer_size < 5) {
        lmi_warn("Dmidecode has no information about baseboard.");
        goto done;
    }

    /* parse information about baseboard */
    for (i = 0; i < buffer_size; i++) {
        /* Serial Number */
        buf = copy_string_part_after_delim(buffer[i], "Serial Number: ");
        if (buf) {
            baseboard->serial_number = buf;
            buf = NULL;
            continue;
        }
        /* Manufacturer */
        buf = copy_string_part_after_delim(buffer[i], "Manufacturer: ");
        if (buf) {
            baseboard->manufacturer = buf;
            buf = NULL;
            continue;
        }
        /* Product Name */
        buf = copy_string_part_after_delim(buffer[i], "Product Name: ");
        if (buf) {
            baseboard->product_name = buf;
            buf = NULL;
            continue;
        }
        /* Version */
        buf = copy_string_part_after_delim(buffer[i], "Version: ");
        if (buf) {
            baseboard->version = buf;
            buf = NULL;
            continue;
        }
    }

    /* fill in default attributes if needed */
    if (check_dmi_baseboard_attributes(baseboard) != 0) {
        goto done;
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        dmi_free_baseboard(baseboard);
    }

    return ret;
}

void dmi_free_baseboard(DmiBaseboard *baseboard)
{
    if (!baseboard) {
        return;
    }

    free(baseboard->serial_number);
    baseboard->serial_number = NULL;
    free(baseboard->manufacturer);
    baseboard->manufacturer = NULL;
    free(baseboard->product_name);
    baseboard->product_name = NULL;
    free(baseboard->version);
    baseboard->version = NULL;
}


/******************************************************************************
 * DmiPorts
 */

/*
 * Initialize DmiPort attributes.
 * @param port
 */
void init_dmiports_struct(DmiPort *port)
{
    port->name = NULL;
    port->type = NULL;
    port->port_type = NULL;
}

/*
 * Check attributes of port structure and fill in defaults if needed.
 * @param port
 * @return 0 if success, negative value otherwise
 */
short check_dmiport_attributes(DmiPort *port)
{
    short ret = -1;

    if (!port->name) {
        if (!(port->name = strdup("Port"))) {
            goto done;
        }
    }
    if (!port->type) {
        if (!(port->type = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!port->port_type) {
        if (!(port->port_type = strdup("Unknown"))) {
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

short dmi_get_ports(DmiPort **ports, unsigned *ports_nb)
{
    short ret = -1;
    int curr_port = -1;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf;

    dmi_free_ports(ports, ports_nb);

    /* get dmidecode output */
    if (run_command("dmidecode -t 8", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* count ports */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            (*ports_nb)++;
        }
    }

    /* if no port was found */
    if (*ports_nb < 1) {
        lmi_warn("Dmidecode didn't recognize any port.");
        goto done;
    }

    /* allocate memory for ports */
    *ports = (DmiPort *)calloc(*ports_nb, sizeof(DmiPort));
    if (!(*ports)) {
        lmi_warn("Failed to allocate memory.");
        *ports_nb = 0;
        goto done;
    }

    /* parse information about ports */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            curr_port++;
            init_dmiports_struct(&(*ports)[curr_port]);
            continue;
        }
        /* ignore first useless lines */
        if (curr_port < 0) {
            continue;
        }
        /* Name */
        buf = copy_string_part_after_delim(buffer[i], "External Reference Designator: ");
        if (buf) {
            (*ports)[curr_port].name = buf;
            buf = NULL;
            continue;
        }
        /* Type */
        buf = copy_string_part_after_delim(buffer[i], "External Connector Type: ");
        if (buf) {
            (*ports)[curr_port].type = buf;
            buf = NULL;
            continue;
        }
        /* Port Type */
        buf = copy_string_part_after_delim(buffer[i], "Port Type: ");
        if (buf) {
            (*ports)[curr_port].port_type = buf;
            buf = NULL;
            continue;
        }
    }

    /* fill in default attributes if needed */
    for (i = 0; i < *ports_nb; i++) {
        if (check_dmiport_attributes(&(*ports)[i]) != 0) {
            goto done;
        }
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        dmi_free_ports(ports, ports_nb);
    }

    return ret;
}

void dmi_free_ports(DmiPort **ports, unsigned *ports_nb)
{
    unsigned i;

    if (*ports && *ports_nb > 0) {
        for (i = 0; i < *ports_nb; i++) {
            free((*ports)[i].name);
            (*ports)[i].name = NULL;
            free((*ports)[i].type);
            (*ports)[i].type = NULL;
            free((*ports)[i].port_type);
            (*ports)[i].port_type = NULL;
        }
        free(*ports);
    }

    *ports_nb = 0;
    *ports = NULL;
}


/******************************************************************************
 * DmiSystemSlot
 */

/*
 * Initialize DmiSystemSlot attributes.
 * @param slot
 */
void init_dmislot_struct(DmiSystemSlot *slot)
{
    slot->name = NULL;
    slot->number = 0;
    slot->type = NULL;
    slot->data_width = 0;
    slot->link_width = NULL;
    slot->supports_hotplug = 0;
}

/*
 * Check attributes of slot structure and fill in defaults if needed.
 * @param slot
 * @return 0 if success, negative value otherwise
 */
short check_dmislot_attributes(DmiSystemSlot *slot)
{
    short ret = -1;

    if (!slot->name) {
        if (!(slot->name = strdup("System slot"))) {
            goto done;
        }
    }
    if (!slot->type) {
        if (!(slot->type = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!slot->link_width) {
        if (!(slot->link_width = strdup("Unknown"))) {
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

short dmi_get_system_slots(DmiSystemSlot **slots, unsigned *slots_nb)
{
    short ret = -1;
    int curr_slot = -1;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf = NULL;

    dmi_free_system_slots(slots, slots_nb);

    /* get dmidecode output */
    if (run_command("dmidecode -t 9", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* count slots */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            (*slots_nb)++;
        }
    }

    /* if no slot was found */
    if (*slots_nb < 1) {
        lmi_warn("Dmidecode didn't recognize any system slot.");
        goto done;
    }

    /* allocate memory for slots */
    *slots = (DmiSystemSlot *)calloc(*slots_nb, sizeof(DmiSystemSlot));
    if (!(*slots)) {
        lmi_warn("Failed to allocate memory.");
        *slots_nb = 0;
        goto done;
    }

    /* parse information about slots */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            curr_slot++;
            init_dmislot_struct(&(*slots)[curr_slot]);
            (*slots)[curr_slot].number = curr_slot;
            continue;
        }
        /* ignore first useless lines */
        if (curr_slot < 0) {
            continue;
        }
        /* Name */
        buf = copy_string_part_after_delim(buffer[i], "Designation: ");
        if (buf) {
            (*slots)[curr_slot].name = buf;
            buf = NULL;
            continue;
        }
        /* Type, data width, link width */
        buf = copy_string_part_after_delim(buffer[i], "Type: ");
        if (buf) {
            char **exploded_buf = NULL;
            unsigned j, exploded_buf_size = 0, from = 0;

            /* Dmi type consists of 2 information. First is data width (NUM-bit)
               or link width (xNUM) or nothing, second is slot type. */
            if (explode(buf, NULL, &exploded_buf, &exploded_buf_size) != 0) {
                goto done;
            }
            if (exploded_buf_size < 1) {
                continue;
            }

            if (exploded_buf[0][0] == 'x') {
                (*slots)[curr_slot].link_width = strdup(exploded_buf[0]);
                if (!(*slots)[curr_slot].link_width) {
                    free_2d_buffer(&exploded_buf, &exploded_buf_size);
                    lmi_warn("Failed to allocate memory.");
                    goto done;
                }
                from = 1;
            } else if (strstr(exploded_buf[0], "-bit")) {
                sscanf(buf, "%u-bit ", &(*slots)[curr_slot].data_width);
                from = 1;
            }

            for (j = from; j < exploded_buf_size; j++) {
                /* If type is not NULL, we need to add space before another string. */
                if (!(*slots)[curr_slot].type) {
                    (*slots)[curr_slot].type = append_str(
                            (*slots)[curr_slot].type, exploded_buf[j], NULL);
                } else {
                    (*slots)[curr_slot].type = append_str(
                            (*slots)[curr_slot].type, " ", exploded_buf[j], NULL);
                }
                /* If type is NULL here, append_str failed. */
                if (!(*slots)[curr_slot].type) {
                    free_2d_buffer(&exploded_buf, &exploded_buf_size);
                    goto done;
                }
            }

            free_2d_buffer(&exploded_buf, &exploded_buf_size);
            free(buf);
            buf = NULL;
            continue;
        }
        if (strcmp(buffer[i], "Hot-plug devices are supported") == 0) {
            (*slots)[curr_slot].supports_hotplug = 1;
        }
    }

    /* fill in default attributes if needed */
    for (i = 0; i < *slots_nb; i++) {
        if (check_dmislot_attributes(&(*slots)[i]) != 0) {
            goto done;
        }
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);
    free(buf);

    if (ret != 0) {
        dmi_free_system_slots(slots, slots_nb);
    }

    return ret;
}

void dmi_free_system_slots(DmiSystemSlot **slots, unsigned *slots_nb)
{
    unsigned i;

    if (*slots && *slots_nb > 0) {
        for (i = 0; i < *slots_nb; i++) {
            free((*slots)[i].name);
            (*slots)[i].name = NULL;
            free((*slots)[i].type);
            (*slots)[i].type = NULL;
            free((*slots)[i].link_width);
            (*slots)[i].link_width = NULL;
        }
        free(*slots);
    }

    *slots_nb = 0;
    *slots = NULL;
}


/******************************************************************************
 * DmiPointingDevice
 */

/*
 * Initialize DmiPointingDevice attributes.
 * @param dev
 */
void init_dmipointingdev_struct(DmiPointingDevice *dev)
{
    dev->type = NULL;
    dev->buttons = 0;
}

/*
 * Check attributes of poiting device structure and fill in defaults if needed.
 * @param dev
 * @return 0 if success, negative value otherwise
 */
short check_dmipointingdev_attributes(DmiPointingDevice *dev)
{
    short ret = -1;

    if (!dev->type) {
        if (!(dev->type = strdup("Unknown"))) {
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

short dmi_get_pointing_devices(DmiPointingDevice **devices, unsigned *devices_nb)
{
    short ret = -1;
    int curr_dev = -1;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf;

    dmi_free_pointing_devices(devices, devices_nb);

    /* get dmidecode output */
    if (run_command("dmidecode -t 21", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* count pointing devices */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            (*devices_nb)++;
        }
    }

    /* if no slot was found */
    if (*devices_nb < 1) {
        lmi_warn("Dmidecode didn't recognize any pointing device.");
        goto done;
    }

    /* allocate memory for pointing devices */
    *devices = (DmiPointingDevice *)calloc(*devices_nb, sizeof(DmiPointingDevice));
    if (!(*devices)) {
        lmi_warn("Failed to allocate memory.");
        *devices_nb = 0;
        goto done;
    }

    /* parse information about slots */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            curr_dev++;
            init_dmipointingdev_struct(&(*devices)[curr_dev]);
            continue;
        }
        /* ignore first useless lines */
        if (curr_dev < 0) {
            continue;
        }
        /* Type */
        buf = copy_string_part_after_delim(buffer[i], "Type: ");
        if (buf) {
            (*devices)[curr_dev].type = buf;
            buf = NULL;
            continue;
        }
        /* Buttons */
        buf = copy_string_part_after_delim(buffer[i], "Buttons: ");
        if (buf) {
            sscanf(buf, "%u", &(*devices)[curr_dev].buttons);
            free(buf);
            buf = NULL;
            continue;
        }
    }

    /* fill in default attributes if needed */
    for (i = 0; i < *devices_nb; i++) {
        if (check_dmipointingdev_attributes(&(*devices)[i]) != 0) {
            goto done;
        }
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        dmi_free_pointing_devices(devices, devices_nb);
    }

    return ret;
}

void dmi_free_pointing_devices(DmiPointingDevice **devices, unsigned *devices_nb)
{
    unsigned i;

    if (*devices && *devices_nb > 0) {
        for (i = 0; i < *devices_nb; i++) {
            free((*devices)[i].type);
            (*devices)[i].type = NULL;
        }
        free(*devices);
    }

    *devices_nb = 0;
    *devices = NULL;
}


/******************************************************************************
 * DmiBattery
 */

/*
 * Initialize DmiBattery attributes.
 * @param batt
 */
void init_dmibattery_struct(DmiBattery *batt)
{
    batt->name = NULL;
    batt->chemistry = NULL;
    batt->design_capacity = 0;
    batt->design_voltage = 0;
    batt->manufacturer = NULL;
    batt->serial_number = NULL;
    batt->version = NULL;
    batt->manufacture_date = NULL;
    batt->location = NULL;
}

/*
 * Check attributes of battery structure and fill in defaults if needed.
 * @param batt
 * @return 0 if success, negative value otherwise
 */
short check_dmibattery_attributes(DmiBattery *batt)
{
    short ret = -1;

    if (!batt->name) {
        if (!(batt->name = strdup("Battery"))) {
            goto done;
        }
    }
    if (!batt->chemistry) {
        if (!(batt->chemistry = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!batt->manufacturer) {
        if (!(batt->manufacturer = strdup(""))) {
            goto done;
        }
    }
    if (!batt->serial_number) {
        if (!(batt->serial_number = strdup(""))) {
            goto done;
        }
    }
    if (!batt->version) {
        if (!(batt->version = strdup(""))) {
            goto done;
        }
    }
    if (!batt->manufacture_date) {
        if (!(batt->manufacture_date = strdup(""))) {
            goto done;
        }
    }
    if (!batt->location) {
        if (!(batt->location = strdup(""))) {
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

short dmi_get_batteries(DmiBattery **batteries, unsigned *batteries_nb)
{
    short ret = -1;
    int curr_batt = -1;
    unsigned i, buffer_size = 0;
    char **buffer = NULL, *buf;

    dmi_free_batteries(batteries, batteries_nb);

    /* get dmidecode output */
    if (run_command("dmidecode -t 22", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* count batteries */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            (*batteries_nb)++;
        }
    }

    /* if no battery was found */
    if (*batteries_nb < 1) {
        lmi_warn("Dmidecode didn't recognize any batteries.");
        goto done;
    }

    /* allocate memory for batteries */
    *batteries = (DmiBattery *)calloc(*batteries_nb, sizeof(DmiBattery));
    if (!(*batteries)) {
        lmi_warn("Failed to allocate memory.");
        *batteries_nb = 0;
        goto done;
    }

    /* parse information about batteries */
    for (i = 0; i < buffer_size; i++) {
        if (strncmp(buffer[i], "Handle 0x", 9) == 0) {
            curr_batt++;
            init_dmibattery_struct(&(*batteries)[curr_batt]);
            continue;
        }
        /* ignore first useless lines */
        if (curr_batt < 0) {
            continue;
        }
        /* Name */
        buf = copy_string_part_after_delim(buffer[i], "Name: ");
        if (buf) {
            (*batteries)[curr_batt].name = buf;
            buf = NULL;
            continue;
        }
        /* Chemistry */
        buf = copy_string_part_after_delim(buffer[i], "Chemistry: ");
        if (buf) {
            if (!(*batteries)[curr_batt].chemistry) {
                (*batteries)[curr_batt].chemistry = buf;
            } else {
                free(buf);
            }
            buf = NULL;
            continue;
        }
        /* Design capacity */
        buf = copy_string_part_after_delim(buffer[i], "Design Capacity: ");
        if (buf) {
            sscanf(buf, "%u", &(*batteries)[curr_batt].design_capacity);
            free(buf);
            buf = NULL;
            continue;
        }
        /* Design voltage */
        buf = copy_string_part_after_delim(buffer[i], "Design Voltage: ");
        if (buf) {
            sscanf(buf, "%u", &(*batteries)[curr_batt].design_voltage);
            free(buf);
            buf = NULL;
            continue;
        }
        /* Manufacturer */
        buf = copy_string_part_after_delim(buffer[i], "Manufacturer: ");
        if (buf) {
            (*batteries)[curr_batt].manufacturer = buf;
            buf = NULL;
            continue;
        }
        /* Serial number */
        buf = copy_string_part_after_delim(buffer[i], "Serial Number: ");
        if (buf) {
            if (!(*batteries)[curr_batt].serial_number) {
                (*batteries)[curr_batt].serial_number = buf;
            } else {
                free(buf);
            }
            buf = NULL;
            continue;
        }
        /* Version */
        buf = copy_string_part_after_delim(buffer[i], "Version: ");
        if (buf) {
            (*batteries)[curr_batt].version = buf;
            buf = NULL;
            continue;
        }
        /* Manufacture date */
        buf = copy_string_part_after_delim(buffer[i], "Manufacture Date: ");
        if (buf) {
            if (!(*batteries)[curr_batt].manufacture_date) {
                (*batteries)[curr_batt].manufacture_date = buf;
            } else {
                free(buf);
            }
            buf = NULL;
            continue;
        }
        /* Location */
        buf = copy_string_part_after_delim(buffer[i], "Location: ");
        if (buf) {
            (*batteries)[curr_batt].location = buf;
            buf = NULL;
            continue;
        }
    }

    /* fill in default attributes if needed */
    for (i = 0; i < *batteries_nb; i++) {
        if (check_dmibattery_attributes(&(*batteries)[i]) != 0) {
            goto done;
        }
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        dmi_free_batteries(batteries, batteries_nb);
    }

    return ret;
}

void dmi_free_batteries(DmiBattery **batteries, unsigned *batteries_nb)
{
    unsigned i;

    if (*batteries && *batteries_nb > 0) {
        for (i = 0; i < *batteries_nb; i++) {
            free((*batteries)[i].name);
            (*batteries)[i].name = NULL;
            free((*batteries)[i].chemistry);
            (*batteries)[i].chemistry = NULL;
            free((*batteries)[i].manufacturer);
            (*batteries)[i].manufacturer = NULL;
            free((*batteries)[i].serial_number);
            (*batteries)[i].serial_number = NULL;
            free((*batteries)[i].version);
            (*batteries)[i].version = NULL;
            free((*batteries)[i].manufacture_date);
            (*batteries)[i].manufacture_date = NULL;
            free((*batteries)[i].location);
            (*batteries)[i].location = NULL;
        }
        free(*batteries);
    }

    *batteries_nb = 0;
    *batteries = NULL;
}
