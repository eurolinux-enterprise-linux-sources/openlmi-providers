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
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include "LMI_Memory.h"
#include "utils.h"
#include "dmidecode.h"
#include "procfs.h"
#include "sysfs.h"

static const CMPIBroker* _cb = NULL;

static void LMI_MemoryInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_MemoryCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemoryEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_MemoryEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_Memory lmi_mem;
    const char *ns = KNameSpace(cop), *name = "System Memory";
    char *error_msg = NULL;
    long page_size;
    unsigned long fallback_memory_size = 0;
    DmiMemory dmi_memory;
    unsigned i, *huge_page_sizes = NULL, huge_page_sizes_nb;

    if (dmi_get_memory(&dmi_memory) != 0) {
        fallback_memory_size = meminfo_get_memory_size();
        if (!fallback_memory_size) {
            error_msg = "Unable to get memory information.";
            goto done;
        }
    }

    page_size = sysconf(_SC_PAGESIZE);
    sysfs_get_sizes_of_hugepages(&huge_page_sizes, &huge_page_sizes_nb);

    LMI_Memory_Init(&lmi_mem, _cb, ns);

    LMI_Memory_Set_SystemCreationClassName(&lmi_mem,
            lmi_get_system_creation_class_name());
    LMI_Memory_Set_SystemName(&lmi_mem, lmi_get_system_name_safe(cc));
    LMI_Memory_Set_CreationClassName(&lmi_mem, LMI_Memory_ClassName);

    LMI_Memory_Set_DeviceID(&lmi_mem, "0");
    LMI_Memory_Set_Volatile(&lmi_mem, 1);
    LMI_Memory_Set_Access(&lmi_mem, LMI_Memory_Access_Read_Write_Supported);
    LMI_Memory_Set_BlockSize(&lmi_mem, 1);
    LMI_Memory_Set_EnabledState(&lmi_mem, LMI_Memory_EnabledState_Enabled);
    LMI_Memory_Init_OperationalStatus(&lmi_mem, 1);
    LMI_Memory_Set_OperationalStatus(&lmi_mem, 0,
            LMI_Memory_OperationalStatus_Unknown);
    LMI_Memory_Set_HealthState(&lmi_mem, LMI_Memory_HealthState_Unknown);
    LMI_Memory_Set_ElementName(&lmi_mem, name);
    LMI_Memory_Set_Caption(&lmi_mem, name);
    LMI_Memory_Set_Name(&lmi_mem, name);
    LMI_Memory_Set_Description(&lmi_mem,
            "This object represents all memory available in system.");
    LMI_Memory_Set_InstanceID(&lmi_mem,
            LMI_ORGID ":" LMI_Memory_ClassName ":0");
    LMI_Memory_Set_IsCompressed(&lmi_mem, 0);
    LMI_Memory_Set_Purpose(&lmi_mem, "The system memory is temporary storage "
            "area storing instructions and data required by processor "
            "to run programs.");

    if (!fallback_memory_size) {
        LMI_Memory_Set_NumberOfBlocks(&lmi_mem, dmi_memory.physical_size);
        LMI_Memory_Set_ConsumableBlocks(&lmi_mem, dmi_memory.available_size);
        LMI_Memory_Set_StartingAddress(&lmi_mem, dmi_memory.start_addr);
        LMI_Memory_Set_EndingAddress(&lmi_mem, dmi_memory.end_addr);
    } else {
        LMI_Memory_Set_NumberOfBlocks(&lmi_mem, fallback_memory_size);
    }

    LMI_Memory_Set_HasNUMA(&lmi_mem, sysfs_has_numa());
    if (page_size > 0) {
        LMI_Memory_Set_StandardMemoryPageSize(&lmi_mem, page_size / 1024);
    }
    if (huge_page_sizes_nb > 0) {
        LMI_Memory_Init_SupportedHugeMemoryPageSizes(&lmi_mem,
                huge_page_sizes_nb);
        for (i = 0; i < huge_page_sizes_nb; i++) {
            LMI_Memory_Set_SupportedHugeMemoryPageSizes(&lmi_mem, i,
                    huge_page_sizes[i]);
        }
    }
    LMI_Memory_Set_TransparentHugeMemoryPageStatus(&lmi_mem,
            sysfs_get_transparent_hugepages_status());

    KReturnInstance(cr, lmi_mem);

done:
    dmi_free_memory(&dmi_memory);
    free(huge_page_sizes);

    if (error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemoryGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_MemoryCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemoryModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemoryDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemoryExecQuery(
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
    LMI_Memory,
    LMI_Memory,
    _cb,
    LMI_MemoryInitialize(ctx))

static CMPIStatus LMI_MemoryMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemoryInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Memory_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_Memory,
    LMI_Memory,
    _cb,
    LMI_MemoryInitialize(ctx))

KUint32 LMI_Memory_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_MemoryRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Memory_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_MemoryRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Memory_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_MemoryRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Memory_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_MemoryRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Memory_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_MemoryRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Memory_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_MemoryRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Memory_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_MemoryRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Memory_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_MemoryRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_Memory",
    "LMI_Memory",
    "instance method")
