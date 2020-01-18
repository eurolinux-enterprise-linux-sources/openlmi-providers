/*
 * Copyright (C) 2014 Red Hat, Inc. All rights reserved.
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
#include "LMI_DiskDriveSAPAvailableForElement.h"
#include "utils.h"
#include "lsblk.h"

static const CMPIBroker* _cb;

static void LMI_DiskDriveSAPAvailableForElementInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_DiskDriveSAPAvailableForElement lmi_hdd_sap;
    LMI_DiskDriveATAProtocolEndpointRef lmi_hdd_ata_endp;
    LMI_DiskDriveRef lmi_hdd;
    const char *ns = KNameSpace(cop);
    char name[BUFLEN];
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

        LMI_DiskDriveSAPAvailableForElement_Init(&lmi_hdd_sap, _cb, ns);

        LMI_DiskDriveRef_Init(&lmi_hdd, _cb, ns);
        LMI_DiskDriveRef_Set_SystemCreationClassName(&lmi_hdd,
                lmi_get_system_creation_class_name());
        LMI_DiskDriveRef_Set_SystemName(&lmi_hdd, lmi_get_system_name_safe(cc));
        LMI_DiskDriveRef_Set_CreationClassName(&lmi_hdd,
                LMI_DiskDrive_ClassName);
        LMI_DiskDriveRef_Set_DeviceID(&lmi_hdd, lsblk_hdds[i].name);

        snprintf(name, BUFLEN,
                "%s " LMI_DiskDriveATAProtocolEndpoint_ClassName,
                lsblk_hdds[i].name);

        LMI_DiskDriveATAProtocolEndpointRef_Init(&lmi_hdd_ata_endp, _cb, ns);
        LMI_DiskDriveATAProtocolEndpointRef_Set_SystemCreationClassName(
                &lmi_hdd_ata_endp, lmi_get_system_creation_class_name());
        LMI_DiskDriveATAProtocolEndpointRef_Set_SystemName(&lmi_hdd_ata_endp,
                lmi_get_system_name_safe(cc));
        LMI_DiskDriveATAProtocolEndpointRef_Set_CreationClassName(&lmi_hdd_ata_endp,
                LMI_DiskDriveATAProtocolEndpoint_ClassName);
        LMI_DiskDriveATAProtocolEndpointRef_Set_Name(&lmi_hdd_ata_endp, name);

        LMI_DiskDriveSAPAvailableForElement_Set_ManagedElement(&lmi_hdd_sap,
                &lmi_hdd);
        LMI_DiskDriveSAPAvailableForElement_Set_AvailableSAP(&lmi_hdd_sap,
                &lmi_hdd_ata_endp);

        KReturnInstance(cr, lmi_hdd_sap);
    }

done:
    lsblk_free_hdds(&lsblk_hdds, &lsblk_hdds_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementAssociators(
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
        LMI_DiskDriveSAPAvailableForElement_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementAssociatorNames(
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
        LMI_DiskDriveSAPAvailableForElement_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementReferences(
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
        LMI_DiskDriveSAPAvailableForElement_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_DiskDriveSAPAvailableForElementReferenceNames(
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
        LMI_DiskDriveSAPAvailableForElement_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_DiskDriveSAPAvailableForElement,
    LMI_DiskDriveSAPAvailableForElement,
    _cb,
    LMI_DiskDriveSAPAvailableForElementInitialize(ctx))

CMAssociationMIStub(
    LMI_DiskDriveSAPAvailableForElement,
    LMI_DiskDriveSAPAvailableForElement,
    _cb,
    LMI_DiskDriveSAPAvailableForElementInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DiskDriveSAPAvailableForElement",
    "LMI_DiskDriveSAPAvailableForElement",
    "instance association")
