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
#include "LMI_DiskDriveDeviceSAPImplementation.h"
#include "utils.h"
#include "lsblk.h"

static const CMPIBroker* _cb;

static void LMI_DiskDriveDeviceSAPImplementationInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_DiskDriveDeviceSAPImplementation lmi_hdd_sap_impl;
    LMI_DiskDriveATAProtocolEndpointRef lmi_hdd_ata_endp;
    LMI_DiskDriveATAPortRef lmi_hdd_ata_port;
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

        LMI_DiskDriveDeviceSAPImplementation_Init(&lmi_hdd_sap_impl, _cb, ns);

        snprintf(name, BUFLEN,
                "%s " LMI_DiskDriveATAPort_ClassName,
                lsblk_hdds[i].name);

        LMI_DiskDriveATAPortRef_Init(&lmi_hdd_ata_port, _cb, ns);
        LMI_DiskDriveATAPortRef_Set_SystemCreationClassName(&lmi_hdd_ata_port,
                lmi_get_system_creation_class_name());
        LMI_DiskDriveATAPortRef_Set_SystemName(&lmi_hdd_ata_port,
                lmi_get_system_name_safe(cc));
        LMI_DiskDriveATAPortRef_Set_CreationClassName(&lmi_hdd_ata_port,
                LMI_DiskDriveATAPort_ClassName);
        LMI_DiskDriveATAPortRef_Set_DeviceID(&lmi_hdd_ata_port, name);

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

        LMI_DiskDriveDeviceSAPImplementation_Set_Antecedent(&lmi_hdd_sap_impl,
                &lmi_hdd_ata_port);
        LMI_DiskDriveDeviceSAPImplementation_Set_Dependent(&lmi_hdd_sap_impl,
                &lmi_hdd_ata_endp);

        KReturnInstance(cr, lmi_hdd_sap_impl);
    }

done:
    lsblk_free_hdds(&lsblk_hdds, &lsblk_hdds_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationAssociators(
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
        LMI_DiskDriveDeviceSAPImplementation_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationAssociatorNames(
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
        LMI_DiskDriveDeviceSAPImplementation_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationReferences(
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
        LMI_DiskDriveDeviceSAPImplementation_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_DiskDriveDeviceSAPImplementationReferenceNames(
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
        LMI_DiskDriveDeviceSAPImplementation_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_DiskDriveDeviceSAPImplementation,
    LMI_DiskDriveDeviceSAPImplementation,
    _cb,
    LMI_DiskDriveDeviceSAPImplementationInitialize(ctx))

CMAssociationMIStub(
    LMI_DiskDriveDeviceSAPImplementation,
    LMI_DiskDriveDeviceSAPImplementation,
    _cb,
    LMI_DiskDriveDeviceSAPImplementationInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DiskDriveDeviceSAPImplementation",
    "LMI_DiskDriveDeviceSAPImplementation",
    "instance association")
