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
#include "LMI_SystemSlotContainer.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_SystemSlotContainerInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_SystemSlotContainerCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SystemSlotContainerEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SystemSlotContainerEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_SystemSlotContainer lmi_slot_container;
    LMI_SystemSlotRef lmi_slot;
    LMI_ChassisRef lmi_chassis;
    const char *ns = KNameSpace(cop);
    unsigned i;
    DmiChassis dmi_chassis;
    DmiSystemSlot *dmi_slots = NULL;
    unsigned dmi_slots_nb = 0;

    if (dmi_get_chassis(&dmi_chassis) != 0) {
        goto done;
    }
    if (dmi_get_system_slots(&dmi_slots, &dmi_slots_nb) != 0 || dmi_slots_nb < 1) {
        goto done;
    }

    LMI_ChassisRef_Init(&lmi_chassis, _cb, ns);
    LMI_ChassisRef_Set_CreationClassName(&lmi_chassis,
            ORGID "_" CHASSIS_CLASS_NAME);
    LMI_ChassisRef_Set_Tag(&lmi_chassis, dmi_get_chassis_tag(&dmi_chassis));

    for (i = 0; i < dmi_slots_nb; i++) {
        LMI_SystemSlotContainer_Init(&lmi_slot_container, _cb, ns);

        LMI_SystemSlotRef_Init(&lmi_slot, _cb, ns);
        LMI_SystemSlotRef_Set_CreationClassName(&lmi_slot,
                ORGID "_" SYSTEM_SLOT_CLASS_NAME);
        LMI_SystemSlotRef_Set_Tag(&lmi_slot, dmi_slots[i].name);

        LMI_SystemSlotContainer_Set_GroupComponent(
                &lmi_slot_container, &lmi_chassis);
        LMI_SystemSlotContainer_Set_PartComponent(
                &lmi_slot_container, &lmi_slot);

        KReturnInstance(cr, lmi_slot_container);
    }

done:
    dmi_free_chassis(&dmi_chassis);
    dmi_free_system_slots(&dmi_slots, &dmi_slots_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SystemSlotContainerGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SystemSlotContainerCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SystemSlotContainerModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SystemSlotContainerDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SystemSlotContainerExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SystemSlotContainerAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SystemSlotContainerAssociators(
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
        LMI_SystemSlotContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_SystemSlotContainerAssociatorNames(
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
        LMI_SystemSlotContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_SystemSlotContainerReferences(
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
        LMI_SystemSlotContainer_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_SystemSlotContainerReferenceNames(
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
        LMI_SystemSlotContainer_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_SystemSlotContainer,
    LMI_SystemSlotContainer,
    _cb,
    LMI_SystemSlotContainerInitialize(ctx))

CMAssociationMIStub( 
    LMI_SystemSlotContainer,
    LMI_SystemSlotContainer,
    _cb,
    LMI_SystemSlotContainerInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SystemSlotContainer",
    "LMI_SystemSlotContainer",
    "instance association")
