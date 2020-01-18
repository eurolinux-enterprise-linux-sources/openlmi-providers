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
#include "LMI_ChassisComputerSystemPackage.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_ChassisComputerSystemPackageInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_ChassisComputerSystemPackageCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ChassisComputerSystemPackageEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ChassisComputerSystemPackageEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_ChassisComputerSystemPackage lmi_chassis_cs_pkg;
    LMI_ChassisRef lmi_chassis;
    const char *ns = KNameSpace(cop);
    DmiChassis dmi_chassis;

    if (dmi_get_chassis(&dmi_chassis) != 0) {
        goto done;
    }

    LMI_ChassisComputerSystemPackage_Init(&lmi_chassis_cs_pkg, _cb, ns);

    LMI_ChassisRef_Init(&lmi_chassis, _cb, ns);
    LMI_ChassisRef_Set_CreationClassName(&lmi_chassis,
            ORGID "_" CHASSIS_CLASS_NAME);
    LMI_ChassisRef_Set_Tag(&lmi_chassis, dmi_get_chassis_tag(&dmi_chassis));

    LMI_ChassisComputerSystemPackage_SetObjectPath_Dependent(
            &lmi_chassis_cs_pkg, lmi_get_computer_system());
    LMI_ChassisComputerSystemPackage_Set_Antecedent(&lmi_chassis_cs_pkg,
            &lmi_chassis);
    LMI_ChassisComputerSystemPackage_Set_PlatformGUID(&lmi_chassis_cs_pkg, "0");

    KReturnInstance(cr, lmi_chassis_cs_pkg);

done:
    dmi_free_chassis(&dmi_chassis);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ChassisComputerSystemPackageGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ChassisComputerSystemPackageCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ChassisComputerSystemPackageModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ChassisComputerSystemPackageDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ChassisComputerSystemPackageExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ChassisComputerSystemPackageAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ChassisComputerSystemPackageAssociators(
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
        LMI_ChassisComputerSystemPackage_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_ChassisComputerSystemPackageAssociatorNames(
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
        LMI_ChassisComputerSystemPackage_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_ChassisComputerSystemPackageReferences(
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
        LMI_ChassisComputerSystemPackage_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_ChassisComputerSystemPackageReferenceNames(
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
        LMI_ChassisComputerSystemPackage_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_ChassisComputerSystemPackage,
    LMI_ChassisComputerSystemPackage,
    _cb,
    LMI_ChassisComputerSystemPackageInitialize(ctx))

CMAssociationMIStub( 
    LMI_ChassisComputerSystemPackage,
    LMI_ChassisComputerSystemPackage,
    _cb,
    LMI_ChassisComputerSystemPackageInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ChassisComputerSystemPackage",
    "LMI_ChassisComputerSystemPackage",
    "instance association")
