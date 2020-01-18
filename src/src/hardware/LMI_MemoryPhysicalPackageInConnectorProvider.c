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
#include "LMI_MemoryPhysicalPackageInConnector.h"
#include "utils.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_MemoryPhysicalPackageInConnectorInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_MemoryPhysicalPackageInConnector lmi_phys_mem_pkg_in_conn;
    LMI_MemoryPhysicalPackageRef lmi_phys_mem_pkg;
    LMI_MemorySlotRef lmi_mem_slot;
    const char *ns = KNameSpace(cop);
    char tag[LONG_INT_LEN];
    unsigned i, j;
    DmiMemory dmi_memory;

    if (dmi_get_memory(&dmi_memory) != 0 || dmi_memory.modules_nb < 1 ||
            dmi_memory.slots_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_memory.modules_nb; i++) {
        LMI_MemoryPhysicalPackageInConnector_Init(&lmi_phys_mem_pkg_in_conn,
                _cb, ns);

        LMI_MemoryPhysicalPackageRef_Init(&lmi_phys_mem_pkg, _cb, ns);
        LMI_MemoryPhysicalPackageRef_Set_CreationClassName(&lmi_phys_mem_pkg,
                LMI_MemoryPhysicalPackage_ClassName);
        LMI_MemoryPhysicalPackageRef_Set_Tag(&lmi_phys_mem_pkg,
                dmi_memory.modules[i].serial_number);

        for (j = 0; j < dmi_memory.slots_nb; j++) {
            if (dmi_memory.modules[i].slot == dmi_memory.slots[j].slot_number) {
                snprintf(tag, LONG_INT_LEN, "%d",
                        dmi_memory.slots[j].slot_number);
                LMI_MemorySlotRef_Init(&lmi_mem_slot, _cb, ns);
                LMI_MemorySlotRef_Set_CreationClassName(&lmi_mem_slot,
                        LMI_MemorySlot_ClassName);
                LMI_MemorySlotRef_Set_Tag(&lmi_mem_slot, tag);

                LMI_MemoryPhysicalPackageInConnector_Set_Antecedent(
                        &lmi_phys_mem_pkg_in_conn, &lmi_mem_slot);
                LMI_MemoryPhysicalPackageInConnector_Set_Dependent(
                        &lmi_phys_mem_pkg_in_conn, &lmi_phys_mem_pkg);

                KReturnInstance(cr, lmi_phys_mem_pkg_in_conn);
                break;
            }
        }
    }

done:
    dmi_free_memory(&dmi_memory);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorAssociators(
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
        LMI_MemoryPhysicalPackageInConnector_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorAssociatorNames(
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
        LMI_MemoryPhysicalPackageInConnector_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorReferences(
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
        LMI_MemoryPhysicalPackageInConnector_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_MemoryPhysicalPackageInConnectorReferenceNames(
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
        LMI_MemoryPhysicalPackageInConnector_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_MemoryPhysicalPackageInConnector,
    LMI_MemoryPhysicalPackageInConnector,
    _cb,
    LMI_MemoryPhysicalPackageInConnectorInitialize(ctx))

CMAssociationMIStub(
    LMI_MemoryPhysicalPackageInConnector,
    LMI_MemoryPhysicalPackageInConnector,
    _cb,
    LMI_MemoryPhysicalPackageInConnectorInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_MemoryPhysicalPackageInConnector",
    "LMI_MemoryPhysicalPackageInConnector",
    "instance association")
