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
#include "LMI_MemoryPhysicalPackage.h"
#include "utils.h"
#include "dmidecode.h"

static const CMPIBroker* _cb = NULL;

static void LMI_MemoryPhysicalPackageInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_MemoryPhysicalPackageCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemoryPhysicalPackageEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_MemoryPhysicalPackageEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_MemoryPhysicalPackage lmi_phys_mem_pkg;
    const char *ns = KNameSpace(cop);
    char instance_id[BUFLEN];
    unsigned i;
    DmiMemory dmi_memory;

    if (dmi_get_memory(&dmi_memory) != 0 || dmi_memory.modules_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_memory.modules_nb; i++) {
        LMI_MemoryPhysicalPackage_Init(&lmi_phys_mem_pkg, _cb, ns);

        LMI_MemoryPhysicalPackage_Set_CreationClassName(&lmi_phys_mem_pkg,
                LMI_MemoryPhysicalPackage_ClassName);
        LMI_MemoryPhysicalPackage_Set_PackageType(&lmi_phys_mem_pkg,
                LMI_MemoryPhysicalPackage_PackageType_Memory);
        LMI_MemoryPhysicalPackage_Set_Caption(&lmi_phys_mem_pkg,
                "Physical Memory Package");
        LMI_MemoryPhysicalPackage_Set_Description(&lmi_phys_mem_pkg,
                "This object represents one physical memory package in system.");

        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_MemoryPhysicalPackage_ClassName ":%s",
                dmi_memory.modules[i].serial_number);

        LMI_MemoryPhysicalPackage_Set_Tag(&lmi_phys_mem_pkg,
                dmi_memory.modules[i].serial_number);
        LMI_MemoryPhysicalPackage_Set_ElementName(&lmi_phys_mem_pkg,
                dmi_memory.modules[i].name);
        LMI_MemoryPhysicalPackage_Set_Name(&lmi_phys_mem_pkg,
                dmi_memory.modules[i].name);
        LMI_MemoryPhysicalPackage_Set_Manufacturer(&lmi_phys_mem_pkg,
                dmi_memory.modules[i].manufacturer);
        LMI_MemoryPhysicalPackage_Set_SerialNumber(&lmi_phys_mem_pkg,
                dmi_memory.modules[i].serial_number);
        LMI_MemoryPhysicalPackage_Set_PartNumber(&lmi_phys_mem_pkg,
                dmi_memory.modules[i].part_number);
        LMI_MemoryPhysicalPackage_Set_InstanceID(&lmi_phys_mem_pkg,
                instance_id);

        KReturnInstance(cr, lmi_phys_mem_pkg);
    }

done:
    dmi_free_memory(&dmi_memory);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemoryPhysicalPackageGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_MemoryPhysicalPackageCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemoryPhysicalPackageModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemoryPhysicalPackageDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemoryPhysicalPackageExecQuery(
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
    LMI_MemoryPhysicalPackage,
    LMI_MemoryPhysicalPackage,
    _cb,
    LMI_MemoryPhysicalPackageInitialize(ctx))

static CMPIStatus LMI_MemoryPhysicalPackageMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemoryPhysicalPackageInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_MemoryPhysicalPackage_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_MemoryPhysicalPackage,
    LMI_MemoryPhysicalPackage,
    _cb,
    LMI_MemoryPhysicalPackageInitialize(ctx))

KUint32 LMI_MemoryPhysicalPackage_IsCompatible(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_MemoryPhysicalPackageRef* self,
    const KRef* ElementToCheck,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_MemoryPhysicalPackage",
    "LMI_MemoryPhysicalPackage",
    "instance method")
