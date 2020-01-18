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

#include "lsblk.h"

/*
 * Initialize LsblkHdd attributes.
 * @param hdd
 */
void init_lsblkhdd_struct(LsblkHdd *hdd)
{
    hdd->name = NULL;
    hdd->basename = NULL;
    hdd->type = NULL;
    hdd->model = NULL;
    hdd->serial = NULL;
    hdd->revision = NULL;
    hdd->vendor = NULL;
    hdd->tran = NULL;
    hdd->capacity = 0;
}

/*
 * Check attributes of hdd structure and fill in defaults if needed.
 * @param hdd
 * @return 0 if success, negative value otherwise
 */
short check_lsblkhdd_attributes(LsblkHdd *hdd)
{
    short ret = -1;

    if (!hdd->name) {
        if (!(hdd->name = strdup(""))) {
            goto done;
        }
    }
    if (!hdd->basename) {
        if (!(hdd->basename = strdup(""))) {
            goto done;
        }
    }
    if (!hdd->type) {
        if (!(hdd->type = strdup(""))) {
            goto done;
        }
    }
    if (!hdd->model) {
        if (!(hdd->model = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!hdd->serial) {
        if (!(hdd->serial = strdup(""))) {
            goto done;
        }
    }
    if (!hdd->revision) {
        if (!(hdd->revision = strdup(""))) {
            goto done;
        }
    }
    if (!hdd->vendor) {
        if (!(hdd->vendor = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!hdd->tran) {
        if (!(hdd->tran = strdup(""))) {
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

short lsblk_get_hdds(LsblkHdd **hdds, unsigned *hdds_nb)
{
    short ret = -1;
    unsigned i, curr_hdd = 0, buffer_size = 0;
    char **buffer = NULL, *path, *type, *buf, errbuf[BUFLEN];
    struct stat sb;
    unsigned long capacity = 0;

    lsblk_free_hdds(hdds, hdds_nb);

    /* get lsblk output */
    if (run_command("lsblk -dPpo NAME,TYPE,MODEL,SIZE,SERIAL,REV,VENDOR,TRAN",
            &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* count hard drives - one line of output is one device */
    *hdds_nb = buffer_size;

    /* if no hard drive was found */
    if (*hdds_nb < 1) {
        lmi_warn("Lsblk didn't recognize any hard drive.");
        goto done;
    }

    /* allocate memory for hard drives */
    *hdds = (LsblkHdd *)calloc(*hdds_nb, sizeof(LsblkHdd));
    if (!(*hdds)) {
        lmi_warn("Failed to allocate memory.");
        *hdds_nb = 0;
        goto done;
    }

    /* parse information about hard drives */
    for (i = 0; i < buffer_size; i++) {
        path = get_part_of_string_between(buffer[i], "NAME=\"", "\"");
        if (!path) {
            continue;
        }
        if (stat(path, &sb) != 0) {
            lmi_warn("Stat() call on file \"%s\" failed: %s",
                    path, strerror_r(errno, errbuf, sizeof(errbuf)));
            free(path);
            continue;
         }
        if ((sb.st_mode & S_IFMT) != S_IFBLK) {
            lmi_warn("File \"%s\" is not a block device.", path);
            free(path);
            continue;
        }

        type = get_part_of_string_between(buffer[i], "TYPE=\"", "\"");
        if (!type) {
            free(path);
            continue;
        }

        /* size looks like: SIZE="465.8G" */
        buf = get_part_of_string_between(buffer[i], "SIZE=\"", "\"");
        if (buf) {
            float size = 0;
            sscanf(buf, "%f", &size);
            if (size) {
                const char *letters = "BKMGTPE";
                char unit = buf[strlen(buf) - 1];
                unsigned long multiplier = 0;
                for (int i = 0; letters[i]; i++) {
                    if (unit == letters[i]) {
                        multiplier = powl(2, 10 * i);
                        break;
                    }
                }
                capacity = round(size * multiplier);
            }
            free(buf);
            buf = NULL;
        }

        init_lsblkhdd_struct(&(*hdds)[curr_hdd]);

        (*hdds)[curr_hdd].name = path;
        (*hdds)[curr_hdd].type = type;
        (*hdds)[curr_hdd].capacity = capacity;
        (*hdds)[curr_hdd].model = get_part_of_string_between(buffer[i], "MODEL=\"", "\"");
        (*hdds)[curr_hdd].serial = get_part_of_string_between(buffer[i], "SERIAL=\"", "\"");
        (*hdds)[curr_hdd].revision = get_part_of_string_between(buffer[i], "REV=\"", "\"");
        (*hdds)[curr_hdd].vendor = get_part_of_string_between(buffer[i], "VENDOR=\"", "\"");
        (*hdds)[curr_hdd].tran = get_part_of_string_between(buffer[i], "TRAN=\"", "\"");

        (*hdds)[curr_hdd].basename = strdup(basename(path));
        if (!(*hdds)[curr_hdd].basename) {
            lmi_warn("Failed to allocate memory.");
            goto done;
        }

        curr_hdd++;
    }

    if (curr_hdd != *hdds_nb) {
        lmi_warn("Not all reported drives by lsblk were processed.");
        LsblkHdd *tmp_hdd = (LsblkHdd *)realloc(*hdds,
                curr_hdd * sizeof(LsblkHdd));
        if (!tmp_hdd) {
            lmi_warn("Failed to allocate memory.");
            goto done;
        }
        *hdds = tmp_hdd;
        *hdds_nb = curr_hdd;
    }

    /* fill in default attributes if needed */
    for (i = 0; i < *hdds_nb; i++) {
        if (check_lsblkhdd_attributes(&(*hdds)[i]) != 0) {
            goto done;
        }
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        lsblk_free_hdds(hdds, hdds_nb);
    }

    return ret;
}

void lsblk_free_hdds(LsblkHdd **hdds, unsigned *hdds_nb)
{
    unsigned i;

    if (*hdds && *hdds_nb > 0) {
        for (i = 0; i < *hdds_nb; i++) {
            free((*hdds)[i].name);
            (*hdds)[i].name = NULL;
            free((*hdds)[i].basename);
            (*hdds)[i].basename = NULL;
            free((*hdds)[i].type);
            (*hdds)[i].type = NULL;
            free((*hdds)[i].model);
            (*hdds)[i].model = NULL;
            free((*hdds)[i].serial);
            (*hdds)[i].serial = NULL;
            free((*hdds)[i].revision);
            (*hdds)[i].revision = NULL;
            free((*hdds)[i].vendor);
            (*hdds)[i].vendor = NULL;
            free((*hdds)[i].tran);
            (*hdds)[i].tran = NULL;
        }
        free(*hdds);
    }

    *hdds_nb = 0;
    *hdds = NULL;
}
