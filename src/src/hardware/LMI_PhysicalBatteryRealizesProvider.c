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
#include "LMI_PhysicalBatteryRealizes.h"
#include "utils.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_PhysicalBatteryRealizesInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_PhysicalBatteryRealizesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalBatteryRealizesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PhysicalBatteryRealizesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_PhysicalBatteryRealizes lmi_phys_batt_realizes;
    LMI_BatteryPhysicalPackageRef lmi_batt_phys;
    LMI_BatteryRef lmi_batt;
    const char *ns = KNameSpace(cop);
    unsigned i;
    DmiBattery *dmi_batt = NULL;
    unsigned dmi_batt_nb = 0;

    if (dmi_get_batteries(&dmi_batt, &dmi_batt_nb) != 0 || dmi_batt_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_batt_nb; i++) {
        LMI_PhysicalBatteryRealizes_Init(&lmi_phys_batt_realizes, _cb, ns);

        LMI_BatteryRef_Init(&lmi_batt, _cb, ns);
        LMI_BatteryRef_Set_SystemCreationClassName(&lmi_batt,
                lmi_get_system_creation_class_name());
        LMI_BatteryRef_Set_SystemName(&lmi_batt, lmi_get_system_name_safe(cc));
        LMI_BatteryRef_Set_CreationClassName(&lmi_batt,
                LMI_Battery_ClassName);
        LMI_BatteryRef_Set_DeviceID(&lmi_batt, dmi_batt[i].name);

        LMI_BatteryPhysicalPackageRef_Init(&lmi_batt_phys, _cb, ns);
        LMI_BatteryPhysicalPackageRef_Set_CreationClassName(&lmi_batt_phys,
                LMI_BatteryPhysicalPackage_ClassName);
        LMI_BatteryPhysicalPackageRef_Set_Tag(&lmi_batt_phys, dmi_batt[i].name);

        LMI_PhysicalBatteryRealizes_Set_Antecedent(&lmi_phys_batt_realizes,
                &lmi_batt_phys);
        LMI_PhysicalBatteryRealizes_Set_Dependent(&lmi_phys_batt_realizes,
                &lmi_batt);

        KReturnInstance(cr, lmi_phys_batt_realizes);
    }

done:
    dmi_free_batteries(&dmi_batt, &dmi_batt_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalBatteryRealizesGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PhysicalBatteryRealizesCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalBatteryRealizesModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalBatteryRealizesDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalBatteryRealizesExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalBatteryRealizesAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalBatteryRealizesAssociators(
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
        LMI_PhysicalBatteryRealizes_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_PhysicalBatteryRealizesAssociatorNames(
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
        LMI_PhysicalBatteryRealizes_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_PhysicalBatteryRealizesReferences(
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
        LMI_PhysicalBatteryRealizes_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_PhysicalBatteryRealizesReferenceNames(
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
        LMI_PhysicalBatteryRealizes_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_PhysicalBatteryRealizes,
    LMI_PhysicalBatteryRealizes,
    _cb,
    LMI_PhysicalBatteryRealizesInitialize(ctx))

CMAssociationMIStub(
    LMI_PhysicalBatteryRealizes,
    LMI_PhysicalBatteryRealizes,
    _cb,
    LMI_PhysicalBatteryRealizesInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PhysicalBatteryRealizes",
    "LMI_PhysicalBatteryRealizes",
    "instance association")
