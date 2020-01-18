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
#include "LMI_PhysicalBatteryContainer.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_PhysicalBatteryContainerInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_PhysicalBatteryContainerCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalBatteryContainerEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PhysicalBatteryContainerEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_PhysicalBatteryContainer lmi_batt_container;
    LMI_BatteryPhysicalPackageRef lmi_batt_phys;
    LMI_ChassisRef lmi_chassis;
    const char *ns = KNameSpace(cop);
    unsigned i;
    DmiChassis dmi_chassis;
    DmiBattery *dmi_batt = NULL;
    unsigned dmi_batt_nb = 0;

    if (dmi_get_chassis(&dmi_chassis) != 0) {
        goto done;
    }
    if (dmi_get_batteries(&dmi_batt, &dmi_batt_nb) != 0 || dmi_batt_nb < 1) {
        goto done;
    }

    LMI_ChassisRef_Init(&lmi_chassis, _cb, ns);
    LMI_ChassisRef_Set_CreationClassName(&lmi_chassis,
            ORGID "_" CHASSIS_CLASS_NAME);
    LMI_ChassisRef_Set_Tag(&lmi_chassis, dmi_get_chassis_tag(&dmi_chassis));

    for (i = 0; i < dmi_batt_nb; i++) {
        LMI_PhysicalBatteryContainer_Init(&lmi_batt_container, _cb, ns);

        LMI_BatteryPhysicalPackageRef_Init(&lmi_batt_phys, _cb, ns);
        LMI_BatteryPhysicalPackageRef_Set_CreationClassName(&lmi_batt_phys,
                ORGID "_" BATTERY_PHYS_PKG_CLASS_NAME);
        LMI_BatteryPhysicalPackageRef_Set_Tag(&lmi_batt_phys, dmi_batt[i].name);

        LMI_PhysicalBatteryContainer_Set_GroupComponent(
                &lmi_batt_container, &lmi_chassis);
        LMI_PhysicalBatteryContainer_Set_PartComponent(
                &lmi_batt_container, &lmi_batt_phys);
        LMI_PhysicalBatteryContainer_Set_LocationWithinContainer(
                &lmi_batt_container, dmi_batt[i].location);

        KReturnInstance(cr, lmi_batt_container);
    }

done:
    dmi_free_chassis(&dmi_chassis);
    dmi_free_batteries(&dmi_batt, &dmi_batt_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalBatteryContainerGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PhysicalBatteryContainerCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalBatteryContainerModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalBatteryContainerDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalBatteryContainerExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalBatteryContainerAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalBatteryContainerAssociators(
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
        LMI_PhysicalBatteryContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_PhysicalBatteryContainerAssociatorNames(
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
        LMI_PhysicalBatteryContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_PhysicalBatteryContainerReferences(
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
        LMI_PhysicalBatteryContainer_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_PhysicalBatteryContainerReferenceNames(
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
        LMI_PhysicalBatteryContainer_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_PhysicalBatteryContainer,
    LMI_PhysicalBatteryContainer,
    _cb,
    LMI_PhysicalBatteryContainerInitialize(ctx))

CMAssociationMIStub( 
    LMI_PhysicalBatteryContainer,
    LMI_PhysicalBatteryContainer,
    _cb,
    LMI_PhysicalBatteryContainerInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PhysicalBatteryContainer",
    "LMI_PhysicalBatteryContainer",
    "instance association")
