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

#include <time.h>
#include <konkret/konkret.h>
#include "LMI_BatteryPhysicalPackage.h"
#include "utils.h"
#include "dmidecode.h"

static const CMPIBroker* _cb = NULL;

static void LMI_BatteryPhysicalPackageInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_BatteryPhysicalPackageCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BatteryPhysicalPackageEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_BatteryPhysicalPackageEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_BatteryPhysicalPackage lmi_batt_phys;
    const char *ns = KNameSpace(cop);
    char instance_id[BUFLEN];
    struct tm tm;
    unsigned i;
    DmiBattery *dmi_batt = NULL;
    unsigned dmi_batt_nb = 0;

    if (dmi_get_batteries(&dmi_batt, &dmi_batt_nb) != 0 || dmi_batt_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_batt_nb; i++) {
        LMI_BatteryPhysicalPackage_Init(&lmi_batt_phys, _cb, ns);

        LMI_BatteryPhysicalPackage_Set_CreationClassName(&lmi_batt_phys,
                LMI_BatteryPhysicalPackage_ClassName);
        LMI_BatteryPhysicalPackage_Set_PackageType(&lmi_batt_phys,
                LMI_BatteryPhysicalPackage_PackageType_Battery);
        LMI_BatteryPhysicalPackage_Set_Caption(&lmi_batt_phys,
                "Physical Battery Package");
        LMI_BatteryPhysicalPackage_Set_Description(&lmi_batt_phys,
                "This object represents one physical battery package in system.");

        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_BatteryPhysicalPackage_ClassName ":%s",
                dmi_batt[i].name);

        LMI_BatteryPhysicalPackage_Set_Tag(&lmi_batt_phys, dmi_batt[i].name);
        LMI_BatteryPhysicalPackage_Set_ElementName(&lmi_batt_phys,
                dmi_batt[i].name);
        LMI_BatteryPhysicalPackage_Set_Name(&lmi_batt_phys, dmi_batt[i].name);
        LMI_BatteryPhysicalPackage_Set_Manufacturer(&lmi_batt_phys,
                dmi_batt[i].manufacturer);
        LMI_BatteryPhysicalPackage_Set_SerialNumber(&lmi_batt_phys,
                dmi_batt[i].serial_number);
        LMI_BatteryPhysicalPackage_Set_Version(&lmi_batt_phys,
                dmi_batt[i].version);
        LMI_BatteryPhysicalPackage_Set_InstanceID(&lmi_batt_phys, instance_id);

        memset(&tm, 0, sizeof(struct tm));
        if (strptime(dmi_batt[i].manufacture_date, "%F", &tm)) {
            LMI_BatteryPhysicalPackage_Set_ManufactureDate(&lmi_batt_phys,
                    CMNewDateTimeFromBinary(_cb, mktime(&tm) * 1000000, 0, NULL));
        }

        KReturnInstance(cr, lmi_batt_phys);
    }

done:
    dmi_free_batteries(&dmi_batt, &dmi_batt_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BatteryPhysicalPackageGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_BatteryPhysicalPackageCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BatteryPhysicalPackageModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BatteryPhysicalPackageDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BatteryPhysicalPackageExecQuery(
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
    LMI_BatteryPhysicalPackage,
    LMI_BatteryPhysicalPackage,
    _cb,
    LMI_BatteryPhysicalPackageInitialize(ctx))

static CMPIStatus LMI_BatteryPhysicalPackageMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BatteryPhysicalPackageInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_BatteryPhysicalPackage_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_BatteryPhysicalPackage,
    LMI_BatteryPhysicalPackage,
    _cb,
    LMI_BatteryPhysicalPackageInitialize(ctx))

KUint32 LMI_BatteryPhysicalPackage_IsCompatible(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BatteryPhysicalPackageRef* self,
    const KRef* ElementToCheck,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_BatteryPhysicalPackage",
    "LMI_BatteryPhysicalPackage",
    "instance method")
