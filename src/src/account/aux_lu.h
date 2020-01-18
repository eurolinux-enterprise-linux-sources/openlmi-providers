/*
 * Copyright (C) 2012-2013 Red Hat, Inc.  All rights reserved.
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
 * Authors: Roman Rakus <rrakus@redhat.com>
 */

#ifndef AUX_LU_H
#define AUX_LU_H

// for GValueArray
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <libuser/entity.h>
#include <libuser/user.h>

const char* aux_lu_get_str(struct lu_ent*, char*);
long aux_lu_get_long(struct lu_ent*, char*);

/* Get the latest login time for given user name */
time_t aux_utmp_latest(const char*);

#endif
