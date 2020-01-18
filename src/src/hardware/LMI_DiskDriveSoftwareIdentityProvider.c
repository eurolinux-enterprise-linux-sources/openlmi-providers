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
#include "LMI_DiskDriveSoftwareIdentity.h"
#include "utils.h"
#include "smartctl.h"
#include "lsblk.h"

static const CMPIBroker* _cb = NULL;

static void LMI_DiskDriveSoftwareIdentityInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_DiskDriveSoftwareIdentityCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveSoftwareIdentityEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_DiskDriveSoftwareIdentityEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_DiskDriveSoftwareIdentity lmi_hdd_swi;
    const char *ns = KNameSpace(cop);
    unsigned i, j;
    char instance_id[BUFLEN], *fw_ver = NULL, name[BUFLEN];
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

        LMI_DiskDriveSoftwareIdentity_Init(&lmi_hdd_swi, _cb, ns);

        LMI_DiskDriveSoftwareIdentity_Init_Classifications(&lmi_hdd_swi, 1);
        LMI_DiskDriveSoftwareIdentity_Set_Classifications(&lmi_hdd_swi, 0,
                LMI_DiskDriveSoftwareIdentity_Classifications_Firmware);
        LMI_DiskDriveSoftwareIdentity_Set_Caption(&lmi_hdd_swi, "Disk Firmware");
        LMI_DiskDriveSoftwareIdentity_Set_Description(&lmi_hdd_swi,
                "This object represents firmware of disk drive in system.");

        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_DiskDriveSoftwareIdentity_ClassName ":%s",
                lsblk_hdds[i].name);
        snprintf(name, BUFLEN, "%s disk firmware",
                lsblk_hdds[i].name);

        LMI_DiskDriveSoftwareIdentity_Set_InstanceID(&lmi_hdd_swi, instance_id);
        LMI_DiskDriveSoftwareIdentity_Set_Name(&lmi_hdd_swi, name);
        LMI_DiskDriveSoftwareIdentity_Set_ElementName(&lmi_hdd_swi, name);

        /* check for smartctl output */
        for (j = 0; j < smtcl_hdds_nb; j++) {
            if (strcmp(smtcl_hdds[j].dev_path, lsblk_hdds[i].name) == 0) {
                fw_ver = smtcl_hdds[j].firmware;
                break;
            }
        }

        if (fw_ver) {
            LMI_DiskDriveSoftwareIdentity_Set_VersionString(
                    &lmi_hdd_swi, fw_ver);
        } else {
            LMI_DiskDriveSoftwareIdentity_Set_VersionString(
                    &lmi_hdd_swi, "Unknown");
        }

        LMI_DiskDriveSoftwareIdentity_Set_Manufacturer(&lmi_hdd_swi,
                lsblk_hdds[i].vendor);

        KReturnInstance(cr, lmi_hdd_swi);
    }

done:
    smartctl_free_hdds(&smtcl_hdds, &smtcl_hdds_nb);
    lsblk_free_hdds(&lsblk_hdds, &lsblk_hdds_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveSoftwareIdentityGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_DiskDriveSoftwareIdentityCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveSoftwareIdentityModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveSoftwareIdentityDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskDriveSoftwareIdentityExecQuery(
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
    LMI_DiskDriveSoftwareIdentity,
    LMI_DiskDriveSoftwareIdentity,
    _cb,
    LMI_DiskDriveSoftwareIdentityInitialize(ctx))

static CMPIStatus LMI_DiskDriveSoftwareIdentityMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskDriveSoftwareIdentityInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_DiskDriveSoftwareIdentity_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_DiskDriveSoftwareIdentity,
    LMI_DiskDriveSoftwareIdentity,
    _cb,
    LMI_DiskDriveSoftwareIdentityInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DiskDriveSoftwareIdentity",
    "LMI_DiskDriveSoftwareIdentity",
    "instance method")
