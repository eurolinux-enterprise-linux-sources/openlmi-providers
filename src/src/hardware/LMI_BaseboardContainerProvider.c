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
#include "LMI_BaseboardContainer.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_BaseboardContainerInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_BaseboardContainerCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BaseboardContainerEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_BaseboardContainerEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_BaseboardContainer lmi_baseboard_container;
    LMI_BaseboardRef lmi_baseboard;
    LMI_ChassisRef lmi_chassis;
    const char *ns = KNameSpace(cop);
    short ret1, ret2;
    DmiChassis dmi_chassis;
    DmiBaseboard dmi_baseboard;

    /* We need to call both functions to initialize dmi_* variables */
    ret1 = dmi_get_chassis(&dmi_chassis);
    ret2 = dmi_get_baseboard(&dmi_baseboard);
    if (ret1 != 0 || ret2 != 0) {
        goto done;
    }

    LMI_ChassisRef_Init(&lmi_chassis, _cb, ns);
    LMI_ChassisRef_Set_CreationClassName(&lmi_chassis,
            ORGID "_" CHASSIS_CLASS_NAME);
    LMI_ChassisRef_Set_Tag(&lmi_chassis, dmi_get_chassis_tag(&dmi_chassis));

    LMI_BaseboardRef_Init(&lmi_baseboard, _cb, ns);
    LMI_BaseboardRef_Set_CreationClassName(&lmi_baseboard,
            ORGID "_" BASEBOARD_CLASS_NAME);
    if (strcmp(dmi_baseboard.serial_number, "Not Specified") == 0) {
        LMI_BaseboardRef_Set_Tag(&lmi_baseboard, "0");
    } else {
        LMI_BaseboardRef_Set_Tag(&lmi_baseboard, dmi_baseboard.serial_number);
    }

    LMI_BaseboardContainer_Init(&lmi_baseboard_container, _cb, ns);

    LMI_BaseboardContainer_Set_GroupComponent(&lmi_baseboard_container,
            &lmi_chassis);
    LMI_BaseboardContainer_Set_PartComponent(&lmi_baseboard_container,
            &lmi_baseboard);

    KReturnInstance(cr, lmi_baseboard_container);

done:
    dmi_free_chassis(&dmi_chassis);
    dmi_free_baseboard(&dmi_baseboard);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BaseboardContainerGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_BaseboardContainerCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BaseboardContainerModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BaseboardContainerDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BaseboardContainerExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BaseboardContainerAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BaseboardContainerAssociators(
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
        LMI_BaseboardContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_BaseboardContainerAssociatorNames(
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
        LMI_BaseboardContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_BaseboardContainerReferences(
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
        LMI_BaseboardContainer_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_BaseboardContainerReferenceNames(
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
        LMI_BaseboardContainer_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_BaseboardContainer,
    LMI_BaseboardContainer,
    _cb,
    LMI_BaseboardContainerInitialize(ctx))

CMAssociationMIStub( 
    LMI_BaseboardContainer,
    LMI_BaseboardContainer,
    _cb,
    LMI_BaseboardContainerInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_BaseboardContainer",
    "LMI_BaseboardContainer",
    "instance association")
