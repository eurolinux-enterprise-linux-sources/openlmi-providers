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

#include "smartctl.h"

/*
 * Initialize SmartctlHdd attributes.
 * @param hdd
 */
void init_smctlhdd_struct(SmartctlHdd *hdd)
{
    hdd->dev_path = NULL;
    hdd->dev_basename = NULL;
    hdd->manufacturer = NULL;
    hdd->model = NULL;
    hdd->serial_number = NULL;
    hdd->name = NULL;
    hdd->smart_status = NULL;
    hdd->firmware = NULL;
    hdd->port_type = NULL;
    hdd->form_factor = 0;
    hdd->port_speed = 0;
    hdd->max_port_speed = 0;
    hdd->rpm = 0xffffffff;
    hdd->capacity = 0;
    hdd->curr_temp = SHRT_MIN;
}

/*
 * Check attributes of hdd structure and fill in defaults if needed.
 * @param hdd
 * @return 0 if success, negative value otherwise
 */
short check_smctlhdd_attributes(SmartctlHdd *hdd)
{
    short ret = -1;

    if (!hdd->dev_path) {
        if (!(hdd->dev_path = strdup(""))) {
            goto done;
        }
    }
    if (!hdd->dev_basename) {
        if (!(hdd->dev_basename = strdup(""))) {
            goto done;
        }
    }
    if (!hdd->manufacturer) {
        if (!(hdd->manufacturer = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!hdd->model) {
        if (!(hdd->model = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!hdd->serial_number) {
        if (!(hdd->serial_number = strdup(""))) {
            goto done;
        }
    }
    if (!hdd->name) {
        if (!(hdd->name = strdup(""))) {
            goto done;
        }
    }
    if (!hdd->smart_status) {
        if (!(hdd->smart_status = strdup("UNKNOWN!"))) {
            goto done;
        }
    }
    if (!hdd->firmware) {
        if (!(hdd->firmware = strdup("Unknown"))) {
            goto done;
        }
    }
    if (!hdd->port_type) {
        if (!(hdd->port_type = strdup("Unknown"))) {
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

/*
 * Scan available hdd devices in system with "smartctl --scan" command and
 * create array of SmartctlHdd structures.
 * @param hdds array of hdds
 * @param hdds_nb number of hard drives in hdds
 * @return 0 if success, negative value otherwise
 */
short scan_smctlhdd_devices(SmartctlHdd **hdds, unsigned *hdds_nb)
{
    short ret = -1;
    unsigned i, curr_hdd = 0, buffer_size = 0, sec_buffer_size = 0;
    char **buffer = NULL, **sec_buffer = NULL;
    struct stat sb;
    char errbuf[BUFLEN];

    smartctl_free_hdds(hdds, hdds_nb);

    /* get smartctl output */
    if (run_command("smartctl --scan", &buffer, &buffer_size) != 0) {
        goto done;
    }

    /* count hard drives - one line of output is one device */
    *hdds_nb = buffer_size;

    /* if no hard drive was found */
    if (*hdds_nb < 1) {
        lmi_warn("Smartctl didn't recognize any hard drive.");
        goto done;
    }

    /* allocate memory for hard drives */
    *hdds = (SmartctlHdd *)calloc(*hdds_nb, sizeof(SmartctlHdd));
    if (!(*hdds)) {
        lmi_warn("Failed to allocate memory.");
        *hdds_nb = 0;
        goto done;
    }

    /* parse hard drive's dev path */
    for (i = 0; i < buffer_size; i++) {
        if (explode(buffer[i], NULL, &sec_buffer, &sec_buffer_size) != 0 ||
                sec_buffer_size < 1) {
            free_2d_buffer(&sec_buffer, &sec_buffer_size);
            continue;
        }

        if (stat(sec_buffer[0], &sb) != 0) {
            lmi_warn("Stat() call on file \"%s\" failed: %s",
                    sec_buffer[0], strerror_r(errno, errbuf, sizeof(errbuf)));
            free_2d_buffer(&sec_buffer, &sec_buffer_size);
            continue;
         }

        if ((sb.st_mode & S_IFMT) != S_IFBLK) {
            lmi_warn("File \"%s\" is not a block device.", sec_buffer[0]);
            free_2d_buffer(&sec_buffer, &sec_buffer_size);
            continue;
        }

        init_smctlhdd_struct(&(*hdds)[curr_hdd]);

        (*hdds)[curr_hdd].dev_path = strdup(sec_buffer[0]);
        (*hdds)[curr_hdd].dev_basename = strdup(basename(sec_buffer[0]));
        if (!(*hdds)[curr_hdd].dev_path || !(*hdds)[curr_hdd].dev_basename) {
            lmi_warn("Failed to allocate memory.");
            free_2d_buffer(&sec_buffer, &sec_buffer_size);
            continue;
        }

        curr_hdd++;
        free_2d_buffer(&sec_buffer, &sec_buffer_size);
    }

    if (curr_hdd != *hdds_nb) {
        lmi_warn("There's some \"smartctl --scan\" output mismatch, "
                "not all reported drives were processed.");
        SmartctlHdd *tmp_hdd = (SmartctlHdd *)realloc(*hdds,
                curr_hdd * sizeof(SmartctlHdd));
        if (!tmp_hdd) {
            lmi_warn("Failed to allocate memory.");
            goto done;
        }
        *hdds = tmp_hdd;
        *hdds_nb = curr_hdd;
    }

    ret = 0;

done:
    free_2d_buffer(&buffer, &buffer_size);

    if (ret != 0) {
        smartctl_free_hdds(hdds, hdds_nb);
    }

    return ret;
}

short smartctl_get_hdds(SmartctlHdd **hdds, unsigned *hdds_nb)
{
    short ret = -1;
    unsigned i, curr_hdd, buffer_size = 0;
    char **buffer = NULL, *buf;

    if (scan_smctlhdd_devices(hdds, hdds_nb) != 0) {
        goto done;
    }

    for (curr_hdd = 0; curr_hdd < *hdds_nb; curr_hdd++) {
        /* get smartctl output */
        char command[PATH_MAX];
        snprintf(command, PATH_MAX, "smartctl -iH -l scttempsts --identify=n %s",
                (*hdds)[curr_hdd].dev_path);
        if (run_command(command, &buffer, &buffer_size) < 0) {
            continue;
        }

        /* parse information about hard drives */
        for (i = 0; i < buffer_size; i++) {
            /* Model */
            buf = copy_string_part_after_delim(buffer[i], "Device Model:");
            if (buf) {
                free((*hdds)[curr_hdd].model);
                (*hdds)[curr_hdd].model = buf;
                buf = NULL;
                continue;
            }
            /* Model, second version */
            buf = copy_string_part_after_delim(buffer[i], "Product:");
            if (buf) {
                if (!(*hdds)[curr_hdd].model) {
                    (*hdds)[curr_hdd].model = buf;
                } else {
                    free(buf);
                }
                buf = NULL;
                continue;
            }
            /* Serial Number */
            buf = copy_string_part_after_delim(buffer[i], "Serial Number:");
            if (buf) {
                (*hdds)[curr_hdd].serial_number = buf;
                buf = NULL;
                continue;
            }
            /* Name */
            buf = copy_string_part_after_delim(buffer[i], "Model Family:");
            if (buf) {
                (*hdds)[curr_hdd].name = buf;
                buf = NULL;
                continue;
            }
            /* SMART status */
            buf = copy_string_part_after_delim(buffer[i],
                    "SMART overall-health self-assessment test result: ");
            if (buf) {
                (*hdds)[curr_hdd].smart_status = buf;
                buf = NULL;
                continue;
            }
            /* Firmware version */
            buf = copy_string_part_after_delim(buffer[i], "Firmware Version:");
            if (buf) {
                free((*hdds)[curr_hdd].firmware);
                (*hdds)[curr_hdd].firmware = buf;
                buf = NULL;
                continue;
            }
            /* Firmware version, second version */
            buf = copy_string_part_after_delim(buffer[i], "Revision:");
            if (buf) {
                if (!(*hdds)[curr_hdd].firmware) {
                    (*hdds)[curr_hdd].firmware = buf;
                } else {
                    free(buf);
                }
                buf = NULL;
                continue;
            }
            /* Form Factor */
            /* Form Factor is stored in the 168. word
             * of the IDENTIFY DEVICE data table. Line with this info looks like:
             * 168      0x0003   Form factor */
            buf = copy_string_part_after_delim(buffer[i], "168");
            if (buf) {
                sscanf(buf, "%hx", &(*hdds)[curr_hdd].form_factor);
                free(buf);
                buf = NULL;
                continue;
            }
            /* Port Type, Max and Current Speed */
            buf = copy_string_part_after_delim(buffer[i], "SATA Version is:");
            if (buf) {
                if ((*hdds)[curr_hdd].dev_basename[0] == 's') {
                    if (strncmp(buf, "SATA 2", 6) == 0) {
                        (*hdds)[curr_hdd].port_type = strdup("SATA 2");
                    } else if (strncmp(buf, "SATA", 4) == 0) {
                        (*hdds)[curr_hdd].port_type = strdup("SATA");
                    }
                } else if ((*hdds)[curr_hdd].dev_basename[0] == 'h') {
                    (*hdds)[curr_hdd].port_type = strdup("ATA");
                }

                char *buf2 = strchr(buf, ',');
                if (buf2 && strlen(buf2) > 20) {
                    float max_speed, curr_speed;
                    /* skip ", " */
                    buf2 += 2;
                    sscanf(buf2, "%f %*s %*s %f", &max_speed, &curr_speed);
                    /* Gb/s -> b/s */
                    (*hdds)[curr_hdd].port_speed = round(curr_speed * 1000000000);
                    (*hdds)[curr_hdd].max_port_speed = round(max_speed * 1000000000);
                }

                free(buf);
                buf = NULL;
                continue;
            }
            /* RPM */
            buf = copy_string_part_after_delim(buffer[i], "Rotation Rate:");
            if (buf) {
                sscanf(buf, "%u", &(*hdds)[curr_hdd].rpm);
                free(buf);
                buf = NULL;
                continue;
            }
            /* Capacity */
            buf = copy_string_part_after_delim(buffer[i], "User Capacity:");
            if (buf) {
                /* capacity is in bytes and uses thousand separator -
                 * it must be cleaned */
                char *buf2 = (char *)calloc(strlen(buf), sizeof(char));
                if (buf2) {
                    int k = 0;
                    for (int j = 0; buf[j]; j++) {
                        if (buf[j] >= '0' && buf[j] <= '9') {
                            buf2[k++] = buf[j];
                        } else if (buf[j] == 'b') {
                            /* word after capacity is "bytes" */
                            break;
                        }
                    }
                    buf2[k] = '\0';
                    sscanf(buf2, "%lu", &(*hdds)[curr_hdd].capacity);
                    free(buf2);
                } else {
                    lmi_warn("Failed to allocate memory.");
                }
                free(buf);
                buf = NULL;
                continue;
            }
            /* Temperature */
            buf = copy_string_part_after_delim(buffer[i], "Current Temperature:");
            if (buf) {
                sscanf(buf, "%hd", &(*hdds)[curr_hdd].curr_temp);
                free(buf);
                buf = NULL;
                continue;
            }
        }

        free_2d_buffer(&buffer, &buffer_size);

        /* get vendor from sysfs */
        char sysfs_path[PATH_MAX];
        snprintf(sysfs_path, PATH_MAX, SYSFS_BLOCK_PATH "/%s/device/vendor",
                (*hdds)[curr_hdd].dev_basename);
        path_get_string(sysfs_path, &(*hdds)[curr_hdd].manufacturer);

        /* fill in default attributes if needed */
        if (check_smctlhdd_attributes(&(*hdds)[curr_hdd]) != 0) {
            goto done;
        }
    }

    ret = 0;

done:
    if (ret != 0) {
        smartctl_free_hdds(hdds, hdds_nb);
    }

    return ret;
}

void smartctl_free_hdds(SmartctlHdd **hdds, unsigned *hdds_nb)
{
    unsigned i;

    if (*hdds && *hdds_nb > 0) {
        for (i = 0; i < *hdds_nb; i++) {
            free((*hdds)[i].dev_path);
            (*hdds)[i].dev_path = NULL;
            free((*hdds)[i].dev_basename);
            (*hdds)[i].dev_basename = NULL;
            free((*hdds)[i].manufacturer);
            (*hdds)[i].manufacturer = NULL;
            free((*hdds)[i].model);
            (*hdds)[i].model = NULL;
            free((*hdds)[i].serial_number);
            (*hdds)[i].serial_number = NULL;
            free((*hdds)[i].name);
            (*hdds)[i].name = NULL;
            free((*hdds)[i].smart_status);
            (*hdds)[i].smart_status = NULL;
            free((*hdds)[i].firmware);
            (*hdds)[i].firmware = NULL;
            free((*hdds)[i].port_type);
            (*hdds)[i].port_type = NULL;
        }
        free(*hdds);
    }

    *hdds_nb = 0;
    *hdds = NULL;
}
