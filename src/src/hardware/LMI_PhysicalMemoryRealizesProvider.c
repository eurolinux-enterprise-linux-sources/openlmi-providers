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
#include "LMI_PhysicalMemoryRealizes.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_PhysicalMemoryRealizesInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_PhysicalMemoryRealizesCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalMemoryRealizesEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PhysicalMemoryRealizesEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_PhysicalMemoryRealizes lmi_phys_mem_realizes;
    LMI_PhysicalMemoryRef lmi_phys_mem;
    LMI_MemoryRef lmi_mem;
    const char *ns = KNameSpace(cop);
    unsigned i;
    DmiMemory dmi_memory;

    if (dmi_get_memory(&dmi_memory) != 0 || dmi_memory.modules_nb < 1) {
        goto done;
    }

    LMI_MemoryRef_Init(&lmi_mem, _cb, ns);
    LMI_MemoryRef_Set_SystemCreationClassName(&lmi_mem,
            get_system_creation_class_name());
    LMI_MemoryRef_Set_SystemName(&lmi_mem, get_system_name());
    LMI_MemoryRef_Set_CreationClassName(&lmi_mem, ORGID "_" MEM_CLASS_NAME);
    LMI_MemoryRef_Set_DeviceID(&lmi_mem, "0");

    for (i = 0; i < dmi_memory.modules_nb; i++) {
        LMI_PhysicalMemoryRealizes_Init(&lmi_phys_mem_realizes, _cb, ns);

        LMI_PhysicalMemoryRef_Init(&lmi_phys_mem, _cb, ns);
        LMI_PhysicalMemoryRef_Set_CreationClassName(&lmi_phys_mem,
                ORGID "_" PHYS_MEM_CLASS_NAME);
        LMI_PhysicalMemoryRef_Set_Tag(&lmi_phys_mem,
                dmi_memory.modules[i].serial_number);

        LMI_PhysicalMemoryRealizes_Set_Antecedent(&lmi_phys_mem_realizes,
                &lmi_phys_mem);
        LMI_PhysicalMemoryRealizes_Set_Dependent(&lmi_phys_mem_realizes,
                &lmi_mem);

        KReturnInstance(cr, lmi_phys_mem_realizes);
    }

done:
    dmi_free_memory(&dmi_memory);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalMemoryRealizesGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PhysicalMemoryRealizesCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryRealizesModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryRealizesDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryRealizesExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryRealizesAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalMemoryRealizesAssociators(
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
        LMI_PhysicalMemoryRealizes_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_PhysicalMemoryRealizesAssociatorNames(
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
        LMI_PhysicalMemoryRealizes_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_PhysicalMemoryRealizesReferences(
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
        LMI_PhysicalMemoryRealizes_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_PhysicalMemoryRealizesReferenceNames(
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
        LMI_PhysicalMemoryRealizes_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_PhysicalMemoryRealizes,
    LMI_PhysicalMemoryRealizes,
    _cb,
    LMI_PhysicalMemoryRealizesInitialize(ctx))

CMAssociationMIStub( 
    LMI_PhysicalMemoryRealizes,
    LMI_PhysicalMemoryRealizes,
    _cb,
    LMI_PhysicalMemoryRealizesInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PhysicalMemoryRealizes",
    "LMI_PhysicalMemoryRealizes",
    "instance association")
