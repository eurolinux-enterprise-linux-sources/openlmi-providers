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

#ifndef VIRT_WHAT_H_
#define VIRT_WHAT_H_

#include "utils.h"

/*
 * Get virtual machine type if running as a virtual machine. If running in
 * bare metal, virt will be empty string.
 * @param virt, should be NULL, function will dynamically allocate it
 * @return 0 if success, negative value otherwise
 */
short virt_what_get_virtual_type(char **virt);

#endif /* VIRT_WHAT_H_ */
