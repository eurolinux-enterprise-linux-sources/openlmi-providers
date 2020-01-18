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
#include "LMI_Chassis.h"
#include "utils.h"
#include "dmidecode.h"
#include "virt_what.h"

CMPIUint16 get_chassis_type(const char *dmi_chassis);

static const CMPIBroker* _cb = NULL;

static void LMI_ChassisInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_ChassisCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ChassisEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ChassisEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_Chassis lmi_chassis;
    const char *ns = KNameSpace(cop);
    char instance_id[BUFLEN], *tag, *virt = NULL;
    DmiChassis dmi_chassis;

    if (dmi_get_chassis(&dmi_chassis) != 0) {
        goto done;
    }

    LMI_Chassis_Init(&lmi_chassis, _cb, ns);

    LMI_Chassis_Set_CreationClassName(&lmi_chassis,
            LMI_Chassis_ClassName);
    LMI_Chassis_Set_PackageType(&lmi_chassis,
            LMI_Chassis_PackageType_Chassis_Frame);
    LMI_Chassis_Set_Caption(&lmi_chassis, "System Chassis");
    LMI_Chassis_Set_Description(&lmi_chassis,
            "This object represents physical chassis of the system.");

    tag = dmi_get_chassis_tag(&dmi_chassis);
    snprintf(instance_id, BUFLEN,
        LMI_ORGID ":" LMI_Chassis_ClassName ":%s", tag);

    LMI_Chassis_Set_Tag(&lmi_chassis, tag);
    LMI_Chassis_Set_InstanceID(&lmi_chassis, instance_id);

    if (strcmp(dmi_chassis.type, "Unknown") == 0 ||
            strcmp(dmi_chassis.type, "Other") == 0) {
        LMI_Chassis_Set_Name(&lmi_chassis, "System Chassis");
        LMI_Chassis_Set_ElementName(&lmi_chassis, "System Chassis");
    } else {
        LMI_Chassis_Set_Name(&lmi_chassis, dmi_chassis.type);
        LMI_Chassis_Set_ElementName(&lmi_chassis, dmi_chassis.type);
    }
    LMI_Chassis_Set_ChassisPackageType(&lmi_chassis,
            get_chassis_type(dmi_chassis.type));
    LMI_Chassis_Set_Manufacturer(&lmi_chassis, dmi_chassis.manufacturer);
    LMI_Chassis_Set_SerialNumber(&lmi_chassis, dmi_chassis.serial_number);
    LMI_Chassis_Set_SKU(&lmi_chassis, dmi_chassis.sku_number);
    LMI_Chassis_Set_Version(&lmi_chassis, dmi_chassis.version);
    LMI_Chassis_Set_LockPresent(&lmi_chassis, dmi_chassis.has_lock);
    LMI_Chassis_Set_Model(&lmi_chassis, dmi_chassis.model);
    LMI_Chassis_Set_ProductName(&lmi_chassis, dmi_chassis.product_name);
    LMI_Chassis_Set_UUID(&lmi_chassis, dmi_chassis.uuid);
    if (dmi_chassis.power_cords) {
        LMI_Chassis_Set_NumberOfPowerCords(&lmi_chassis,
                dmi_chassis.power_cords);
    }
    if (virt_what_get_virtual_type(&virt) == 0 && virt && strlen(virt)) {
        LMI_Chassis_Set_VirtualMachine(&lmi_chassis, virt);
    } else {
        LMI_Chassis_Set_VirtualMachine(&lmi_chassis, "No");
    }
    free(virt);

    KReturnInstance(cr, lmi_chassis);

done:
    dmi_free_chassis(&dmi_chassis);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ChassisGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ChassisCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ChassisModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ChassisDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ChassisExecQuery(
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
    LMI_Chassis,
    LMI_Chassis,
    _cb,
    LMI_ChassisInitialize(ctx))

static CMPIStatus LMI_ChassisMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ChassisInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Chassis_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_Chassis,
    LMI_Chassis,
    _cb,
    LMI_ChassisInitialize(ctx))

KUint32 LMI_Chassis_IsCompatible(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ChassisRef* self,
    const KRef* ElementToCheck,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

/*
 * Get Chassis Type according to the dmidecode output.
 * @param dmi_chassis chassis type from dmidecode
 * @return chassis type ID defined by CIM
 */
CMPIUint16 get_chassis_type(const char *dmi_chassis)
{
    if (!dmi_chassis || strlen(dmi_chassis) < 1) {
        return 0; /* Unknown */
    }

    static struct {
        CMPIUint16 value_map;   /* ValueMap defined by CIM */
        char *search;           /* String used to match the dmidecode chassis type */
    } types[] = {
        {0,  "Unknown"},
        {1,  "Other"},
        /*
        {2,  "SMBIOS Reserved"},
        */
        {3,  "Desktop"},
        {4,  "Low Profile Desktop"},
        {5,  "Pizza Box"},
        {6,  "Mini Tower"},
        {7,  "Tower"},
        {8,  "Portable"},
        {9,  "Laptop"},
        {10, "Notebook"},
        {11, "Hand Held"},
        {12, "Docking Station"},
        {13, "All In One"},
        {14, "Sub Notebook"},
        {15, "Space-saving"},
        {16, "Lunch Box"},
        {17, "Main Server Chassis"},
        {18, "Expansion Chassis"},
        {19, "Sub Chassis"},
        {20, "Bus Expansion Chassis"},
        {21, "Peripheral Chassis"},
        {22, "RAID Chassis"},
        {23, "Rack Mount Chassis"},
        {24, "Sealed-case PC"},
        {25, "Multi-system"},
        {26, "CompactPCI"},
        {27, "AdvancedTCA"},
        /*
        {28, "Blade Enclosure"},
        */
    };

    size_t i, types_length = sizeof(types) / sizeof(types[0]);

    for (i = 0; i < types_length; i++) {
        if (strcmp(dmi_chassis, types[i].search) == 0) {
            return types[i].value_map;
        }
    }

    return 1; /* Other */
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_Chassis",
    "LMI_Chassis",
    "instance method")
