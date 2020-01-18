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

#include <konkret/konkret.h>
#include "LMI_PhysicalMemory.h"
#include "utils.h"

#include "dmidecode.h"

CMPIUint16 get_form_factor(const char *dmi_ff);
CMPIUint16 get_memory_type(const char *dmi_type);

static const CMPIBroker* _cb = NULL;

static void LMI_PhysicalMemoryInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_PhysicalMemoryCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalMemoryEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PhysicalMemoryEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_PhysicalMemory lmi_phys_mem;
    const char *ns = KNameSpace(cop);
    char instance_id[BUFLEN];
    unsigned i;
    DmiMemory dmi_memory;

    if (dmi_get_memory(&dmi_memory) != 0 || dmi_memory.modules_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_memory.modules_nb; i++) {
        LMI_PhysicalMemory_Init(&lmi_phys_mem, _cb, ns);

        LMI_PhysicalMemory_Set_CreationClassName(&lmi_phys_mem,
                LMI_PhysicalMemory_ClassName);

        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_PhysicalMemory_ClassName ":%s",
                dmi_memory.modules[i].serial_number);

        LMI_PhysicalMemory_Set_Tag(&lmi_phys_mem,
                dmi_memory.modules[i].serial_number);
        LMI_PhysicalMemory_Set_Capacity(&lmi_phys_mem,
                dmi_memory.modules[i].size);
        LMI_PhysicalMemory_Set_FormFactor(&lmi_phys_mem,
                get_form_factor(dmi_memory.modules[i].form_factor));
        LMI_PhysicalMemory_Set_MemoryType(&lmi_phys_mem,
                get_memory_type(dmi_memory.modules[i].type));
        LMI_PhysicalMemory_Set_BankLabel(&lmi_phys_mem,
                dmi_memory.modules[i].bank_label);
        LMI_PhysicalMemory_Set_ElementName(&lmi_phys_mem,
                dmi_memory.modules[i].name);
        LMI_PhysicalMemory_Set_Manufacturer(&lmi_phys_mem,
                dmi_memory.modules[i].manufacturer);
        LMI_PhysicalMemory_Set_SerialNumber(&lmi_phys_mem,
                dmi_memory.modules[i].serial_number);
        LMI_PhysicalMemory_Set_PartNumber(&lmi_phys_mem,
                dmi_memory.modules[i].part_number);
        LMI_PhysicalMemory_Set_Caption(&lmi_phys_mem, "Physical Memory Module");
        LMI_PhysicalMemory_Set_Description(&lmi_phys_mem,
                "This object represents one physical memory module in system.");
        LMI_PhysicalMemory_Set_InstanceID(&lmi_phys_mem, instance_id);
        LMI_PhysicalMemory_Set_Name(&lmi_phys_mem, dmi_memory.modules[i].name);
        LMI_PhysicalMemory_Set_TotalWidth(&lmi_phys_mem,
                dmi_memory.modules[i].total_width);
        LMI_PhysicalMemory_Set_DataWidth(&lmi_phys_mem,
                dmi_memory.modules[i].data_width);

        if (dmi_memory.modules[i].speed_time) {
            LMI_PhysicalMemory_Set_Speed(&lmi_phys_mem,
                    dmi_memory.modules[i].speed_time);
        }
        if (dmi_memory.modules[i].speed_clock) {
            LMI_PhysicalMemory_Set_ConfiguredMemoryClockSpeed(&lmi_phys_mem,
                    dmi_memory.modules[i].speed_clock);
        }

        KReturnInstance(cr, lmi_phys_mem);
    }

done:
    dmi_free_memory(&dmi_memory);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalMemoryGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PhysicalMemoryCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

CMInstanceMIStub(
    LMI_PhysicalMemory,
    LMI_PhysicalMemory,
    _cb,
    LMI_PhysicalMemoryInitialize(ctx))

static CMPIStatus LMI_PhysicalMemoryMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalMemoryInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_PhysicalMemory_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

/*
 * Get Memory Type according to the dmidecode output.
 * @param dmi_type memory type from dmidecode
 * @return memory type ID defined by CIM
 */
CMPIUint16 get_memory_type(const char *dmi_type)
{
    if (!dmi_type || strlen(dmi_type) < 1) {
        return 0; /* Unknown */
    }

    static struct {
        CMPIUint16 value_map;   /* ValueMap defined by CIM */
        char *search;           /* String used to match the dmidecode memory type */
    } mem_types[] = {
        {0,  "Unknown"},
        {1,  "Other"},
        {2,  "DRAM"},
        /*
        {3,  "Synchronous DRAM"},
        {4,  "Cache DRAM"},
        {5,  "EDO"},
        */
        {6,  "EDRAM"},
        {7,  "VRAM"},
        {8,  "SRAM"},
        {9,  "RAM"},
        {10, "ROM"},
        {11, "Flash"},
        {12, "EEPROM"},
        {13, "FEPROM"},
        {14, "EPROM"},
        {15, "CDRAM"},
        {16, "3DRAM"},
        {17, "SDRAM"},
        {18, "SGRAM"},
        {19, "RDRAM"},
        {20, "DDR"},
        {21, "DDR2"},
        /*
        {22, "BRAM"},
        */
        {23, "DDR2 FB-DIMM"},
        {24, "DDR3"},
        {25, "FBD2"},
    };

    size_t i, types_length = sizeof(mem_types) / sizeof(mem_types[0]);

    for (i = 0; i < types_length; i++) {
        if (strcmp(dmi_type, mem_types[i].search) == 0) {
            return mem_types[i].value_map;
        }
    }

    return 1; /* Other */
}

/*
 * Get Form Factor according to the dmidecode output.
 * @param dmi_ff form factor from dmidecode
 * @return form factor ID defined by CIM
 */
CMPIUint16 get_form_factor(const char *dmi_ff)
{
    if (!dmi_ff || strlen(dmi_ff) < 1) {
        return 0; /* Unknown */
    }

    static struct {
        CMPIUint16 value_map;   /* ValueMap defined by CIM */
        char *search;           /* String used to match the dmidecode form factor */
    } form_factors[] = {
        {0,  "Unknown"},
        {1,  "Other"},
        {2,  "SIP"},
        {3,  "DIP"},
        {4,  "ZIP"},
        /*
        {5,  "SOJ"},
        */
        {6,  "Proprietary Card"},
        {7,  "SIMM"},
        {8,  "DIMM"},
        {9,  "TSOP"},
        /*
        {10, "PGA"},
        */
        {11, "RIMM"},
        {12, "SODIMM"},
        {13, "SRIMM"},
        /*
        {14, "SMD"},
        {15, "SSMP"},
        {16, "QFP"},
        {17, "TQFP"},
        {18, "SOIC"},
        {19, "LCC"},
        {20, "PLCC"},
        {21, "BGA"},
        {22, "FPBGA"},
        {23, "LGA"},
        */
    };

    size_t i, ff_length = sizeof(form_factors) / sizeof(form_factors[0]);

    for (i = 0; i < ff_length; i++) {
        if (strcmp(dmi_ff, form_factors[i].search) == 0) {
            return form_factors[i].value_map;
        }
    }

    return 1; /* Other */
}

CMMethodMIStub(
    LMI_PhysicalMemory,
    LMI_PhysicalMemory,
    _cb,
    LMI_PhysicalMemoryInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PhysicalMemory",
    "LMI_PhysicalMemory",
    "instance method")
