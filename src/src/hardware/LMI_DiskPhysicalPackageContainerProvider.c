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
#include "LMI_DiskPhysicalPackageContainer.h"
#include "utils.h"
#include "dmidecode.h"
#include "lsblk.h"

static const CMPIBroker* _cb;

static void LMI_DiskPhysicalPackageContainerInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_DiskPhysicalPackageContainer lmi_hdd_container;
    LMI_DiskPhysicalPackageRef lmi_hdd;
    LMI_ChassisRef lmi_chassis;
    const char *ns = KNameSpace(cop);
    unsigned i;
    DmiChassis dmi_chassis;
    LsblkHdd *lsblk_hdds = NULL;
    unsigned lsblk_hdds_nb = 0;

    if (dmi_get_chassis(&dmi_chassis) != 0) {
        goto done;
    }
    if (lsblk_get_hdds(&lsblk_hdds, &lsblk_hdds_nb) != 0 || lsblk_hdds_nb < 1) {
        goto done;
    }

    LMI_ChassisRef_Init(&lmi_chassis, _cb, ns);
    LMI_ChassisRef_Set_CreationClassName(&lmi_chassis,
            LMI_Chassis_ClassName);
    LMI_ChassisRef_Set_Tag(&lmi_chassis, dmi_get_chassis_tag(&dmi_chassis));

    for (i = 0; i < lsblk_hdds_nb; i++) {
        /* use only disk devices from lsblk */
        if (strcmp(lsblk_hdds[i].type, "disk") != 0) {
            continue;
        }

        LMI_DiskPhysicalPackageContainer_Init(&lmi_hdd_container, _cb, ns);

        LMI_DiskPhysicalPackageRef_Init(&lmi_hdd, _cb, ns);
        LMI_DiskPhysicalPackageRef_Set_CreationClassName(&lmi_hdd,
                LMI_DiskPhysicalPackage_ClassName);
        LMI_DiskPhysicalPackageRef_Set_Tag(&lmi_hdd, lsblk_hdds[i].name);

        LMI_DiskPhysicalPackageContainer_Set_GroupComponent(&lmi_hdd_container,
                &lmi_chassis);
        LMI_DiskPhysicalPackageContainer_Set_PartComponent(&lmi_hdd_container,
                &lmi_hdd);

        KReturnInstance(cr, lmi_hdd_container);
    }

done:
    dmi_free_chassis(&dmi_chassis);
    lsblk_free_hdds(&lsblk_hdds, &lsblk_hdds_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerAssociators(
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
        LMI_DiskPhysicalPackageContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerAssociatorNames(
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
        LMI_DiskPhysicalPackageContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerReferences(
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
        LMI_DiskPhysicalPackageContainer_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_DiskPhysicalPackageContainerReferenceNames(
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
        LMI_DiskPhysicalPackageContainer_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_DiskPhysicalPackageContainer,
    LMI_DiskPhysicalPackageContainer,
    _cb,
    LMI_DiskPhysicalPackageContainerInitialize(ctx))

CMAssociationMIStub(
    LMI_DiskPhysicalPackageContainer,
    LMI_DiskPhysicalPackageContainer,
    _cb,
    LMI_DiskPhysicalPackageContainerInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DiskPhysicalPackageContainer",
    "LMI_DiskPhysicalPackageContainer",
    "instance association")
