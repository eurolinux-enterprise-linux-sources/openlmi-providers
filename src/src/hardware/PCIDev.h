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

#ifndef PCIDEV_H_
#define PCIDEV_H_

#include <stddef.h>
#include <pci/pci.h>

/*
 * Initialize pci access structure.
 * @param acc pci access structure
 * @param flags flags to be passed to pci_fill_info() fn
 * @return 0 on success, negative value otherwise
 */
short init_pci_access(struct pci_access **acc, const int flags);

/*
 * Cleanup pci access structure.
 * @param acc pci access structure
 */
void cleanup_pci_access(struct pci_access **acc);

/*
 * Get subsystem IDs of pci device.
 * @param d pci device
 * @param subvp subsystem vendor id
 * @param subdp subsystem id
 */
void get_subid(struct pci_dev *d, u16 *subvp, u16 *subdp);

/*
 * Get pci capability according to the pci lib.
 * @param pci_cap from pci lib
 * @return CIM id of pci capability
 */
unsigned short get_capability(const u16 pci_cap);

#endif /* PCIDEV_H_ */
