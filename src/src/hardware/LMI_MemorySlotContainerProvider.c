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
#include "LMI_MemorySlotContainer.h"
#include "utils.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_MemorySlotContainerInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_MemorySlotContainerCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemorySlotContainerEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_MemorySlotContainerEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_MemorySlotContainer lmi_mem_slot_container;
    LMI_MemorySlotRef lmi_mem_slot;
    LMI_ChassisRef lmi_chassis;
    const char *ns = KNameSpace(cop);
    char tag[LONG_INT_LEN];
    short ret1, ret2;
    unsigned i;
    DmiChassis dmi_chassis;
    DmiMemory dmi_memory;

    ret1 = dmi_get_chassis(&dmi_chassis);
    ret2 = dmi_get_memory(&dmi_memory);
    if (ret1 != 0 || ret2 != 0 || dmi_memory.slots_nb < 1) {
        goto done;
    }

    LMI_ChassisRef_Init(&lmi_chassis, _cb, ns);
    LMI_ChassisRef_Set_CreationClassName(&lmi_chassis,
            LMI_Chassis_ClassName);
    if (strcmp(dmi_chassis.serial_number, "Not Specified") == 0) {
        LMI_ChassisRef_Set_Tag(&lmi_chassis, "0");
    } else {
        LMI_ChassisRef_Set_Tag(&lmi_chassis, dmi_chassis.serial_number);
    }

    for (i = 0; i < dmi_memory.slots_nb; i++) {
        LMI_MemorySlotContainer_Init(&lmi_mem_slot_container, _cb, ns);

        snprintf(tag, LONG_INT_LEN, "%d", dmi_memory.slots[i].slot_number);

        LMI_MemorySlotRef_Init(&lmi_mem_slot, _cb, ns);
        LMI_MemorySlotRef_Set_CreationClassName(&lmi_mem_slot,
                LMI_MemorySlot_ClassName);
        LMI_MemorySlotRef_Set_Tag(&lmi_mem_slot, tag);

        LMI_MemorySlotContainer_Set_GroupComponent(&lmi_mem_slot_container,
                &lmi_chassis);
        LMI_MemorySlotContainer_Set_PartComponent(&lmi_mem_slot_container,
                &lmi_mem_slot);

        KReturnInstance(cr, lmi_mem_slot_container);
    }

done:
    dmi_free_chassis(&dmi_chassis);
    dmi_free_memory(&dmi_memory);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemorySlotContainerGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_MemorySlotContainerCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySlotContainerModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySlotContainerDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySlotContainerExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySlotContainerAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemorySlotContainerAssociators(
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
        LMI_MemorySlotContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_MemorySlotContainerAssociatorNames(
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
        LMI_MemorySlotContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_MemorySlotContainerReferences(
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
        LMI_MemorySlotContainer_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_MemorySlotContainerReferenceNames(
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
        LMI_MemorySlotContainer_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_MemorySlotContainer,
    LMI_MemorySlotContainer,
    _cb,
    LMI_MemorySlotContainerInitialize(ctx))

CMAssociationMIStub(
    LMI_MemorySlotContainer,
    LMI_MemorySlotContainer,
    _cb,
    LMI_MemorySlotContainerInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_MemorySlotContainer",
    "LMI_MemorySlotContainer",
    "instance association")
