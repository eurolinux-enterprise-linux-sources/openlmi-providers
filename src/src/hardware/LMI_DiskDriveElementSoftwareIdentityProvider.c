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
#include "LMI_DiskDriveElementSoftwareIdentity.h"
#include "utils.h"
#include "lsblk.h"

static const CMPIBroker* _cb;

static void LMI_DiskDriveElementSoftwareIdentityInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_DiskDriveElementSoftwareIdentity lmi_hdd_swi_el;
    LMI_DiskDriveSoftwareIdentityRef lmi_hdd_swi;
    LMI_DiskDriveRef lmi_hdd;
    const char *ns = KNameSpace(cop);
    char instance_id[BUFLEN];
    unsigned i;
    LsblkHdd *lsblk_hdds = NULL;
    unsigned lsblk_hdds_nb = 0;

    if (lsblk_get_hdds(&lsblk_hdds, &lsblk_hdds_nb) != 0 || lsblk_hdds_nb < 1) {
        goto done;
    }

    for (i = 0; i < lsblk_hdds_nb; i++) {
        /* use only disk devices from lsblk */
        if (strcmp(lsblk_hdds[i].type, "disk") != 0) {
            continue;
        }

        LMI_DiskDriveElementSoftwareIdentity_Init(&lmi_hdd_swi_el, _cb, ns);

        LMI_DiskDriveRef_Init(&lmi_hdd, _cb, ns);
        LMI_DiskDriveRef_Set_SystemCreationClassName(&lmi_hdd,
                lmi_get_system_creation_class_name());
        LMI_DiskDriveRef_Set_SystemName(&lmi_hdd, lmi_get_system_name_safe(cc));
        LMI_DiskDriveRef_Set_CreationClassName(&lmi_hdd,
                LMI_DiskDrive_ClassName);
        LMI_DiskDriveRef_Set_DeviceID(&lmi_hdd, lsblk_hdds[i].name);

        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_DiskDriveSoftwareIdentity_ClassName ":%s",
                lsblk_hdds[i].name);

        LMI_DiskDriveSoftwareIdentityRef_Init(&lmi_hdd_swi, _cb, ns);
        LMI_DiskDriveSoftwareIdentityRef_Set_InstanceID(&lmi_hdd_swi,
                instance_id);

        LMI_DiskDriveElementSoftwareIdentity_Set_Dependent(&lmi_hdd_swi_el,
                &lmi_hdd);
        LMI_DiskDriveElementSoftwareIdentity_Set_Antecedent(&lmi_hdd_swi_el,
                &lmi_hdd_swi);

        KReturnInstance(cr, lmi_hdd_swi_el);
    }

done:
    lsblk_free_hdds(&lsblk_hdds, &lsblk_hdds_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityAssociators(
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
        LMI_DiskDriveElementSoftwareIdentity_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityAssociatorNames(
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
        LMI_DiskDriveElementSoftwareIdentity_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityReferences(
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
        LMI_DiskDriveElementSoftwareIdentity_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_DiskDriveElementSoftwareIdentityReferenceNames(
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
        LMI_DiskDriveElementSoftwareIdentity_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_DiskDriveElementSoftwareIdentity,
    LMI_DiskDriveElementSoftwareIdentity,
    _cb,
    LMI_DiskDriveElementSoftwareIdentityInitialize(ctx))

CMAssociationMIStub(
    LMI_DiskDriveElementSoftwareIdentity,
    LMI_DiskDriveElementSoftwareIdentity,
    _cb,
    LMI_DiskDriveElementSoftwareIdentityInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DiskDriveElementSoftwareIdentity",
    "LMI_DiskDriveElementSoftwareIdentity",
    "instance association")
