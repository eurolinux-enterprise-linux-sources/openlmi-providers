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

#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "globals.h"

#define LONG_INT_LEN 21     /* 64 bit unsigned int can has 20 decimals + \0 */

#define WHITESPACES " \f\n\r\t\v"


/*
 * Read given file pointer and save it's output in buffer. Number of lines read
 * is stored in buffer_size. Function skips lines starting with '#'.
 * Every line from output is trimmed.
 * Buffer has to be NULL, and buffer_size 0. Function will allocate
 * necessary memory, buffer can be freed with free_2d_buffer() function.
 * @param fp file pointer to be read
 * @param buffer which will be filled in
 * @param buffer_size number of lines in buffer
 * @return 0 if success, negative value otherwise
 */
short read_fp_to_2d_buffer(FILE *fp, char ***buffer, unsigned *buffer_size);

/*
 * Free 2D buffer.
 * @param buffer
 * @param buffer_size number of lines in buffer
 */
void free_2d_buffer(char ***buffer, unsigned *buffer_size);

/*
 * Run given command and store its output in buffer. Number of lines in buffer
 * is stored in buffer_size. Function skips lines starting with '#'.
 * Buffer has to be NULL, and buffer_size 0. Function will allocate necessary
 * memory, buffer can be freed with free_2d_buffer() function.
 * @param command to be run
 * @param buffer
 * @param buffer_size number of lines in buffer
 * @return 0 if success, negative value otherwise
 */
short run_command(const char *command, char ***buffer, unsigned *buffer_size);

/*
 * Run given file and store its output in buffer. Number of lines in buffer
 * is stored in buffer_size. Function skips lines starting with '#'.
 * Buffer has to be NULL, and buffer_size 0. Function will allocate necessary
 * memory, buffer can be freed with free_2d_buffer() function.
 * @param filename
 * @param buffer
 * @param buffer_size number of lines in buffer
 * @return 0 if success, negative value otherwise
 */
short read_file(const char *filename, char ***buffer, unsigned *buffer_size);

/*
 * Copy trimmed part of the given string after delimiter and returns pointer
 * to the newly created string, allocated with malloc.
 * If delimiter is not part of the string, or the string ends right after
 * delimiter, NULL is returned.
 * @param str string to be searched
 * @param delim delimiter
 * @return newly created string or NULL
 */
char *copy_string_part_after_delim(const char *str, const char *delim);

/*
 * Create trimmed copy of given string. Trimmed will be any characters
 * found in delims parameter, or, if delims is NULL, any white space characters.
 * @param str
 * @param delims string containing delimiters. If NULL, white space characters
 *      are used.
 * @return trimmed string allocated with malloc or NULL if allocation failed
 *      or given string was empty or contained only delimiters
 */
char *trim(const char *str, const char *delims);

/*
 * Explode given string to substrings delimited by delims.
 * @param str input string
 * @param delims string consisted of delimiters
 * @param buffer output 2D buffer. Can be NULL if input string is NULL, empty,
 *      or only delimiters.
 * @param buffer_size number of substrings
 * @return 0 if success, negative value otherwise.
 */
short explode(const char *str, const char *delims, char ***buffer, unsigned *buffer_size);

/*
 * Append strings to the first string parameter.
 * @param str string where the rest of params will be appended. Can be NULL.
 *      When appending, str will be reallocated to the correct size.
 *      Last parameter must be NULL.
 * @return pointer to the final string (same as str, if it wasn't NULL). In case
 *      of any problem, NULL is returned.
 */
char *append_str(char *str, ...);


#endif /* UTILS_H_ */
