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

#include "utils.h"

const ConfigEntry *provider_config_defaults = NULL;
const char *provider_name = "hardware";

short read_fp_to_2d_buffer(FILE *fp, char ***buffer, unsigned *buffer_size)
{
    short ret = -1;
    ssize_t read;
    size_t line_len = 0;
    unsigned tmp_buffer_lines = 0, lines_read = 0;
    char **tmp_buffer = NULL, *line = NULL;

    free_2d_buffer(buffer, buffer_size);

    if (!fp) {
        warn("Given file pointer is NULL.");
        goto done;
    }

    /* allocate buffer */
    tmp_buffer_lines = 128;
    tmp_buffer = (char **)calloc(tmp_buffer_lines, sizeof(char *));
    if (!tmp_buffer) {
        warn("Failed to allocate memory.");
        tmp_buffer_lines = 0;
        goto done;
    }

    while ((read = getline(&line, &line_len, fp)) != -1) {
        /* filter comment lines */
        if (read > 0 && line[0] == '#') {
            continue;
        }

        /* reallocate if needed */
        if (lines_read >= tmp_buffer_lines) {
            tmp_buffer_lines *= 2;
            char **newtmp = (char **)realloc(tmp_buffer,
                    tmp_buffer_lines * sizeof(char *));
            if (!newtmp) {
                warn("Failed to allocate memory.");
                tmp_buffer_lines /= 2;
                goto done;
            }
            tmp_buffer = newtmp;
        }

        /* copy trimmed line to buffer */
        tmp_buffer[lines_read] = trim(line, NULL);
        if (!tmp_buffer[lines_read]) {
            tmp_buffer[lines_read] = strdup("");
            if (!tmp_buffer[lines_read]) {
                warn("Failed to allocate memory.");
                goto done;
            }
        }
        lines_read++;
    }

    if (lines_read < 1) {
        warn("No data read from given source.");
        goto done;
    }

    /* reallocate buffer to free unused space */
    if (tmp_buffer_lines > lines_read) {
        char **newtmp = (char **)realloc(tmp_buffer,
                lines_read * sizeof(char *));
        if (!newtmp) {
            warn("Failed to allocate memory.");
            goto done;
        }
        tmp_buffer = newtmp;
        tmp_buffer_lines = lines_read;
    }

    *buffer_size = tmp_buffer_lines;
    *buffer = tmp_buffer;

    ret = 0;

done:
    free(line);

    if (ret != 0) {
        free_2d_buffer(&tmp_buffer, &tmp_buffer_lines);
    }

    return ret;
}

void free_2d_buffer(char ***buffer, unsigned *buffer_size)
{
    unsigned i, tmp_buffer_lines = *buffer_size;
    char **tmp_buffer = *buffer;

    if (tmp_buffer && tmp_buffer_lines > 0) {
        for (i = 0; i < tmp_buffer_lines; i++) {
            free(tmp_buffer[i]);
            tmp_buffer[i] = NULL;
        }
        free(tmp_buffer);
    }

    tmp_buffer = NULL;
    *buffer_size = 0;
    *buffer = NULL;
}

short run_command(const char *command, char ***buffer, unsigned *buffer_size)
{
    FILE *fp = NULL;
    short ret = -1;

    /* if command is empty */
    if (!command || strlen(command) < 1) {
        warn("Given command is empty.");
        goto done;
    }

    /* execute command */
    debug("Running command: \"%s\"", command);
    fp = popen(command, "r");
    if (!fp) {
        warn("Failed to run command: \"%s\"; Error: %s",
                command, strerror(errno));
        goto done;
    }

    if (read_fp_to_2d_buffer(fp, buffer, buffer_size) != 0) {
        goto done;
    }

    ret = 0;

done:
    if (fp) {
        int ret_code = pclose(fp);
        if (ret_code == -1) {
            warn("Failed to run command: \"%s\"; Error: %s",
                    command, strerror(errno));
            if (ret == 0) {
                ret = -1;
            }
        } else if (ret_code != 0) {
            warn("Command \"%s\" exited unexpectedly.", command);
            if (ret == 0) {
                ret = -1;
            }
        }
    }

    if (ret != 0) {
        free_2d_buffer(buffer, buffer_size);
    }

    return ret;
}

short read_file(const char *filename, char ***buffer, unsigned *buffer_size)
{
    FILE *fp = NULL;
    short ret = -1;

    /* if filename is empty */
    if (!filename || strlen(filename) < 1) {
        warn("Given file name is empty.");
        goto done;
    }

    /* open file */
    debug("Reading \"%s\" file.", filename);
    fp = fopen(filename, "r");
    if (!fp) {
        warn("Failed to open \"%s\" file.", filename);
        goto done;
    }

    if (read_fp_to_2d_buffer(fp, buffer, buffer_size) != 0) {
        goto done;
    }

    ret = 0;

done:
    if (fp) {
        fclose(fp);
    }

    if (ret != 0) {
        free_2d_buffer(buffer, buffer_size);
    }

    return ret;
}

char *copy_string_part_after_delim(const char *str, const char *delim)
{
    if (!str || strlen(str) < 1 || !delim || strlen(delim) < 1) {
        return NULL;
    }

    char *p, *out = NULL;
    size_t delim_len = strlen(delim);

    /* if str contains delim and there is something after it */
    if ((p = strstr(str, delim)) && strlen(p + delim_len) > 0) {
        out = trim(p + delim_len, NULL);
    }

    return out;
}

char *trim(const char *str, const char *delims)
{
    char *out;
    const char *default_delims = WHITESPACES;
    size_t l;

    /* if string is empty */
    if (!str || strlen(str) < 1) {
        return NULL;
    }

    if (!delims) {
        delims = default_delims;
    }

    /* trim beginning of the string */
    while (strchr(delims, str[0]) && str[0] != '\0') {
        str++;
    }

    l = strlen(str);

    /* if string was only white spaces */
    if (l < 1) {
        return NULL;
    }

    /* shorten length of string if there are trailing white spaces */
    while (strchr(delims, str[l - 1]) && l > 0) {
        l--;
    }

    /* sanity check */
    if (l < 1) {
        return NULL;
    }

    /* copy string */
    out = strndup(str, l);
    if (!out) {
        warn("Failed to allocate memory.");
    }

    return out;
}

short explode(const char *str, const char *delims, char ***buffer, unsigned *buffer_size)
{
    size_t l;
    short ret = -1;
    unsigned item = 0, tmp_buffer_size;
    char *default_delims = WHITESPACES, *trimmed_str = NULL, *ts, **tmp_buffer;

    free_2d_buffer(buffer, buffer_size);

    if (!str || strlen(str) < 1) {
        ret = 0;
        goto done;
    }

    if (!delims) {
        delims = default_delims;
    }

    trimmed_str = trim(str, delims);
    if (!trimmed_str || strlen(trimmed_str) < 1) {
        ret = 0;
        goto done;
    }

    tmp_buffer_size = 128;
    tmp_buffer = (char **)calloc(tmp_buffer_size, sizeof(char *));
    if (!tmp_buffer) {
        warn("Failed to allocate memory.");
        tmp_buffer_size = 0;
        goto done;
    }

    ts = trimmed_str;
    while (ts[0] != '\0') {
        /* skip leading delimiters of substring */
        while (strchr(delims, ts[0]) && ts[0] != '\0') {
            ts++;
        }
        /* find length of valid substring */
        l = 0;
        while (!strchr(delims, ts[l]) && ts[l] != '\0') {
            l++;
        }
        /* reallocate if needed */
        if (item >= tmp_buffer_size) {
            tmp_buffer_size *= 2;
            char **new_temp = (char **)realloc(tmp_buffer,
                    tmp_buffer_size * sizeof(char *));
            if (!new_temp) {
                warn("Failed to allocate memory.");
                tmp_buffer_size /= 2;
                goto done;
            }
            tmp_buffer = new_temp;
        }
        /* copy the substring */
        tmp_buffer[item] = strndup(ts, l);
        if (!tmp_buffer[item]) {
            warn("Failed to allocate memory.");
            goto done;
        }
        item++;
        ts += l;
    }

    /* reallocate to save unused space */
    if (tmp_buffer_size > item) {
        char **new_temp = (char **)realloc(tmp_buffer,
                item * sizeof(char *));
        if (!new_temp) {
            warn("Failed to allocate memory.");
            goto done;
        }
        tmp_buffer = new_temp;
        tmp_buffer_size = item;
    }

    *buffer_size = tmp_buffer_size;
    *buffer = tmp_buffer;

    ret = 0;

done:
    free(trimmed_str);

    if (ret != 0) {
        free_2d_buffer(&tmp_buffer, &tmp_buffer_size);
    }

    return ret;
}

char *append_str(char *str, ...)
{
    va_list ap;
    size_t len, newlen;
    char *next, *end;

    if (str) {
        len = strlen(str);
    } else {
        len = 0;
    }

    /* count final length */
    va_start(ap, str);
    newlen = len + 1;
    while ((next = va_arg(ap, char *))) {
        newlen += strlen(next);
    }
    va_end(ap);

    /* reallocate string */
    char *temp = (char *)realloc(str, newlen);
    if (!temp) {
        warn("Failed to allocate memory.");
        return NULL;
    }
    str = temp;
    end = str + len;

    /* append parameters */
    va_start(ap, str);
    while ((next = va_arg(ap, char *))) {
        strcpy(end, next);
        end += strlen(next);
    }
    va_end(ap);

    return str;
}
