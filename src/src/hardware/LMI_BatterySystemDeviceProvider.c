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
#include "LMI_BatterySystemDevice.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_BatterySystemDeviceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_BatterySystemDeviceCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BatterySystemDeviceEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_BatterySystemDeviceEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_BatterySystemDevice lmi_batt_sys_device;
    LMI_BatteryRef lmi_batt;
    const char *ns = KNameSpace(cop);
    unsigned i;
    DmiBattery *dmi_batt = NULL;
    unsigned dmi_batt_nb = 0;

    if (dmi_get_batteries(&dmi_batt, &dmi_batt_nb) != 0 || dmi_batt_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_batt_nb; i++) {
        LMI_BatterySystemDevice_Init(&lmi_batt_sys_device, _cb, ns);

        LMI_BatteryRef_Init(&lmi_batt, _cb, ns);
        LMI_BatteryRef_Set_SystemCreationClassName(&lmi_batt,
                get_system_creation_class_name());
        LMI_BatteryRef_Set_SystemName(&lmi_batt, get_system_name());
        LMI_BatteryRef_Set_CreationClassName(&lmi_batt,
                ORGID "_" BATTERY_CLASS_NAME);
        LMI_BatteryRef_Set_DeviceID(&lmi_batt, dmi_batt[i].name);

        LMI_BatterySystemDevice_SetObjectPath_GroupComponent(
                &lmi_batt_sys_device, lmi_get_computer_system());
        LMI_BatterySystemDevice_Set_PartComponent(&lmi_batt_sys_device,
                &lmi_batt);

        KReturnInstance(cr, lmi_batt_sys_device);
    }

done:
    dmi_free_batteries(&dmi_batt, &dmi_batt_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BatterySystemDeviceGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_BatterySystemDeviceCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BatterySystemDeviceModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BatterySystemDeviceDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BatterySystemDeviceExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BatterySystemDeviceAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BatterySystemDeviceAssociators(
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
        LMI_BatterySystemDevice_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_BatterySystemDeviceAssociatorNames(
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
        LMI_BatterySystemDevice_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_BatterySystemDeviceReferences(
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
        LMI_BatterySystemDevice_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_BatterySystemDeviceReferenceNames(
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
        LMI_BatterySystemDevice_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_BatterySystemDevice,
    LMI_BatterySystemDevice,
    _cb,
    LMI_BatterySystemDeviceInitialize(ctx))

CMAssociationMIStub( 
    LMI_BatterySystemDevice,
    LMI_BatterySystemDevice,
    _cb,
    LMI_BatterySystemDeviceInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_BatterySystemDevice",
    "LMI_BatterySystemDevice",
    "instance association")
