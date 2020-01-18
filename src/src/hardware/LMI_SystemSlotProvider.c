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
#include "LMI_SystemSlot.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

CMPIUint16 get_connectorlayout_slot(const char *dmi_val);
CMPIUint16 get_maxlinkwidth(const char *dmi_val);

static const CMPIBroker* _cb = NULL;

static void LMI_SystemSlotInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_SystemSlotCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SystemSlotEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SystemSlotEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_SystemSlot lmi_slot;
    const char *ns = KNameSpace(cop);
    CMPIUint16 conn_layout, maxlinkwidth;
    char instance_id[INSTANCE_ID_LEN];
    unsigned i;
    DmiSystemSlot *dmi_slots = NULL;
    unsigned dmi_slots_nb = 0;

    if (dmi_get_system_slots(&dmi_slots, &dmi_slots_nb) != 0 || dmi_slots_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_slots_nb; i++) {
        LMI_SystemSlot_Init(&lmi_slot, _cb, ns);

        LMI_SystemSlot_Set_CreationClassName(&lmi_slot,
                ORGID "_" SYSTEM_SLOT_CLASS_NAME);
        LMI_SystemSlot_Set_Caption(&lmi_slot, "System Slot");
        LMI_SystemSlot_Set_Description(&lmi_slot,
                "This object represents one system slot.");
        LMI_SystemSlot_Set_ConnectorGender(&lmi_slot,
                LMI_SystemSlot_ConnectorGender_Female);

        snprintf(instance_id, INSTANCE_ID_LEN,
                ORGID ":" ORGID "_" SYSTEM_SLOT_CLASS_NAME ":%s",
                dmi_slots[i].name);
        conn_layout = get_connectorlayout_slot(dmi_slots[i].type);
        maxlinkwidth = get_maxlinkwidth(dmi_slots[i].link_width);

        LMI_SystemSlot_Set_Tag(&lmi_slot, dmi_slots[i].name);
        LMI_SystemSlot_Set_Number(&lmi_slot, dmi_slots[i].number);
        LMI_SystemSlot_Set_ConnectorLayout(&lmi_slot, conn_layout);
        LMI_SystemSlot_Set_ElementName(&lmi_slot, dmi_slots[i].name);
        LMI_SystemSlot_Set_Name(&lmi_slot, dmi_slots[i].name);
        LMI_SystemSlot_Set_InstanceID(&lmi_slot, instance_id);

        if (conn_layout == LMI_SystemSlot_ConnectorLayout_Other) {
            if (strcmp(dmi_slots[i].type, "Other") != 0) {
                LMI_SystemSlot_Set_ConnectorDescription(&lmi_slot,
                        dmi_slots[i].type);
            } else {
                LMI_SystemSlot_Set_ConnectorDescription(&lmi_slot,
                        dmi_slots[i].name);
            }
        }
        if (dmi_slots[i].data_width) {
            LMI_SystemSlot_Set_MaxDataWidth(&lmi_slot, dmi_slots[i].data_width);
        }
        if (maxlinkwidth) {
            LMI_SystemSlot_Set_MaxLinkWidth(&lmi_slot, maxlinkwidth);
        }
        if (dmi_slots[i].supports_hotplug) {
            LMI_SystemSlot_Set_SupportsHotPlug(&lmi_slot,
                    dmi_slots[i].supports_hotplug);
        }

        KReturnInstance(cr, lmi_slot);
    }

done:
    dmi_free_system_slots(&dmi_slots, &dmi_slots_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SystemSlotGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SystemSlotCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SystemSlotModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SystemSlotDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SystemSlotExecQuery(
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
    LMI_SystemSlot,
    LMI_SystemSlot,
    _cb,
    LMI_SystemSlotInitialize(ctx))

static CMPIStatus LMI_SystemSlotMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SystemSlotInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SystemSlot_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

/*
 * Get max link width according to the dmidecode.
 * @param dmi_val from dmidecode
 * @return CIM id of max link width
 */
CMPIUint16 get_maxlinkwidth(const char *dmi_val)
{
    if (!dmi_val || !strlen(dmi_val)) {
        return 0; /* Unknown */
    }

    static struct {
        CMPIUint16 cim_val;     /* CIM value */
        char *dmi_val;          /* dmidecode value */
    } values[] = {
        {0, "Unknown"},
        {2, "x1"},
        {3, "x2"},
        {4, "x4"},
        {5, "x8"},
        {6, "x12"},
        {7, "x16"},
        {8, "x32"},
    };

    size_t i, val_length = sizeof(values) / sizeof(values[0]);

    for (i = 0; i < val_length; i++) {
        if (strcmp(dmi_val, values[i].dmi_val) == 0) {
            return values[i].cim_val;
        }
    }

    return 0; /* Unknown */
}

/*
 * Get connector layout for slot according to the dmidecode.
 * @param dmi_val from dmidecode
 * @return CIM id of connector layout
 */
CMPIUint16 get_connectorlayout_slot(const char *dmi_val)
{
    if (!dmi_val || !strlen(dmi_val)) {
        return 0; /* Unknown */
    }

    static struct {
        CMPIUint16 cim_val;     /* CIM value */
        char *dmi_val;          /* dmidecode value */
    } values[] = {
        {0,  "Unknown"},
        {1,  "Other"},
        /*
        {2,  "RS232"},
        {3,  "BNC"},
        {4,  "RJ11"},
        {5,  "RJ45"},
        {6,  "DB9"},
        {7,  "Slot"},
        {8,  "SCSI High Density"},
        {9,  "SCSI Low Density"},
        {10, "Ribbon"},
        {11, "AUI"},
        {12, "Fiber SC"},
        {13, "Fiber ST"},
        {14, "FDDI-MIC"},
        {15, "Fiber-RTMJ"},
        */
        {16, "PCI"},
        {17, "PCI-X"},
        {18, "PCI Express"},
        {18, "PCI Express 2"},
        {18, "PCI Express 3"},
        {19, "PCI Express x1"},
        {19, "PCI Express 2 x1"},
        {19, "PCI Express 3 x1"},
        {20, "PCI Express x2"},
        {20, "PCI Express 2 x2"},
        {20, "PCI Express 3 x2"},
        {21, "PCI Express x4"},
        {21, "PCI Express 2 x4"},
        {21, "PCI Express 3 x4"},
        {22, "PCI Express x8"},
        {22, "PCI Express 2 x8"},
        {22, "PCI Express 3 x8"},
        {23, "PCI Express x16"},
        {23, "PCI Express 2 x16"},
        {23, "PCI Express 3 x16"},
        /*
        {24, "PCI Express x32"},
        {24, "PCI Express 2 x32"},
        {24, "PCI Express 3 x32"},
        {25, "PCI Express x64"},
        {25, "PCI Express 2 x64"},
        {25, "PCI Express 3 x64"},
        */
    };

    size_t i, val_length = sizeof(values) / sizeof(values[0]);

    for (i = 0; i < val_length; i++) {
        if (strcmp(dmi_val, values[i].dmi_val) == 0) {
            return values[i].cim_val;
        }
    }

    return 1; /* Other */
}

CMMethodMIStub(
    LMI_SystemSlot,
    LMI_SystemSlot,
    _cb,
    LMI_SystemSlotInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SystemSlot",
    "LMI_SystemSlot",
    "instance method")
