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
#include "LMI_MemorySlot.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb = NULL;

static void LMI_MemorySlotInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_MemorySlotCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemorySlotEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_MemorySlotEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_MemorySlot lmi_mem_slot;
    const char *ns = KNameSpace(cop);
    char tag[LONG_INT_LEN], instance_id[INSTANCE_ID_LEN];
    unsigned i;
    DmiMemory dmi_memory;

    if (dmi_get_memory(&dmi_memory) != 0 || dmi_memory.slots_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_memory.slots_nb; i++) {
        LMI_MemorySlot_Init(&lmi_mem_slot, _cb, ns);

        snprintf(tag, LONG_INT_LEN, "%d", dmi_memory.slots[i].slot_number);
        snprintf(instance_id, INSTANCE_ID_LEN,
                ORGID ":" ORGID "_" MEMORY_SLOT_CLASS_NAME ":%d",
                dmi_memory.slots[i].slot_number);

        LMI_MemorySlot_Set_ConnectorLayout(&lmi_mem_slot,
                LMI_MemorySlot_ConnectorLayout_Slot);
        LMI_MemorySlot_Set_Caption(&lmi_mem_slot, "Memory Slot");
        LMI_MemorySlot_Set_Description(&lmi_mem_slot,
                "This object represents one memory slot in system.");
        LMI_MemorySlot_Set_ConnectorGender(&lmi_mem_slot,
                LMI_MemorySlot_ConnectorGender_Female);

        LMI_MemorySlot_Set_CreationClassName(&lmi_mem_slot,
                ORGID "_" MEMORY_SLOT_CLASS_NAME);
        LMI_MemorySlot_Set_Tag(&lmi_mem_slot, tag);
        LMI_MemorySlot_Set_Number(&lmi_mem_slot,
                dmi_memory.slots[i].slot_number);
        LMI_MemorySlot_Set_ElementName(&lmi_mem_slot, dmi_memory.slots[i].name);
        LMI_MemorySlot_Set_Name(&lmi_mem_slot, dmi_memory.slots[i].name);
        LMI_MemorySlot_Set_InstanceID(&lmi_mem_slot, instance_id);

        KReturnInstance(cr, lmi_mem_slot);
    }

done:
    dmi_free_memory(&dmi_memory);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemorySlotGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_MemorySlotCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySlotModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySlotDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemorySlotExecQuery(
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
    LMI_MemorySlot,
    LMI_MemorySlot,
    _cb,
    LMI_MemorySlotInitialize(ctx))

static CMPIStatus LMI_MemorySlotMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemorySlotInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_MemorySlot_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_MemorySlot,
    LMI_MemorySlot,
    _cb,
    LMI_MemorySlotInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_MemorySlot",
    "LMI_MemorySlot",
    "instance method")
