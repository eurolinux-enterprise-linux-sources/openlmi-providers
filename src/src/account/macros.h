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

#ifndef MACROS_H
#define MACROS_H

/* Few global names of instances */
#define LAMSNAME "OpenLMI Linux Users Account Management Service"
#define LAMCNAME "OpenLMI Linux Users Account Management Capabilities"
#define LEACNAME "OpenLMI Linux Account Capabilities"

/* Organization ID. Used for InstaceIDs */
#define ORGID "LMI"

/* convert days to microseconds */
#define DAYSTOMS(days) ((days) * 86400000000)
#define MSTODAYS(ms) ((ms) / 86400000000)
#define STOMS(s) ((s) * 1000000)

/* This will identify empty values in shadow file */
#define SHADOW_VALUE_EMPTY -1

/* libuser uses this value to disable checking for password change */
#define SHADOW_MAX_DISABLED 99999

#endif
