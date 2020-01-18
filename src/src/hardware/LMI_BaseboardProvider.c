/*
 * Copyright (C) 2013 Red Hat, Inc. All rights reserved.
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
#include "LMI_Baseboard.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb = NULL;

static void LMI_BaseboardInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_BaseboardCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BaseboardEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_BaseboardEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_Baseboard lmi_baseboard;
    const char *ns = KNameSpace(cop);
    char instance_id[INSTANCE_ID_LEN];
    DmiBaseboard dmi_baseboard;

    if (dmi_get_baseboard(&dmi_baseboard) != 0) {
        goto done;
    }

    LMI_Baseboard_Init(&lmi_baseboard, _cb, ns);

    LMI_Baseboard_Set_CreationClassName(&lmi_baseboard,
            ORGID "_" BASEBOARD_CLASS_NAME);
    LMI_Baseboard_Set_PackageType(&lmi_baseboard,
            LMI_Baseboard_PackageType_Cross_Connect_Backplane);
    LMI_Baseboard_Set_Name(&lmi_baseboard, BASEBOARD_CLASS_NAME);
    LMI_Baseboard_Set_ElementName(&lmi_baseboard, BASEBOARD_CLASS_NAME);
    LMI_Baseboard_Set_HostingBoard(&lmi_baseboard, 1);
    LMI_Baseboard_Set_Caption(&lmi_baseboard, BASEBOARD_CLASS_NAME);
    LMI_Baseboard_Set_Description(&lmi_baseboard,
            "This object represents baseboard of the system.");

    if (strcmp(dmi_baseboard.serial_number, "Not Specified") == 0) {
        LMI_Baseboard_Set_Tag(&lmi_baseboard, "0");
        LMI_Baseboard_Set_InstanceID(&lmi_baseboard,
            ORGID ":" ORGID "_" BASEBOARD_CLASS_NAME ":0");
    } else {
        LMI_Baseboard_Set_Tag(&lmi_baseboard, dmi_baseboard.serial_number);
        snprintf(instance_id, INSTANCE_ID_LEN,
            ORGID ":" ORGID "_" BASEBOARD_CLASS_NAME ":%s",
            dmi_baseboard.serial_number);
        LMI_Baseboard_Set_InstanceID(&lmi_baseboard, instance_id);
    }
    LMI_Baseboard_Set_Manufacturer(&lmi_baseboard, dmi_baseboard.manufacturer);
    LMI_Baseboard_Set_Model(&lmi_baseboard, dmi_baseboard.product_name);
    LMI_Baseboard_Set_SerialNumber(&lmi_baseboard, dmi_baseboard.serial_number);
    LMI_Baseboard_Set_Version(&lmi_baseboard, dmi_baseboard.version);

    KReturnInstance(cr, lmi_baseboard);

done:
    dmi_free_baseboard(&dmi_baseboard);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BaseboardGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_BaseboardCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BaseboardModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BaseboardDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BaseboardExecQuery(
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
    LMI_Baseboard,
    LMI_Baseboard,
    _cb,
    LMI_BaseboardInitialize(ctx))

static CMPIStatus LMI_BaseboardMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BaseboardInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Baseboard_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_Baseboard,
    LMI_Baseboard,
    _cb,
    LMI_BaseboardInitialize(ctx))

KUint32 LMI_Baseboard_IsCompatible(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BaseboardRef* self,
    const KRef* ElementToCheck,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Baseboard_ConnectorPower(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BaseboardRef* self,
    const KRef* Connector,
    const KBoolean* PoweredOn,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_Baseboard",
    "LMI_Baseboard",
    "instance method")
