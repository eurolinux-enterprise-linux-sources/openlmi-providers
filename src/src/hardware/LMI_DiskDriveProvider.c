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

#include <unistd.h>
#include <limits.h>
#include <konkret/konkret.h>
#include "LMI_DiskDrive.h"
#include "utils.h"
#include "smartctl.h"
#include "lsblk.h"
#include "sysfs.h"

CMPIUint16 get_operational_status(const char *smart_status);
CMPIUint16 get_hdd_form_factor(const unsigned short form_factor);

static const CMPIBroker* _cb = NULL;

static void LMI_DiskDriveInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_DiskDriveCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_DiskDriveEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_DiskDrive lmi_hdd;
    const char *ns = KNameSpace(cop);
    unsigned i, j, rotational = 0, rpm;
    unsigned long capacity = 0;
    char instance_id[BUFLEN], path[PATH_MAX], *name;
    CMPIUint16 operational_status;
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
        /* skip iscsi devices */
        if (strcmp(lsblk_hdds[i].tran, "iscsi") == 0) {
            continue;
        }

        name = lsblk_hdds[i].name;
        capacity = lsblk_hdds[i].capacity;

        /* RPM value for unknown */
        rpm = 0xffffffff;

        LMI_DiskDrive_Init(&lmi_hdd, _cb, ns);

        LMI_DiskDrive_Set_SystemCreationClassName(&lmi_hdd,
                lmi_get_system_creation_class_name());
        LMI_DiskDrive_Set_SystemName(&lmi_hdd, lmi_get_system_name_safe(cc));
        LMI_DiskDrive_Set_CreationClassName(&lmi_hdd,
                LMI_DiskDrive_ClassName);
        LMI_DiskDrive_Set_Caption(&lmi_hdd, "Disk Drive");
        LMI_DiskDrive_Set_Description(&lmi_hdd,
                "This object represents one physical disk drive in system.");

        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_DiskDrive_ClassName ":%s",
                lsblk_hdds[i].name);

        LMI_DiskDrive_Set_DeviceID(&lmi_hdd, lsblk_hdds[i].name);
        LMI_DiskDrive_Set_InstanceID(&lmi_hdd, instance_id);

        /* check for smartctl output */
        for (j = 0; j < smtcl_hdds_nb; j++) {
            if (strcmp(smtcl_hdds[j].dev_path, lsblk_hdds[i].name) == 0) {
                operational_status = get_operational_status(
                        smtcl_hdds[j].smart_status);
                if (strlen(smtcl_hdds[j].name)) {
                    name = smtcl_hdds[j].name;
                } else if (strlen(smtcl_hdds[j].model)) {
                    name = smtcl_hdds[j].model;
                }
                if (!capacity) {
                    capacity = smtcl_hdds[j].capacity;
                }

                LMI_DiskDrive_Init_OperationalStatus(&lmi_hdd, 1);
                LMI_DiskDrive_Set_OperationalStatus(&lmi_hdd, 0, operational_status);
                if (operational_status == LMI_DiskDrive_OperationalStatus_OK) {
                    LMI_DiskDrive_Set_EnabledState(&lmi_hdd,
                            LMI_DiskDrive_EnabledState_Enabled);
                } else {
                    LMI_DiskDrive_Set_EnabledState(&lmi_hdd,
                            LMI_DiskDrive_EnabledState_Unknown);
                }

                LMI_DiskDrive_Set_FormFactor(&lmi_hdd,
                        get_hdd_form_factor(smtcl_hdds[j].form_factor));
                rpm = smtcl_hdds[j].rpm;
                if (smtcl_hdds[j].port_speed) {
                    LMI_DiskDrive_Set_InterconnectSpeed(&lmi_hdd,
                            smtcl_hdds[j].port_speed);
                }

                if (smtcl_hdds[j].curr_temp > SHRT_MIN) {
                    LMI_DiskDrive_Set_Temperature(&lmi_hdd,
                            smtcl_hdds[j].curr_temp);
                }

                break;
            }
        }

        LMI_DiskDrive_Set_Name(&lmi_hdd, name);
        LMI_DiskDrive_Set_ElementName(&lmi_hdd, name);
        LMI_DiskDrive_Set_Capacity(&lmi_hdd, capacity);

        snprintf(path, PATH_MAX, SYSFS_BLOCK_PATH "/%s/queue/rotational",
                lsblk_hdds[i].basename);
        if (path_get_unsigned(path, &rotational) == 0) {
            if (rotational == 1) {
                LMI_DiskDrive_Set_DiskType(&lmi_hdd,
                        LMI_DiskDrive_DiskType_Hard_Disk_Drive);
            } else if (rotational == 0) {
                LMI_DiskDrive_Set_DiskType(&lmi_hdd,
                        LMI_DiskDrive_DiskType_Solid_State_Drive);
                rpm = 0;
            } else {
                LMI_DiskDrive_Set_DiskType(&lmi_hdd,
                        LMI_DiskDrive_DiskType_Other);
            }
        } else {
            LMI_DiskDrive_Set_DiskType(&lmi_hdd,
                    LMI_DiskDrive_DiskType_Unknown);
        }

        LMI_DiskDrive_Set_RPM(&lmi_hdd, rpm);

        if (lsblk_hdds[i].basename[0] == 'h') {
            LMI_DiskDrive_Set_InterconnectType(&lmi_hdd,
                    LMI_DiskDrive_InterconnectType_ATA);
        } else if (lsblk_hdds[i].basename[0] == 's') {
            /* TODO: What about SAS? */
            LMI_DiskDrive_Set_InterconnectType(&lmi_hdd,
                    LMI_DiskDrive_InterconnectType_SATA);
        } else {
            LMI_DiskDrive_Set_InterconnectType(&lmi_hdd,
                    LMI_DiskDrive_InterconnectType_Other);
        }

        KReturnInstance(cr, lmi_hdd);
    }

done:
    smartctl_free_hdds(&smtcl_hdds, &smtcl_hdds_nb);
    lsblk_free_hdds(&lsblk_hdds, &lsblk_hdds_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_DiskDriveCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveExecQuery(
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
    LMI_DiskDrive,
    LMI_DiskDrive,
    _cb,
    LMI_DiskDriveInitialize(ctx))

static CMPIStatus LMI_DiskDriveMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_DiskDrive_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_DiskDrive,
    LMI_DiskDrive,
    _cb,
    LMI_DiskDriveInitialize(ctx))

KUint32 LMI_DiskDrive_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDrive_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDrive_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDrive_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDrive_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDrive_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDrive_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDrive_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_DiskDrive_LockMedia(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskDriveRef* self,
    const KBoolean* Lock,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

/*
 * Get CIM operational status according to smart status.
 * @param smart_status from smartctl
 * @return CIM id of operational status
 */
CMPIUint16 get_operational_status(const char *smart_status)
{
    static struct {
        unsigned short cim_val;     /* CIM value */
        char *smart_status;         /* smart status */
    } values[] = {
        {LMI_DiskDrive_OperationalStatus_Unknown, "UNKNOWN!"},
        {LMI_DiskDrive_OperationalStatus_OK, "PASSED"},
        {LMI_DiskDrive_OperationalStatus_Predictive_Failure, "FAILED!"},
    };

    size_t i, val_length = sizeof(values) / sizeof(values[0]);

    for (i = 0; i < val_length; i++) {
        if (strcmp(smart_status, values[i].smart_status) == 0) {
            return values[i].cim_val;
        }
    }

    return LMI_DiskDrive_OperationalStatus_Unknown;
}

/*
 * Get CIM form factor according to reported disk value.
 * @param form_factor from smartctl
 * @return CIM id of form factor
 */
CMPIUint16 get_hdd_form_factor(const unsigned short form_factor)
{
    static struct {
        unsigned short cim_val;     /* CIM value */
        unsigned short form_factor; /* form factor */
    } values[] = {
        {LMI_DiskDrive_FormFactor_Not_Reported, 0},
        {LMI_DiskDrive_FormFactor_5_25_inch, 1},
        {LMI_DiskDrive_FormFactor_3_5_inch, 2},
        {LMI_DiskDrive_FormFactor_2_5_inch, 3},
        {LMI_DiskDrive_FormFactor_1_8_inch, 4},
    };

    size_t i, val_length = sizeof(values) / sizeof(values[0]);

    for (i = 0; i < val_length; i++) {
        if (form_factor == values[i].form_factor) {
            return values[i].cim_val;
        }
    }

    return LMI_DiskDrive_FormFactor_Other;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DiskDrive",
    "LMI_DiskDrive",
    "instance method")
