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
#include "LMI_DiskDriveATAPort.h"
#include "utils.h"
#include "smartctl.h"
#include "lsblk.h"

CMPIUint16 get_port_type(const char *port_type);

static const CMPIBroker* _cb = NULL;

static void LMI_DiskDriveATAPortInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_DiskDriveATAPortCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveATAPortEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_DiskDriveATAPortEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_DiskDriveATAPort lmi_hdd_ata_port;
    const char *ns = KNameSpace(cop);
    unsigned i, j;
    char instance_id[BUFLEN], name[BUFLEN];
    SmartctlHdd *smtcl_hdds = NULL;
    unsigned smtcl_hdds_nb = 0;
    LsblkHdd *lsblk_hdds = NULL;
    unsigned lsblk_hdds_nb = 0;

    if (lsblk_get_hdds(&lsblk_hdds, &lsblk_hdds_nb) != 0 || lsblk_hdds_nb < 1) {
        goto done;
    }
    if (smartctl_get_hdds(&smtcl_hdds, &smtcl_hdds_nb) != 0 || smtcl_hdds_nb < 1) {
        smartctl_free_hdds(&smtcl_hdds, &smtcl_hdds_nb);
    }

    for (i = 0; i < lsblk_hdds_nb; i++) {
        /* use only disk devices from lsblk */
        if (strcmp(lsblk_hdds[i].type, "disk") != 0) {
            continue;
        }

        LMI_DiskDriveATAPort_Init(&lmi_hdd_ata_port, _cb, ns);

        LMI_DiskDriveATAPort_Set_SystemCreationClassName(&lmi_hdd_ata_port,
                lmi_get_system_creation_class_name());
        LMI_DiskDriveATAPort_Set_SystemName(&lmi_hdd_ata_port,
                lmi_get_system_name_safe(cc));
        LMI_DiskDriveATAPort_Set_CreationClassName(&lmi_hdd_ata_port,
                LMI_DiskDriveATAPort_ClassName);
        LMI_DiskDriveATAPort_Set_Caption(&lmi_hdd_ata_port,
                "Disk Drive ATA Port");
        LMI_DiskDriveATAPort_Set_Description(&lmi_hdd_ata_port,
                "This object represents ATA Port of disk drive in system.");
        LMI_DiskDriveATAPort_Set_UsageRestriction(&lmi_hdd_ata_port,
                LMI_DiskDriveATAPort_UsageRestriction_Front_end_only);

        snprintf(name, BUFLEN,
                "%s " LMI_DiskDriveATAPort_ClassName,
                lsblk_hdds[i].name);
        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_DiskDriveATAPort_ClassName ":%s",
                name);

        LMI_DiskDriveATAPort_Set_DeviceID(&lmi_hdd_ata_port, name);
        LMI_DiskDriveATAPort_Set_Name(&lmi_hdd_ata_port, name);
        LMI_DiskDriveATAPort_Set_ElementName(&lmi_hdd_ata_port, name);
        LMI_DiskDriveATAPort_Set_InstanceID(&lmi_hdd_ata_port, instance_id);

        /* check for smartctl output */
        for (j = 0; j < smtcl_hdds_nb; j++) {
            if (strcmp(smtcl_hdds[j].dev_path, lsblk_hdds[i].name) == 0) {
                LMI_DiskDriveATAPort_Set_PortType(&lmi_hdd_ata_port,
                        get_port_type(smtcl_hdds[j].port_type));

                if (smtcl_hdds[j].max_port_speed) {
                    LMI_DiskDriveATAPort_Set_MaxSpeed(&lmi_hdd_ata_port,
                            smtcl_hdds[j].max_port_speed);
                }
                if (smtcl_hdds[j].port_speed) {
                    LMI_DiskDriveATAPort_Set_Speed(&lmi_hdd_ata_port,
                            smtcl_hdds[j].port_speed);
                }

                break;
            }
        }

        KReturnInstance(cr, lmi_hdd_ata_port);
    }

done:
    smartctl_free_hdds(&smtcl_hdds, &smtcl_hdds_nb);
    lsblk_free_hdds(&lsblk_hdds, &lsblk_hdds_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveATAPortGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_DiskDriveATAPortCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveATAPortModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveATAPortDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveATAPortExecQuery(
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
    LMI_DiskDriveATAPort,
    LMI_DiskDriveATAPort,
    _cb,
    LMI_DiskDriveATAPortInitialize(ctx))

static CMPIStatus LMI_DiskDriveATAPortMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveATAPortInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_DiskDriveATAPort_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_DiskDriveATAPort,
    LMI_DiskDriveATAPort,
    _cb,
    LMI_DiskDriveATAPortInitialize(ctx))

KUint32 LMI_DiskDriveATAPort_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveATAPortRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDriveATAPort_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveATAPortRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDriveATAPort_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveATAPortRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDriveATAPort_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveATAPortRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDriveATAPort_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveATAPortRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDriveATAPort_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveATAPortRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDriveATAPort_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveATAPortRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDriveATAPort_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveATAPortRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

/*
 * Get CIM port type according to smartctl port type.
 * @param port_type from smartctl
 * @return CIM id of port type
 */
CMPIUint16 get_port_type(const char *port_type)
{
    static struct {
        unsigned short cim_val;     /* CIM value */
        char *port_type;            /* smartctl port type */
    } values[] = {
        {LMI_DiskDriveATAPort_PortType_Unknown, "Unknown"},
        {LMI_DiskDriveATAPort_PortType_ATA, "ATA"},
        {LMI_DiskDriveATAPort_PortType_SATA, "SATA"},
        {LMI_DiskDriveATAPort_PortType_SATA2, "SATA 2"},
    };

    size_t i, val_length = sizeof(values) / sizeof(values[0]);

    for (i = 0; i < val_length; i++) {
        if (strcmp(port_type, values[i].port_type) == 0) {
            return values[i].cim_val;
        }
    }

    return LMI_DiskDriveATAPort_PortType_Other;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DiskDriveATAPort",
    "LMI_DiskDriveATAPort",
    "instance method")
