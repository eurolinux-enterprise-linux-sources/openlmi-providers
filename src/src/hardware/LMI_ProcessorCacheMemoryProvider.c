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

#include <konkret/konkret.h>
#include "LMI_ProcessorCacheMemory.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"
#include "sysfs.h"

CMPIUint16 get_cachestatus(const char *status);

static const CMPIBroker* _cb = NULL;

static void LMI_ProcessorCacheMemoryInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_ProcessorCacheMemoryCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorCacheMemoryEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ProcessorCacheMemoryEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_ProcessorCacheMemory lmi_cpu_cache;
    const char *ns = KNameSpace(cop);
    char *error_msg = NULL, instance_id[INSTANCE_ID_LEN];
    unsigned i, caches = 0;
    DmiCpuCache *dmi_cpu_caches = NULL;
    unsigned dmi_cpu_caches_nb = 0;
    SysfsCpuCache *sysfs_cpu_caches = NULL;
    unsigned sysfs_cpu_caches_nb = 0;

    if (dmi_get_cpu_caches(&dmi_cpu_caches, &dmi_cpu_caches_nb) != 0
            || dmi_cpu_caches_nb < 1) {
        dmi_free_cpu_caches(&dmi_cpu_caches, &dmi_cpu_caches_nb);

        if (sysfs_get_cpu_caches(&sysfs_cpu_caches, &sysfs_cpu_caches_nb) != 0
                || sysfs_cpu_caches_nb < 1) {
            error_msg = "Unable to get processor cache information.";
            goto done;
        }
    }

    if (dmi_cpu_caches_nb > 0) {
        caches = dmi_cpu_caches_nb;
    } else {
        caches = sysfs_cpu_caches_nb;
    }

    for (i = 0; i < caches; i++) {
        LMI_ProcessorCacheMemory_Init(&lmi_cpu_cache, _cb, ns);

        LMI_ProcessorCacheMemory_Set_SystemCreationClassName(&lmi_cpu_cache,
                get_system_creation_class_name());
        LMI_ProcessorCacheMemory_Set_SystemName(&lmi_cpu_cache,
                get_system_name());
        LMI_ProcessorCacheMemory_Set_CreationClassName(&lmi_cpu_cache,
                ORGID "_" CPU_CACHE_CLASS_NAME);

        LMI_ProcessorCacheMemory_Set_BlockSize(&lmi_cpu_cache, 1);
        LMI_ProcessorCacheMemory_Set_Volatile(&lmi_cpu_cache, 1);
        LMI_ProcessorCacheMemory_Set_HealthState(&lmi_cpu_cache,
                LMI_ProcessorCacheMemory_HealthState_Unknown);
        LMI_ProcessorCacheMemory_Init_OperationalStatus(&lmi_cpu_cache, 1);
        LMI_ProcessorCacheMemory_Set_OperationalStatus(&lmi_cpu_cache, 0,
                LMI_ProcessorCacheMemory_OperationalStatus_Unknown);
        LMI_ProcessorCacheMemory_Set_Access(&lmi_cpu_cache,
                LMI_ProcessorCacheMemory_Access_Read_Write_Supported);
        LMI_ProcessorCacheMemory_Set_Caption(&lmi_cpu_cache,
                "Processor Cache Memory");
        LMI_ProcessorCacheMemory_Set_Description(&lmi_cpu_cache,
                "This object represents one cache memory of processor in system.");
        LMI_ProcessorCacheMemory_Set_IsCompressed(&lmi_cpu_cache, 0);
        LMI_ProcessorCacheMemory_Set_Purpose(&lmi_cpu_cache,
                "Processor cache is used to reduce the average time to "
                "access memory. The cache is a smaller, faster memory which "
                "stores copies of the data from the most frequently used main "
                "memory locations.");

        /* do we have dmidecode output? */
        if (dmi_cpu_caches_nb > 0) {
            snprintf(instance_id, INSTANCE_ID_LEN,
                    ORGID ":" ORGID "_" CPU_CACHE_CLASS_NAME ":%s",
                    dmi_cpu_caches[i].id);

            LMI_ProcessorCacheMemory_Set_DeviceID(&lmi_cpu_cache,
                    dmi_cpu_caches[i].id);

            LMI_ProcessorCacheMemory_Set_NumberOfBlocks(&lmi_cpu_cache,
                    dmi_cpu_caches[i].size);
            LMI_ProcessorCacheMemory_Set_ElementName(&lmi_cpu_cache,
                    dmi_cpu_caches[i].name);
            LMI_ProcessorCacheMemory_Set_Name(&lmi_cpu_cache,
                    dmi_cpu_caches[i].name);
            LMI_ProcessorCacheMemory_Set_EnabledState(&lmi_cpu_cache,
                    get_cachestatus(dmi_cpu_caches[i].status));
        } else {
            snprintf(instance_id, INSTANCE_ID_LEN,
                    ORGID ":" ORGID "_" CPU_CACHE_CLASS_NAME ":%s",
                    sysfs_cpu_caches[i].id);

            LMI_ProcessorCacheMemory_Set_DeviceID(&lmi_cpu_cache,
                    sysfs_cpu_caches[i].id);

            LMI_ProcessorCacheMemory_Set_NumberOfBlocks(&lmi_cpu_cache,
                    sysfs_cpu_caches[i].size);
            LMI_ProcessorCacheMemory_Set_ElementName(&lmi_cpu_cache,
                    sysfs_cpu_caches[i].name);
            LMI_ProcessorCacheMemory_Set_Name(&lmi_cpu_cache,
                    sysfs_cpu_caches[i].name);
        }

        LMI_ProcessorCacheMemory_Set_InstanceID(&lmi_cpu_cache, instance_id);

        KReturnInstance(cr, lmi_cpu_cache);
    }

done:
    dmi_free_cpu_caches(&dmi_cpu_caches, &dmi_cpu_caches_nb);
    sysfs_free_cpu_caches(&sysfs_cpu_caches, &sysfs_cpu_caches_nb);

    if (error_msg) {
        KReturn2(_cb, ERR_FAILED, error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorCacheMemoryGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ProcessorCacheMemoryCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorCacheMemoryModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorCacheMemoryDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorCacheMemoryExecQuery(
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
    LMI_ProcessorCacheMemory,
    LMI_ProcessorCacheMemory,
    _cb,
    LMI_ProcessorCacheMemoryInitialize(ctx))

static CMPIStatus LMI_ProcessorCacheMemoryMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorCacheMemoryInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_ProcessorCacheMemory_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_ProcessorCacheMemory,
    LMI_ProcessorCacheMemory,
    _cb,
    LMI_ProcessorCacheMemoryInitialize(ctx))

KUint32 LMI_ProcessorCacheMemory_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorCacheMemoryRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_ProcessorCacheMemory_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorCacheMemoryRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_ProcessorCacheMemory_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorCacheMemoryRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_ProcessorCacheMemory_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorCacheMemoryRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_ProcessorCacheMemory_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorCacheMemoryRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_ProcessorCacheMemory_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorCacheMemoryRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_ProcessorCacheMemory_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorCacheMemoryRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_ProcessorCacheMemory_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorCacheMemoryRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

/*
 * Get CPU Cache status according to the dmidecode.
 * @param status from dmidecode
 * @return CIM id of CPU Cache status
 */
CMPIUint16 get_cachestatus(const char *status)
{
    if (!status || strlen(status) < 1) {
        return 5; /* Not Applicable */
    }

    static struct {
        CMPIUint16 val;     /* CIM value */
        char *stat;         /* dmidecode status */
    } statuses[] = {
        {2, "Enabled"},
        {3, "Disabled"},
    };

    size_t i, st_length = sizeof(statuses) / sizeof(statuses[0]);

    for (i = 0; i < st_length; i++) {
        if (strcmp(status, statuses[i].stat) == 0) {
            return statuses[i].val;
        }
    }

    return 5; /* Not Applicable */
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ProcessorCacheMemory",
    "LMI_ProcessorCacheMemory",
    "instance method")
