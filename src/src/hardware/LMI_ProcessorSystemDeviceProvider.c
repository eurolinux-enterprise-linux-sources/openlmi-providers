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
#include "LMI_ProcessorSystemDevice.h"
#include "utils.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_ProcessorSystemDeviceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_ProcessorSystemDeviceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorSystemDeviceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ProcessorSystemDeviceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_ProcessorSystemDevice lmi_cpu_sys_device;
    LMI_ProcessorRef lmi_cpu;
    const char *ns = KNameSpace(cop);
    unsigned i;
    DmiProcessor *dmi_cpus = NULL;
    unsigned dmi_cpus_nb = 0;

    if (dmi_get_processors(&dmi_cpus, &dmi_cpus_nb) != 0 || dmi_cpus_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_cpus_nb; i++) {
        LMI_ProcessorSystemDevice_Init(&lmi_cpu_sys_device, _cb, ns);

        LMI_ProcessorRef_Init(&lmi_cpu, _cb, ns);
        LMI_ProcessorRef_Set_SystemCreationClassName(&lmi_cpu,
                lmi_get_system_creation_class_name());
        LMI_ProcessorRef_Set_SystemName(&lmi_cpu, lmi_get_system_name_safe(cc));
        LMI_ProcessorRef_Set_CreationClassName(&lmi_cpu,
                LMI_Processor_ClassName);
        LMI_ProcessorRef_Set_DeviceID(&lmi_cpu, dmi_cpus[i].id);

        LMI_ProcessorSystemDevice_SetObjectPath_GroupComponent(
                &lmi_cpu_sys_device, lmi_get_computer_system_safe(cc));
        LMI_ProcessorSystemDevice_Set_PartComponent(&lmi_cpu_sys_device,
                &lmi_cpu);

        KReturnInstance(cr, lmi_cpu_sys_device);
    }

done:
    dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorSystemDeviceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ProcessorSystemDeviceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorSystemDeviceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorSystemDeviceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorSystemDeviceExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorSystemDeviceAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorSystemDeviceAssociators(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole,
    const char** properties)
{
    return KDefaultAssociators(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_ProcessorSystemDevice_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_ProcessorSystemDeviceAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return KDefaultAssociatorNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_ProcessorSystemDevice_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_ProcessorSystemDeviceReferences(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    return KDefaultReferences(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_ProcessorSystemDevice_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_ProcessorSystemDeviceReferenceNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role)
{
    return KDefaultReferenceNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_ProcessorSystemDevice_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_ProcessorSystemDevice,
    LMI_ProcessorSystemDevice,
    _cb,
    LMI_ProcessorSystemDeviceInitialize(ctx))

CMAssociationMIStub(
    LMI_ProcessorSystemDevice,
    LMI_ProcessorSystemDevice,
    _cb,
    LMI_ProcessorSystemDeviceInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ProcessorSystemDevice",
    "LMI_ProcessorSystemDevice",
    "instance association")
