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
#include "LMI_PhysicalMemoryContainer.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_PhysicalMemoryContainerInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_PhysicalMemoryContainerCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalMemoryContainerEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PhysicalMemoryContainerEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_PhysicalMemoryContainer lmi_phys_mem_container;
    LMI_MemoryPhysicalPackageRef lmi_phys_mem_pkg;
    LMI_PhysicalMemoryRef lmi_phys_mem;
    const char *ns = KNameSpace(cop);
    unsigned i;
    DmiMemory dmi_memory;

    if (dmi_get_memory(&dmi_memory) != 0 || dmi_memory.modules_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_memory.modules_nb; i++) {
        LMI_PhysicalMemoryContainer_Init(&lmi_phys_mem_container, _cb, ns);

        LMI_PhysicalMemoryRef_Init(&lmi_phys_mem, _cb, ns);
        LMI_PhysicalMemoryRef_Set_CreationClassName(&lmi_phys_mem,
                ORGID "_" PHYS_MEM_CLASS_NAME);
        LMI_PhysicalMemoryRef_Set_Tag(&lmi_phys_mem,
                dmi_memory.modules[i].serial_number);

        LMI_MemoryPhysicalPackageRef_Init(&lmi_phys_mem_pkg, _cb, ns);
        LMI_MemoryPhysicalPackageRef_Set_CreationClassName(&lmi_phys_mem_pkg,
                ORGID "_" MEMORY_PHYS_PKG_CLASS_NAME);
        LMI_MemoryPhysicalPackageRef_Set_Tag(&lmi_phys_mem_pkg,
                dmi_memory.modules[i].serial_number);

        LMI_PhysicalMemoryContainer_Set_GroupComponent(&lmi_phys_mem_container,
                &lmi_phys_mem_pkg);
        LMI_PhysicalMemoryContainer_Set_PartComponent(&lmi_phys_mem_container,
                &lmi_phys_mem);

        KReturnInstance(cr, lmi_phys_mem_container);
    }

done:
    dmi_free_memory(&dmi_memory);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalMemoryContainerGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PhysicalMemoryContainerCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryContainerModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryContainerDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryContainerExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PhysicalMemoryContainerAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PhysicalMemoryContainerAssociators(
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
        LMI_PhysicalMemoryContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_PhysicalMemoryContainerAssociatorNames(
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
        LMI_PhysicalMemoryContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_PhysicalMemoryContainerReferences(
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
        LMI_PhysicalMemoryContainer_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_PhysicalMemoryContainerReferenceNames(
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
        LMI_PhysicalMemoryContainer_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_PhysicalMemoryContainer,
    LMI_PhysicalMemoryContainer,
    _cb,
    LMI_PhysicalMemoryContainerInitialize(ctx))

CMAssociationMIStub( 
    LMI_PhysicalMemoryContainer,
    LMI_PhysicalMemoryContainer,
    _cb,
    LMI_PhysicalMemoryContainerInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PhysicalMemoryContainer",
    "LMI_PhysicalMemoryContainer",
    "instance association")
