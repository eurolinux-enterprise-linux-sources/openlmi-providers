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

#ifndef LMI_HARDWARE_H_
#define LMI_HARDWARE_H_

#include "globals.h"

const ConfigEntry *provider_config_defaults;
const char *provider_name;

#define INSTANCE_ID_LEN 128
#define ELEMENT_NAME_LEN 128

#define CPU_CLASS_NAME "Processor"
#define CPU_CAP_CLASS_NAME "ProcessorCapabilities"
#define CPU_CACHE_CLASS_NAME "ProcessorCacheMemory"
#define CPU_CHIP_CLASS_NAME "ProcessorChip"
#define MEM_CLASS_NAME "Memory"
#define PHYS_MEM_CLASS_NAME "PhysicalMemory"
#define CHASSIS_CLASS_NAME "Chassis"
#define BASEBOARD_CLASS_NAME "Baseboard"
#define MEMORY_SLOT_CLASS_NAME "MemorySlot"
#define MEMORY_PHYS_PKG_CLASS_NAME "MemoryPhysicalPackage"
#define PORT_PHYS_CONN_CLASS_NAME "PortPhysicalConnector"
#define SYSTEM_SLOT_CLASS_NAME "SystemSlot"
#define POINTING_DEVICE_CLASS_NAME "PointingDevice"
#define BATTERY_CLASS_NAME "Battery"
#define BATTERY_PHYS_PKG_CLASS_NAME "BatteryPhysicalPackage"
#define PCI_DEVICE_CLASS_NAME "PCIDevice"
#define PCI_BRIDGE_CLASS_NAME "PCIBridge"

#endif /* LMI_HARDWARE_H_ */
