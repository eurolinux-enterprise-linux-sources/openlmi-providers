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
#include "LMI_PointingDevice.h"
#include "utils.h"
#include "dmidecode.h"

CMPIUint16 get_pointingtype(const char *dmi_val);

static const CMPIBroker* _cb = NULL;

static void LMI_PointingDeviceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_PointingDeviceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PointingDeviceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PointingDeviceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_PointingDevice lmi_dev;
    const char *ns = KNameSpace(cop);
    char instance_id[BUFLEN];
    unsigned i;
    DmiPointingDevice *dmi_dev = NULL;
    unsigned dmi_dev_nb = 0;

    if (dmi_get_pointing_devices(&dmi_dev, &dmi_dev_nb) != 0 || dmi_dev_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_dev_nb; i++) {
        LMI_PointingDevice_Init(&lmi_dev, _cb, ns);

        LMI_PointingDevice_Set_SystemCreationClassName(&lmi_dev,
                lmi_get_system_creation_class_name());
        LMI_PointingDevice_Set_SystemName(&lmi_dev, lmi_get_system_name_safe(cc));
        LMI_PointingDevice_Set_CreationClassName(&lmi_dev,
                LMI_PointingDevice_ClassName);
        LMI_PointingDevice_Set_Caption(&lmi_dev, "Pointing Device");
        LMI_PointingDevice_Set_Description(&lmi_dev,
                "This object represents one pointing device.");

        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_PointingDevice_ClassName ":%s",
                dmi_dev[i].type);

        LMI_PointingDevice_Set_DeviceID(&lmi_dev, dmi_dev[i].type);
        LMI_PointingDevice_Set_NumberOfButtons(&lmi_dev, dmi_dev[i].buttons);
        LMI_PointingDevice_Set_PointingType(&lmi_dev,
                get_pointingtype(dmi_dev[i].type));
        LMI_PointingDevice_Set_ElementName(&lmi_dev, dmi_dev[i].type);
        LMI_PointingDevice_Set_Name(&lmi_dev, dmi_dev[i].type);
        LMI_PointingDevice_Set_InstanceID(&lmi_dev, instance_id);

        KReturnInstance(cr, lmi_dev);
    }

done:
    dmi_free_pointing_devices(&dmi_dev, &dmi_dev_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PointingDeviceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PointingDeviceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PointingDeviceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PointingDeviceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PointingDeviceExecQuery(
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
    LMI_PointingDevice,
    LMI_PointingDevice,
    _cb,
    LMI_PointingDeviceInitialize(ctx))

static CMPIStatus LMI_PointingDeviceMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PointingDeviceInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_PointingDevice_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_PointingDevice,
    LMI_PointingDevice,
    _cb,
    LMI_PointingDeviceInitialize(ctx))

KUint32 LMI_PointingDevice_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PointingDeviceRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PointingDevice_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PointingDeviceRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PointingDevice_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PointingDeviceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PointingDevice_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PointingDeviceRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PointingDevice_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PointingDeviceRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PointingDevice_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PointingDeviceRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PointingDevice_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PointingDeviceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PointingDevice_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PointingDeviceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

/*
 * Get pointing type according to the dmidecode.
 * @param dmi_val from dmidecode
 * @return CIM id of pointing type
 */
CMPIUint16 get_pointingtype(const char *dmi_val)
{
    if (!dmi_val || !strlen(dmi_val)) {
        return 2; /* Unknown */
    }

    static struct {
        CMPIUint16 cim_val;     /* CIM value */
        char *dmi_val;          /* dmidecode value */
    } values[] = {
        {1, "Other"},
        {2, "Unknown"},
        {3, "Mouse"},
        {4, "Track Ball"},
        {5, "Track Point"},
        {6, "Glide Point"},
        {7, "Touch Pad"},
        {8, "Touch Screen"},
        {9, "Optical Sensor"},
    };

    size_t i, val_length = sizeof(values) / sizeof(values[0]);

    for (i = 0; i < val_length; i++) {
        if (strcmp(dmi_val, values[i].dmi_val) == 0) {
            return values[i].cim_val;
        }
    }

    return 1; /* Other */
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PointingDevice",
    "LMI_PointingDevice",
    "instance method")
