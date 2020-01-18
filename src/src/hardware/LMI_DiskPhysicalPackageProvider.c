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
#include "LMI_DiskPhysicalPackage.h"
#include "utils.h"
#include "smartctl.h"
#include "lsblk.h"

static const CMPIBroker* _cb = NULL;

static void LMI_DiskPhysicalPackageInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_DiskPhysicalPackageCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskPhysicalPackageEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_DiskPhysicalPackageEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_DiskPhysicalPackage lmi_hdd;
    const char *ns = KNameSpace(cop);
    unsigned i, j;
    char instance_id[BUFLEN], *serial_nb, *name, *model;
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

        model = lsblk_hdds[i].model;
        serial_nb = lsblk_hdds[i].serial;
        name = lsblk_hdds[i].name;

        /* check for smartctl output */
        for (j = 0; j < smtcl_hdds_nb; j++) {
            if (strcmp(smtcl_hdds[j].dev_path, lsblk_hdds[i].name) == 0) {
                model = smtcl_hdds[i].model;
                serial_nb = smtcl_hdds[i].serial_number;
                if (strlen(smtcl_hdds[j].name)) {
                    name = smtcl_hdds[j].name;
                } else if (strlen(smtcl_hdds[j].model)) {
                    name = smtcl_hdds[j].model;
                }

                break;
            }
        }

        LMI_DiskPhysicalPackage_Init(&lmi_hdd, _cb, ns);

        LMI_DiskPhysicalPackage_Set_CreationClassName(&lmi_hdd,
                LMI_DiskPhysicalPackage_ClassName);
        LMI_DiskPhysicalPackage_Set_PackageType(&lmi_hdd,
                LMI_DiskPhysicalPackage_PackageType_Storage_Media_Package_e_g___Disk_or_Tape_Drive);
        LMI_DiskPhysicalPackage_Set_Caption(&lmi_hdd, "Physical Disk Package");
        LMI_DiskPhysicalPackage_Set_Description(&lmi_hdd,
                "This object represents one physical disk package in system.");

        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_DiskPhysicalPackage_ClassName ":%s",
                lsblk_hdds[i].name);

        LMI_DiskPhysicalPackage_Set_Tag(&lmi_hdd, lsblk_hdds[i].name);
        LMI_DiskPhysicalPackage_Set_Manufacturer(&lmi_hdd, lsblk_hdds[i].vendor);
        LMI_DiskPhysicalPackage_Set_Model(&lmi_hdd, model);
        LMI_DiskPhysicalPackage_Set_SerialNumber(&lmi_hdd, serial_nb);
        LMI_DiskPhysicalPackage_Set_Name(&lmi_hdd, name);
        LMI_DiskPhysicalPackage_Set_ElementName(&lmi_hdd, name);
        LMI_DiskPhysicalPackage_Set_InstanceID(&lmi_hdd, instance_id);

        KReturnInstance(cr, lmi_hdd);
    }

done:
    smartctl_free_hdds(&smtcl_hdds, &smtcl_hdds_nb);
    lsblk_free_hdds(&lsblk_hdds, &lsblk_hdds_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskPhysicalPackageGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_DiskPhysicalPackageCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskPhysicalPackageModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskPhysicalPackageDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DiskPhysicalPackageExecQuery(
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
    LMI_DiskPhysicalPackage,
    LMI_DiskPhysicalPackage,
    _cb,
    LMI_DiskPhysicalPackageInitialize(ctx))

static CMPIStatus LMI_DiskPhysicalPackageMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DiskPhysicalPackageInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_DiskPhysicalPackage_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_DiskPhysicalPackage,
    LMI_DiskPhysicalPackage,
    _cb,
    LMI_DiskPhysicalPackageInitialize(ctx))

KUint32 LMI_DiskPhysicalPackage_IsCompatible(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_DiskPhysicalPackageRef* self,
    const KRef* ElementToCheck,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DiskPhysicalPackage",
    "LMI_DiskPhysicalPackage",
    "instance method")
