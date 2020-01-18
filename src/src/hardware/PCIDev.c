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

#include "PCIDev.h"

short init_pci_access(struct pci_access **acc, const int flags)
{
    struct pci_dev *dev;

    if (!acc) {
        return -1;
    }

    if (*acc) {
        return 0;
    }

    if (!(*acc = pci_alloc())) {
      return -1;
    }

    pci_init(*acc);
    pci_scan_bus(*acc);

    for (dev = (*acc)->devices; dev; dev = dev->next) {
        pci_fill_info(dev, flags);
    }

    return 0;
}

void cleanup_pci_access(struct pci_access **acc)
{
    if (!acc) {
        return;
    }

    if (*acc) {
        pci_cleanup(*acc);
    }

    *acc = NULL;
}

void get_subid(struct pci_dev *d, u16 *subvp, u16 *subdp)
{
    *subvp = *subdp = 0xffff;
    u8 htype = pci_read_byte(d, PCI_HEADER_TYPE) & 0x7f;

    if (htype == PCI_HEADER_TYPE_NORMAL) {
        *subvp = pci_read_word(d, PCI_SUBSYSTEM_VENDOR_ID);
        *subdp = pci_read_word(d, PCI_SUBSYSTEM_ID);
    } else if (htype == PCI_HEADER_TYPE_BRIDGE) {
        struct pci_cap *cap = pci_find_cap(d, PCI_CAP_ID_SSVID, PCI_CAP_NORMAL);
        if (cap) {
            *subvp = pci_read_word(d, cap->addr + PCI_SSVID_VENDOR);
            *subdp = pci_read_word(d, cap->addr + PCI_SSVID_DEVICE);
        }
    } else if (htype == PCI_HEADER_TYPE_CARDBUS) {
        *subvp = pci_read_word(d, PCI_CB_SUBSYSTEM_VENDOR_ID);
        *subdp = pci_read_word(d, PCI_CB_SUBSYSTEM_ID);
    }
}

unsigned short get_capability(const u16 pci_cap)
{
    static struct {
        unsigned short cim_val;     /* CIM value */
        u16 pci_cap;                /* pci value */
    } values[] = {
        /*
        {0,  "Unknown"},
        {1,  "Other"},
        {2,  "Supports 66MHz"},
        {3,  "Supports User Definable Features"},
        {4,  "Supports Fast Back-to-Back Transactions"},
        */
        {5,  PCI_CAP_ID_PCIX},
        {6,  PCI_CAP_ID_PM},
        {7,  PCI_CAP_ID_MSI},
        /*
        {8,  "Parity Error Recovery Capable"},
        */
        {9,  PCI_CAP_ID_AGP},
        {10, PCI_CAP_ID_VPD},
        {11, PCI_CAP_ID_SLOTID},
        {12, PCI_CAP_ID_HOTPLUG},
        {13, PCI_CAP_ID_EXP},
        /*
        {14, "Supports PCIe Gen 2"},
        {15, "Supports PCIe Gen 3"},
        */
    };

    size_t i, val_length = sizeof(values) / sizeof(values[0]);

    for (i = 0; i < val_length; i++) {
        if (pci_cap == values[i].pci_cap) {
            return values[i].cim_val;
        }
    }

    return 1; /* Other */
}
