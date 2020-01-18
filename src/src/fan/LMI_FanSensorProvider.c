/*
 * Copyright (C) 2012-2013 Red Hat, Inc.  All rights reserved.
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
 * Authors: Michal Minar <miminar@redhat.com>
 *          Radek Novacek <rnovacek@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_FanSensor.h"
#include "fan.h"
#include "globals.h"

static const CMPIBroker* _cb = NULL;

static void LMI_FanSensorInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
    init_linux_fan_module();
}

static CMPIStatus LMI_FanSensorCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FanSensorEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_FanSensorEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus status;
    char buf[200];
    struct cim_fan *sptr = NULL;
    struct fanlist *lptr = NULL, *fans = NULL;
    if (enum_all_fans(&fans) != 0 ) {
        KReturn2(_cb, ERR_FAILED, "Could not list get fan list.");
    }

    lptr = fans;
    // iterate fan list
    while (lptr) {
        sptr = lptr->f;
        LMI_FanSensor w;
        LMI_FanSensor_Init(&w, _cb, KNameSpace(cop));
        LMI_FanSensor_Set_CreationClassName(&w, "LMI_FanSensor");
        LMI_FanSensor_Set_SystemCreationClassName(&w, get_system_creation_class_name());
        LMI_FanSensor_Set_SystemName(&w, get_system_name());
        LMI_FanSensor_Set_DeviceID(&w, sptr->device_id);

        LMI_FanSensor_Set_Caption(&w, "Computer's fan");
        LMI_FanSensor_Set_Description(&w,"Computer's fan.");
        snprintf(buf, 200, "Fan \"%s\" on chip \"%s\"", sptr->name, sptr->chip_name);
        LMI_FanSensor_Set_ElementName(&w, buf);

        // ManagedSystemElement
        LMI_FanSensor_Set_Name(&w, sptr->name);

        LMI_FanSensor_Init_OperationalStatus(&w, 2);
        LMI_FanSensor_Set_OperationalStatus(&w, 0, sptr->fault ?
                LMI_FanSensor_OperationalStatus_Error :
                LMI_FanSensor_OperationalStatus_OK);
        if (sptr->alarm || sptr->alarm_min || sptr->alarm_max) {
            LMI_FanSensor_Set_OperationalStatus(&w, 1, LMI_FanSensor_OperationalStatus_Stressed);
        }

        LMI_FanSensor_Init_StatusDescriptions(&w, 2);
        LMI_FanSensor_Set_StatusDescriptions(&w, 0, sptr->fault ?
                "Chip indicates, that fan is in fault state."
                " Possible causes are open diodes, unconnected fan etc."
                " Thus the measurement for this channel should not be trusted."
                : "Fan seems to be functioning correctly.");
        if (sptr->alarm || sptr->alarm_min || sptr->alarm_max) {
            snprintf(buf, 200, "These alarm flags are set by the fan's chip:"
                     "  alarm=%s, min_alarm=%s, max_alarm=%s",
                     sptr->alarm ? "1":"0",
                     sptr->alarm_min ? "1":"0",
                     sptr->alarm_max ? "1":"0");
            LMI_FanSensor_Set_StatusDescriptions(&w, 1, buf);
        }

        LMI_FanSensor_Set_HealthState(&w, sptr->fault ?
                LMI_FanSensor_HealthState_Major_failure :
                LMI_FanSensor_HealthState_OK);

        LMI_FanSensor_Set_OperatingStatus(&w, sptr->fault ?
                LMI_FanSensor_OperatingStatus_Stopped :
                LMI_FanSensor_OperatingStatus_In_Service);

        LMI_FanSensor_Set_PrimaryStatus(&w, sptr->fault ?
                LMI_FanSensor_PrimaryStatus_Error :
                LMI_FanSensor_PrimaryStatus_OK);

        // EnabledLogicalElement
        LMI_FanSensor_Init_OtherIdentifyingInfo(&w, 2);
        LMI_FanSensor_Set_OtherIdentifyingInfo(&w, 0, sptr->chip_name);
        LMI_FanSensor_Set_OtherIdentifyingInfo(&w, 1, sptr->sys_path);

        LMI_FanSensor_Init_IdentifyingDescriptions(&w, 2);
        LMI_FanSensor_Set_IdentifyingDescriptions(&w, 0, "ChipName - name of fan's chip.");
        LMI_FanSensor_Set_IdentifyingDescriptions(&w, 1, "SysPath - system path of fan's chip.");

        // ManagedElement
        LMI_FanSensor_Set_Caption(&w, "Fan's tachometer");
        LMI_FanSensor_Set_Description(&w,"Associated sensor of fan. Giving information about its speed.");

        snprintf(buf, 200, "Tachometer of fan \"%s\" on chip \"%s\"", sptr->name, sptr->chip_name);
        LMI_FanSensor_Set_ElementName(&w, buf);

        // Sensor
        LMI_FanSensor_Set_SensorType(&w, LMI_FanSensor_SensorType_Tachometer);
        LMI_FanSensor_Set_CurrentState(&w, fan_get_current_state(sptr));

        LMI_FanSensor_Init_PossibleStates(&w, 5);
        int index = 0;
        if (sptr->accessible_features & CIM_FAN_AF_MIN_SPEED) {
            LMI_FanSensor_Set_PossibleStates(&w, index++, "Below Minimum");
            LMI_FanSensor_Set_PossibleStates(&w, index++, "At Minimum");
        }
        LMI_FanSensor_Set_PossibleStates(&w, index++, "Normal");
        if (sptr->accessible_features & CIM_FAN_AF_MAX_SPEED) {
            LMI_FanSensor_Set_PossibleStates(&w, index++, "At Maximum");
            LMI_FanSensor_Set_PossibleStates(&w, index++, "Above Maximum");
        }

        // NumericSensor
        LMI_FanSensor_Set_BaseUnits(&w, LMI_FanSensor_BaseUnits_Revolutions);
        LMI_FanSensor_Set_UnitModifier(&w, 0);
        LMI_FanSensor_Set_RateUnits(&w, LMI_FanSensor_RateUnits_Per_Minute);
        LMI_FanSensor_Set_CurrentReading(&w, sptr->speed);
        if (sptr->accessible_features & CIM_FAN_AF_MAX_SPEED) {
            LMI_FanSensor_Set_NormalMax(&w, sptr->max_speed);
        }
        if (sptr->accessible_features & CIM_FAN_AF_MIN_SPEED) {
            LMI_FanSensor_Set_NormalMin(&w, sptr->min_speed);
        }
        LMI_FanSensor_Set_MinReadable(&w, 0);
        LMI_FanSensor_Set_IsLinear(&w, true);

        status = __KReturnInstance((cr), &(w).__base);
        if (!KOkay(status)) {
            free_fanlist(fans);
            return status;
        }

        lptr = lptr->next;
    }

    free_fanlist(fans);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FanSensorGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_FanSensorCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FanSensorModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FanSensorDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FanSensorExecQuery(
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
    LMI_FanSensor,
    LMI_FanSensor,
    _cb,
    LMI_FanSensorInitialize(ctx))

static CMPIStatus LMI_FanSensorMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FanSensorInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_FanSensor_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_FanSensor,
    LMI_FanSensor,
    _cb,
    LMI_FanSensorInitialize(ctx))

KUint32 LMI_FanSensor_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanSensorRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_FanSensor_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanSensorRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_FanSensor_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanSensorRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_FanSensor_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanSensorRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_FanSensor_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanSensorRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_FanSensor_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanSensorRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_FanSensor_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanSensorRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_FanSensor_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanSensorRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_FanSensor_RestoreDefaultThresholds(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanSensorRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_FanSensor_GetNonLinearFactors(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanSensorRef* self,
    const KSint32* SensorReading,
    KSint32* Accuracy,
    KUint32* Resolution,
    KSint32* Tolerance,
    KUint32* Hysteresis,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_FanSensor",
    "LMI_FanSensor",
    "instance method")
