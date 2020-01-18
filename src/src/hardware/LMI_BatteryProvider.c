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
#include "LMI_Battery.h"
#include "utils.h"
#include "dmidecode.h"

CMPIUint16 get_chemistry(const char *dmi_val);

static const CMPIBroker* _cb = NULL;

static void LMI_BatteryInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_BatteryCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BatteryEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_BatteryEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_Battery lmi_batt;
    const char *ns = KNameSpace(cop);
    char instance_id[BUFLEN];
    unsigned i;
    DmiBattery *dmi_batt = NULL;
    unsigned dmi_batt_nb = 0;

    if (dmi_get_batteries(&dmi_batt, &dmi_batt_nb) != 0 || dmi_batt_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_batt_nb; i++) {
        LMI_Battery_Init(&lmi_batt, _cb, ns);

        LMI_Battery_Set_SystemCreationClassName(&lmi_batt,
                lmi_get_system_creation_class_name());
        LMI_Battery_Set_SystemName(&lmi_batt, lmi_get_system_name_safe(cc));
        LMI_Battery_Set_CreationClassName(&lmi_batt,
                LMI_Battery_ClassName);
        LMI_Battery_Set_BatteryStatus(&lmi_batt,
                LMI_Battery_BatteryStatus_Unknown);
        LMI_Battery_Init_OperationalStatus(&lmi_batt, 1);
        LMI_Battery_Set_OperationalStatus(&lmi_batt, 0,
                LMI_Battery_OperationalStatus_Unknown);
        LMI_Battery_Set_HealthState(&lmi_batt,
                LMI_Battery_HealthState_Unknown);
        LMI_Battery_Set_EnabledState(&lmi_batt,
                LMI_Battery_EnabledState_Unknown);
        LMI_Battery_Set_Caption(&lmi_batt, "Battery");
        LMI_Battery_Set_Description(&lmi_batt,
                "This object represents one battery in system.");

        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_Battery_ClassName ":%s",
                dmi_batt[i].name);

        LMI_Battery_Set_DeviceID(&lmi_batt, dmi_batt[i].name);
        LMI_Battery_Set_ElementName(&lmi_batt, dmi_batt[i].name);
        LMI_Battery_Set_Name(&lmi_batt, dmi_batt[i].name);
        LMI_Battery_Set_InstanceID(&lmi_batt, instance_id);
        LMI_Battery_Set_Chemistry(&lmi_batt,
                get_chemistry(dmi_batt[i].chemistry));
        LMI_Battery_Set_DesignCapacity(&lmi_batt, dmi_batt[i].design_capacity);
        LMI_Battery_Set_DesignVoltage(&lmi_batt, dmi_batt[i].design_voltage);

        KReturnInstance(cr, lmi_batt);
    }

done:
    dmi_free_batteries(&dmi_batt, &dmi_batt_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BatteryGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_BatteryCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BatteryModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BatteryDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BatteryExecQuery(
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
    LMI_Battery,
    LMI_Battery,
    _cb,
    LMI_BatteryInitialize(ctx))

static CMPIStatus LMI_BatteryMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BatteryInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Battery_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_Battery,
    LMI_Battery,
    _cb,
    LMI_BatteryInitialize(ctx))

KUint32 LMI_Battery_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BatteryRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Battery_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BatteryRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Battery_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BatteryRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Battery_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BatteryRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Battery_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BatteryRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Battery_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BatteryRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Battery_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BatteryRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Battery_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BatteryRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

/*
 * Get battery chemistry according to the dmidecode.
 * @param dmi_val from dmidecode
 * @return CIM id of pointing type
 */
CMPIUint16 get_chemistry(const char *dmi_val)
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
        {3, "Lead Acid"},
        {4, "Nickel Cadmium"},
        {5, "Nickel Metal Hydride"},
        {6, "Lithium Ion"},
        {6, "LION"},
        {7, "Zinc Air"},
        {8, "Lithium Polymer"},
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
    "LMI_Battery",
    "LMI_Battery",
    "instance method")
