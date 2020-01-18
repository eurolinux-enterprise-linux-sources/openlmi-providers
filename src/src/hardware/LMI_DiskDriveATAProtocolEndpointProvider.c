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
#include "LMI_DiskDriveATAProtocolEndpoint.h"
#include "utils.h"
#include "lsblk.h"

static const CMPIBroker* _cb = NULL;

static void LMI_DiskDriveATAProtocolEndpointInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_DiskDriveATAProtocolEndpointCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveATAProtocolEndpointEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_DiskDriveATAProtocolEndpointEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_DiskDriveATAProtocolEndpoint lmi_hdd_endpoint;
    const char *ns = KNameSpace(cop);
    unsigned i;
    char instance_id[BUFLEN], name[BUFLEN];
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

        LMI_DiskDriveATAProtocolEndpoint_Init(&lmi_hdd_endpoint, _cb, ns);

        LMI_DiskDriveATAProtocolEndpoint_Set_SystemCreationClassName(
                &lmi_hdd_endpoint, lmi_get_system_creation_class_name());
        LMI_DiskDriveATAProtocolEndpoint_Set_SystemName(&lmi_hdd_endpoint,
                lmi_get_system_name_safe(cc));
        LMI_DiskDriveATAProtocolEndpoint_Set_CreationClassName(&lmi_hdd_endpoint,
                LMI_DiskDriveATAProtocolEndpoint_ClassName);
        LMI_DiskDriveATAProtocolEndpoint_Set_Caption(&lmi_hdd_endpoint,
                "Disk Drive ATA Protocol Endpoint");
        LMI_DiskDriveATAProtocolEndpoint_Set_Description(&lmi_hdd_endpoint,
                "This object represents ATA Protocol Endpoint of disk drive in system.");

        LMI_DiskDriveATAProtocolEndpoint_Set_Role(&lmi_hdd_endpoint,
                LMI_DiskDriveATAProtocolEndpoint_Role_Target);
        LMI_DiskDriveATAProtocolEndpoint_Set_ProtocolIFType(&lmi_hdd_endpoint,
                LMI_DiskDriveATAProtocolEndpoint_ProtocolIFType_Unknown);

        snprintf(name, BUFLEN,
                "%s " LMI_DiskDriveATAProtocolEndpoint_ClassName,
                lsblk_hdds[i].name);
        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_DiskDriveATAProtocolEndpoint_ClassName ":%s",
                name);

        LMI_DiskDriveATAProtocolEndpoint_Set_Name(&lmi_hdd_endpoint, name);
        LMI_DiskDriveATAProtocolEndpoint_Set_ElementName(&lmi_hdd_endpoint,
                name);
        LMI_DiskDriveATAProtocolEndpoint_Set_InstanceID(&lmi_hdd_endpoint,
                instance_id);

        if (lsblk_hdds[i].basename[0] == 'h') {
            LMI_DiskDriveATAProtocolEndpoint_Set_ConnectionType(&lmi_hdd_endpoint,
                    LMI_DiskDriveATAProtocolEndpoint_ConnectionType_ATA);
        } else if (lsblk_hdds[i].basename[0] == 's') {
            /* TODO: What about SAS? */
            LMI_DiskDriveATAProtocolEndpoint_Set_ConnectionType(&lmi_hdd_endpoint,
                    LMI_DiskDriveATAProtocolEndpoint_ConnectionType_SATA);
        } else {
            LMI_DiskDriveATAProtocolEndpoint_Set_ConnectionType(&lmi_hdd_endpoint,
                    LMI_DiskDriveATAProtocolEndpoint_ConnectionType_Other);
        }

        KReturnInstance(cr, lmi_hdd_endpoint);
    }

done:
    lsblk_free_hdds(&lsblk_hdds, &lsblk_hdds_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveATAProtocolEndpointGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_DiskDriveATAProtocolEndpointCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveATAProtocolEndpointModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveATAProtocolEndpointDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveATAProtocolEndpointExecQuery(
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
    LMI_DiskDriveATAProtocolEndpoint,
    LMI_DiskDriveATAProtocolEndpoint,
    _cb,
    LMI_DiskDriveATAProtocolEndpointInitialize(ctx))

static CMPIStatus LMI_DiskDriveATAProtocolEndpointMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveATAProtocolEndpointInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_DiskDriveATAProtocolEndpoint_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_DiskDriveATAProtocolEndpoint,
    LMI_DiskDriveATAProtocolEndpoint,
    _cb,
    LMI_DiskDriveATAProtocolEndpointInitialize(ctx))

KUint32 LMI_DiskDriveATAProtocolEndpoint_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveATAProtocolEndpointRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDriveATAProtocolEndpoint_BroadcastReset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveATAProtocolEndpointRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DiskDriveATAProtocolEndpoint",
    "LMI_DiskDriveATAProtocolEndpoint",
    "instance method")
