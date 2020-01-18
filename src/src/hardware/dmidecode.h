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

#ifndef DMIDECODE_H_
#define DMIDECODE_H_

#include "utils.h"

/* Processor from dmidecode. */
typedef struct _DmiProcessor {
    char *id;                   /* ID */
    char *family;               /* Family */
    char *status;               /* CPU Status */
    unsigned current_speed;     /* Current Speed in MHz */
    unsigned max_speed;         /* Max Speed in MHz */
    unsigned external_clock;    /* External Clock Speed in MHz */
    char *name;                 /* CPU name, version in dmidecode */
    unsigned cores;             /* Number of cores */
    unsigned enabled_cores;     /* Number of enabled cores */
    unsigned threads;           /* Number of threads */
    char *type;                 /* CPU Type/Role */
    char *stepping;             /* Stepping (revision level within family) */
    char *upgrade;              /* CPU upgrade method - socket */
    unsigned charact_nb;        /* Number of CPU Characteristics */
    char **characteristics;     /* CPU Characteristics */
    char *l1_cache_handle;      /* Level 1 Cache Handle */
    char *l2_cache_handle;      /* Level 2 Cache Handle */
    char *l3_cache_handle;      /* Level 3 Cache Handle */
    char *manufacturer;         /* CPU Manufacturer */
    char *serial_number;        /* CPU Serial Number */
    char *part_number;          /* CPU Part Number */
} DmiProcessor;

/* Processor cache from dmidecode. */
typedef struct _DmiCpuCache {
    char *id;                   /* ID */
    unsigned size;              /* Cache Size */
    char *name;                 /* Cache Name */
    char *status;               /* Cache Status (Enabled or Disabled) */
    unsigned level;             /* Cache Level */
    char *op_mode;              /* Cache Operational Mode (Write Back, ..) */
    char *type;                 /* Cache Type (Data, Instruction, Unified..) */
    char *associativity;        /* Cache Associativity */
} DmiCpuCache;

/* Memory module from dmidecode. */
typedef struct _DmiMemoryModule {
    unsigned long size;             /* Memory Module Size in Bytes */
    char *serial_number;            /* Memory Module Serial Number */
    char *form_factor;              /* Memory Module Form Factor */
    char *type;                     /* Memory Module Type */
    int slot;                       /* Slot where Memory Module is placed */
    unsigned speed_time;            /* Memory Speed in ns */
    char *bank_label;               /* Memory Module Bank Label */
    char *name;                     /* Memory Module Name */
    char *manufacturer;             /* Memory Module Manufacturer */
    char *part_number;              /* Memory Module Part Number */
    unsigned speed_clock;           /* Memory Module Speed in MHz */
    unsigned data_width;            /* Memory Module Data Width in bits */
    unsigned total_width;           /* Memory Module Total Width in bits */
} DmiMemoryModule;

/* Memory slot from dmidecode. */
typedef struct _DmiMemorySlot {
    int slot_number;                /* Slot number */
    char *name;                     /* Slot name */
} DmiMemorySlot;

/* Memory from dmidecode. */
typedef struct _DmiMemory {
    unsigned long physical_size;    /* Physical Memory Size in Bytes */
    unsigned long available_size;   /* Available Memory Size in Bytes */
    unsigned long start_addr;       /* Starting Address of Memory Array in KB */
    unsigned long end_addr;         /* Ending Address of Memory Array in KB */
    DmiMemoryModule *modules;       /* Memory Modules */
    unsigned modules_nb;            /* Number of Memory Modules */
    DmiMemorySlot *slots;           /* Memory slots, including empty ones */
    unsigned slots_nb;              /* Number of all memory slots */
} DmiMemory;

/* Chassis from dmidecode. */
typedef struct _DmiChassis {
    char *serial_number;            /* Chassis serial number */
    char *type;                     /* Chassis Type */
    char *manufacturer;             /* Chassis Manufacturer */
    char *sku_number;               /* Chassis SKU number */
    char *version;                  /* Chassis Version */
    short has_lock;                 /* Has chassis lock? 0 or 1 */
    unsigned power_cords;           /* Number of Power Cords */
    char *asset_tag;                /* Asset Tag */
    char *model;                    /* Model (Version field in dmidecode) */
    char *product_name;             /* Product Name */
    char *uuid;                     /* UUID */
} DmiChassis;

/* Baseboard from dmidecode. */
typedef struct _DmiBaseboard {
    char *serial_number;            /* Serial number */
    char *manufacturer;             /* Manufacturer */
    char *product_name;             /* Product Name */
    char *version;                  /* Version */
} DmiBaseboard;

/* Baseboard Port from dmidecode. */
typedef struct _DmiPort {
    char *name;                     /* Name */
    char *type;                     /* Type */
    char *port_type;                /* Port Type */
} DmiPort;

/* System Slot from dmidecode. */
typedef struct _DmiSystemSlot {
    char *name;                     /* Name */
    unsigned number;                /* Slot number */
    char *type;                     /* Slot type */
    unsigned data_width;            /* Data width */
    char *link_width;               /* Link width */
    short supports_hotplug;         /* Supports slot hotplug? */
} DmiSystemSlot;

/* Pointing Device from dmidecode. */
typedef struct _DmiPointingDevice {
    char *type;                     /* Type */
    unsigned buttons;               /* Number of buttons */
} DmiPointingDevice;

/* Battery from dmidecode. */
typedef struct _DmiBattery {
    char *name;                     /* Battery name */
    char *chemistry;                /* Battery chemistry */
    unsigned design_capacity;       /* Design capacity */
    unsigned design_voltage;        /* Design voltage */
    char *manufacturer;             /* Manufacturer */
    char *serial_number;            /* Serial number */
    char *version;                  /* Version */
    char *manufacture_date;         /* Manufacture date */
    char *location;                 /* Battery location */
} DmiBattery;

/*
 * Get array of processors according to the dmidecode program.
 * @param cpus array of cpus, this function will allocate necessary memory,
 *      but caller is responsible for freeing it
 * @param cpus_nb number of processors in cpus
 * @return 0 if success, negative value otherwise
 */
short dmi_get_processors(DmiProcessor **cpus, unsigned *cpus_nb);

/*
 * Free array of processor structures.
 * @param cpus array of cpus
 * @param cpus_nb number of cpus
 */
void dmi_free_processors(DmiProcessor **cpus, unsigned *cpus_nb);

/*
 * Get array of processor caches according to the dmidecode program.
 * @param caches array of cpu caches, this function will allocate necessary
 *      memory, but caller is responsible for freeing it
 * @param caches_nb number of caches in caches
 * @return 0 if success, negative value otherwise
 */
short dmi_get_cpu_caches(DmiCpuCache **caches, unsigned *caches_nb);

/*
 * Free array of cpu cache structures.
 * @param caches array of caches
 * @param caches_nb number of caches
 */
void dmi_free_cpu_caches(DmiCpuCache **caches, unsigned *caches_nb);

/*
 * Get memory structure according to the dmidecode program.
 * @param memory structure, this function will allocate
 *      necessary memory, but caller is responsible for freeing it
 * @return 0 if success, negative value otherwise
 */
short dmi_get_memory(DmiMemory *memory);

/*
 * Free memory structure.
 * @param memory structure
 */
void dmi_free_memory(DmiMemory *memory);

/*
 * Get chassis structure according to the dmidecode program.
 * @param chassis structure, this function will allocate
 *      necessary memory, but caller is responsible for freeing it
 * @return 0 if success, negative value otherwise
 */
short dmi_get_chassis(DmiChassis *chassis);

/*
 * Free chassis structure.
 * @param chassis structure
 */
void dmi_free_chassis(DmiChassis *chassis);

/*
 * Get best available chassis tag from given chassis. Return value should not
 * be freed.
 * @param chassis
 * @return tag
 */
char *dmi_get_chassis_tag(DmiChassis *chassis);

/*
 * Get baseboard structure according to the dmidecode program.
 * @param baseboard structure, this function will allocate
 *      necessary memory, but caller is responsible for freeing it
 * @return 0 if success, negative value otherwise
 */
short dmi_get_baseboard(DmiBaseboard *baseboard);

/*
 * Free baseboard structure.
 * @param baseboard structure
 */
void dmi_free_baseboard(DmiBaseboard *baseboard);

/*
 * Get array of baseboard ports according to the dmidecode program.
 * @param ports array of ports, this function will allocate necessary memory,
 *      but caller is responsible for freeing it
 * @param ports_nb number of ports
 * @return 0 if success, negative value otherwise
 */
short dmi_get_ports(DmiPort **ports, unsigned *ports_nb);

/*
 * Free array of port structures.
 * @param ports array of ports
 * @param ports_nb number of ports
 */
void dmi_free_ports(DmiPort **ports, unsigned *ports_nb);

/*
 * Get array of system slots according to the dmidecode program.
 * @param slots array of slots, this function will allocate necessary memory,
 *      but caller is responsible for freeing it
 * @param slots_nb number of slots
 * @return 0 if success, negative value otherwise
 */
short dmi_get_system_slots(DmiSystemSlot **slots, unsigned *slots_nb);

/*
 * Free array of slot structures.
 * @param slots array of slots
 * @param slots_nb number of slots
 */
void dmi_free_system_slots(DmiSystemSlot **slots, unsigned *slots_nb);

/*
 * Get array of pointing devices according to the dmidecode program.
 * @param devices array of devices, this function will allocate necessary memory,
 *      but caller is responsible for freeing it
 * @param devices_nb number of devices
 * @return 0 if success, negative value otherwise
 */
short dmi_get_pointing_devices(DmiPointingDevice **devices, unsigned *devices_nb);

/*
 * Free array of pointing device structures.
 * @param devices array of pointing devices
 * @param devices_nb number of pointing devices
 */
void dmi_free_pointing_devices(DmiPointingDevice **devices, unsigned *devices_nb);

/*
 * Get array of batteries according to the dmidecode program.
 * @param batteries array of batteries, this function will allocate necessary memory,
 *      but caller is responsible for freeing it
 * @param batteries_nb number of batteries
 * @return 0 if success, negative value otherwise
 */
short dmi_get_batteries(DmiBattery **batteries, unsigned *batteries_nb);

/*
 * Free array of batteries structures.
 * @param batteries array of batteries
 * @param batteries_nb number of batteries
 */
void dmi_free_batteries(DmiBattery **batteries, unsigned *batteries_nb);

#endif /* DMIDECODE_H_ */
