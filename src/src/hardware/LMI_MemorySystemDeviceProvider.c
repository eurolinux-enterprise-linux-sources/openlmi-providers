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
#include "LMI_MemorySystemDevice.h"
#include "utils.h"

static const CMPIBroker* _cb;

static void LMI_MemorySystemDeviceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_MemorySystemDeviceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemorySystemDeviceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_MemorySystemDeviceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_MemorySystemDevice lmi_mem_sys_device;
    LMI_MemoryRef lmi_mem;
    const char *ns = KNameSpace(cop);

    LMI_MemorySystemDevice_Init(&lmi_mem_sys_device, _cb, ns);

    LMI_MemoryRef_Init(&lmi_mem, _cb, ns);
    LMI_MemoryRef_Set_SystemCreationClassName(&lmi_mem,
            lmi_get_system_creation_class_name());
    LMI_MemoryRef_Set_SystemName(&lmi_mem, lmi_get_system_name_safe(cc));
    LMI_MemoryRef_Set_CreationClassName(&lmi_mem, LMI_Memory_ClassName);
    LMI_MemoryRef_Set_DeviceID(&lmi_mem, "0");

    LMI_MemorySystemDevice_SetObjectPath_GroupComponent(
            &lmi_mem_sys_device, lmi_get_computer_system_safe(cc));
    LMI_MemorySystemDevice_Set_PartComponent(&lmi_mem_sys_device,
            &lmi_mem);

    KReturnInstance(cr, lmi_mem_sys_device);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemorySystemDeviceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_MemorySystemDeviceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySystemDeviceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySystemDeviceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySystemDeviceExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySystemDeviceAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemorySystemDeviceAssociators(
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
        LMI_MemorySystemDevice_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_MemorySystemDeviceAssociatorNames(
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
        LMI_MemorySystemDevice_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_MemorySystemDeviceReferences(
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
        LMI_MemorySystemDevice_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_MemorySystemDeviceReferenceNames(
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
        LMI_MemorySystemDevice_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_MemorySystemDevice,
    LMI_MemorySystemDevice,
    _cb,
    LMI_MemorySystemDeviceInitialize(ctx))

CMAssociationMIStub(
    LMI_MemorySystemDevice,
    LMI_MemorySystemDevice,
    _cb,
    LMI_MemorySystemDeviceInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_MemorySystemDevice",
    "LMI_MemorySystemDevice",
    "instance association")
