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

#include "sysfs.h"


/*
 * Read unsigned value from file.
 * @param path of file
 * @paratm result
 * @return 0 if success, negative value otherwise
 */
short path_get_unsigned(const char *path, unsigned *result)
{
    short ret = -1;
    unsigned buffer_size = 0;
    char **buffer = NULL;

    if (read_file(path, &buffer, &buffer_size) != 0 || buffer_size < 1) {
        goto done;
    }
    if (sscanf(buffer[0], "%u", result) != 1) {
        warn("Failed to parse file: \"%s\"; Error: %s",
                path, strerror(errno));
        goto done;
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        *result = 0;
    }

    return ret;
}

/*
 * Read string value from file.
 * @param path of file
 * @paratm result
 * @return 0 if success, negative value otherwise
 */
short path_get_string(const char *path, char **result)
{
    short ret = -1;
    unsigned buffer_size = 0;
    char **buffer = NULL;

    if (read_file(path, &buffer, &buffer_size) != 0 || buffer_size < 1) {
        goto done;
    }
    *result = trim(buffer[0], NULL);
    if (!(*result)) {
        warn("Failed to parse file: \"%s\"", path);
        goto done;
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        *result = NULL;
    }

    return ret;
}

/*
 * Initialize SysfsCpuCache attributes.
 * @param cache
 */
void init_sysfs_cpu_cache_struct(SysfsCpuCache *cache)
{
    cache->id = NULL;
    cache->size = 0;
    cache->name = NULL;
    cache->level = 0;
    cache->type = NULL;
    cache->ways_of_assoc = 0;
    cache->line_size = 0;
}

/*
 * Check attributes of cache structure and fill in defaults if needed.
 * @param cache
 * @return 0 if success, negative value otherwise
 */
short check_sysfs_cpu_cache_attributes(SysfsCpuCache *cache)
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
    if (!cache->type) {
        if (!(cache->type = strdup("Unknown"))) {
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

/*
 * Deep copy SysfsCpuCache structure.
 * @param to destination
 * @param from source
 * @return 0 if success, negative value otherwise
 */
short copy_sysfs_cpu_cache(SysfsCpuCache *to, const SysfsCpuCache from)
{
    *to = from;
    to->id = strdup(from.id);
    to->name = strdup(from.name);
    to->type = strdup(from.type);

    if (!to->id || !to->name || !to->type) {
        warn("Failed to allocate memory.");
        free(to->id);
        to->id = NULL;
        free(to->name);
        to->name = NULL;
        free(to->type);
        to->type = NULL;
        return -1;
    }

    return 0;
}

short sysfs_get_cpu_caches(SysfsCpuCache **caches, unsigned *caches_nb)
{
    short ret = -1;
    unsigned i, level;
    char *buf = NULL, *format_str, path[PATH_MAX];
    DmiProcessor *dmi_cpus = NULL;
    unsigned dmi_cpus_nb = 0, cpus_nb = 0;
    LscpuProcessor lscpu;

    *caches_nb = 0;

    /* get processor information */
    if (dmi_get_processors(&dmi_cpus, &dmi_cpus_nb) != 0 || dmi_cpus_nb < 1) {
        dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);

        if (lscpu_get_processor(&lscpu) != 0) {
            goto done;
        }
    }
    if (dmi_cpus_nb > 0) {
        cpus_nb = dmi_cpus_nb;
    } else if (lscpu.processors > 0) {
        cpus_nb = lscpu.processors;
    } else {
        warn("No processor found.");
        goto done;
    }

    /* count caches */
    DIR *dir;
    char *cache_dir = SYSFS_CPU_PATH "/cpu0/cache";
    dir = opendir(cache_dir);
    if (!dir) {
        warn("Failed to read directory: \"%s\"; Error: %s",
                cache_dir, strerror(errno));
        goto done;
    }
    while (readdir(dir)) {
        (*caches_nb)++;
    }
    closedir(dir);

    /* do not count . and .. */
    *caches_nb -= 2;

    /* if no cache was found */
    if (*caches_nb < 1) {
        warn("No processor cache was found in sysfs.");
        goto done;
    }

    /* allocate memory for caches */
    *caches = (SysfsCpuCache *)calloc(*caches_nb * cpus_nb, sizeof(SysfsCpuCache));
    if (!(*caches)) {
        warn("Failed to allocate memory.");
        *caches_nb = 0;
        goto done;
    }

    for (i = 0; i < *caches_nb; i++) {
        init_sysfs_cpu_cache_struct(&(*caches)[i]);

        /* cache ID and name */
        /* cache level */
        snprintf(path, PATH_MAX, SYSFS_CPU_PATH "/cpu0/cache/index%u/level", i);
        if (path_get_unsigned(path, &level) != 0) {
            goto done;
        }
        (*caches)[i].level = level;
        /* cache type */
        snprintf(path, PATH_MAX, SYSFS_CPU_PATH "/cpu0/cache/index%u/type", i);
        if (path_get_string(path, &buf) != 0) {
            goto done;
        }
        if (strncmp(buf, "Data", 4) == 0) {
            format_str = "L%ud";
        } else if (strncmp(buf, "Instruction", 11) == 0) {
            format_str = "L%ui";
        } else {
            format_str = "L%u";
        }
        if (asprintf(&(*caches)[i].id, format_str, level) < 0) {
            (*caches)[i].id = NULL;
            warn("Failed to allocate memory.");
            goto done;
        }
        if (asprintf(&(*caches)[i].name, "Level %u %s cache",
                level, buf) < 0) {
            (*caches)[i].name = NULL;
            warn("Failed to allocate memory.");
            goto done;
        }
        (*caches)[i].type = buf;
        buf = NULL;

        /* cache size */
        snprintf(path, PATH_MAX, SYSFS_CPU_PATH "/cpu0/cache/index%u/size", i);
        if (path_get_unsigned(path, &(*caches)[i].size) != 0) {
            (*caches)[i].size = 0;
        }
        (*caches)[i].size *= 1024;      /* It's in kB, we want B */

        /* ways of associativity */
        snprintf(path, PATH_MAX,
                SYSFS_CPU_PATH "/cpu0/cache/index%u/ways_of_associativity", i);
        if (path_get_unsigned(path, &(*caches)[i].ways_of_assoc) != 0) {
            (*caches)[i].ways_of_assoc = 0;
        }

        /* line size */
        snprintf(path, PATH_MAX,
                SYSFS_CPU_PATH "/cpu0/cache/index%u/coherency_line_size", i);
        if (path_get_unsigned(path, &(*caches)[i].line_size) != 0) {
            (*caches)[i].line_size = 0;
        }

        /* fill in default attributes if needed */
        if (check_sysfs_cpu_cache_attributes(&(*caches)[i]) != 0) {
            goto done;
        }
    }

    /* duplicate all caches for every processor;
       this assumes that all CPUs in the system are the same */
    for (i = *caches_nb; i < *caches_nb * cpus_nb; i++) {
        copy_sysfs_cpu_cache(&(*caches)[i], (*caches)[i % *caches_nb]);
    }
    *caches_nb *= cpus_nb;

    /* and give them unique ID */
    char cache_id[LONG_INT_LEN];
    for (i = 0; i < *caches_nb; i++) {
        snprintf(cache_id, LONG_INT_LEN, "-%u", i);
        (*caches)[i].id = append_str((*caches)[i].id, cache_id, NULL);
        if (!(*caches)[i].id) {
            goto done;
        }
    }

    ret = 0;

done:
    free(buf);

    /* free lscpu only if it was used */
    if (dmi_cpus_nb < 1) {
        lscpu_free_processor(&lscpu);
    }
    dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);

    if (ret != 0) {
        sysfs_free_cpu_caches(caches, caches_nb);
    }

    return ret;
}

void sysfs_free_cpu_caches(SysfsCpuCache **caches, unsigned *caches_nb)
{
    unsigned i;

    if (*caches && *caches_nb > 0) {
        for (i = 0; i < *caches_nb; i++) {
            free((*caches)[i].id);
            (*caches)[i].id = NULL;
            free((*caches)[i].name);
            (*caches)[i].name = NULL;
            free((*caches)[i].type);
            (*caches)[i].type = NULL;
        }
        free (*caches);
    }

    *caches_nb = 0;
    *caches = NULL;
}

short sysfs_has_numa()
{
    struct stat st;

    if (stat(SYSFS_PATH "/node/node0", &st) == 0) {
        return 1;
    }

    return 0;
}

short sysfs_get_sizes_of_hugepages(unsigned **sizes, unsigned *sizes_nb)
{
    short ret = -1;
    DIR *dir = NULL;

    *sizes_nb = 0;
    *sizes = NULL;

    /* count all sizes */
    char *sizes_dir = SYSFS_KERNEL_MM "/hugepages";
    dir = opendir(sizes_dir);
    if (!dir) {
        warn("Failed to read directory: \"%s\"; Error: %s",
                sizes_dir, strerror(errno));
        goto done;
    }
    while (readdir(dir)) {
        (*sizes_nb)++;
    }

    /* do not count . and .. */
    *sizes_nb -= 2;

    /* if no size was found */
    if (*sizes_nb < 1) {
        warn("Looks like kernel doesn't support huge memory pages.");
        goto done;
    }

    /* allocate memory for sizes */
    *sizes = (unsigned *)calloc(*sizes_nb, sizeof(unsigned));
    if (!(*sizes)) {
        warn("Failed to allocate memory.");
        *sizes_nb = 0;
        goto done;
    }

    /* get sizes */
    struct dirent *d;
    unsigned i = 0;
    rewinddir(dir);
    while ((d = readdir(dir)) && i < *sizes_nb) {
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) {
            continue;
        }
        if (strlen(d->d_name) > 11) {
            /* read page size from the dirname, which looks like: hugepages-2048kB */
            if (sscanf(d->d_name + 10, "%u", &(*sizes)[i]) == 1) {
                i++;
            }
        }
    }

    ret = 0;

done:
    if (dir) {
        closedir(dir);
    }

    if (ret != 0) {
        *sizes_nb = 0;
        free(*sizes);
        *sizes = NULL;
    }

    return ret;
}

ThpStatus sysfs_get_transparent_hugepages_status()
{
    ThpStatus ret = thp_unsupported;
    char *val = NULL;

    if (path_get_string(SYSFS_KERNEL_MM "/transparent_hugepage/enabled", &val) != 0) {
        goto done;
    }

    if (strstr(val, "[always]")) {
        ret = thp_always;
    } else if (strstr(val, "[madvise]")) {
        ret = thp_madvise;
    } else if (strstr(val, "[never]")) {
        ret = thp_never;
    }

done:
    free(val);

    return ret;
}
