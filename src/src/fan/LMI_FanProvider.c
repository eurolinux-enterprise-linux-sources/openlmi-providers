/*
 * Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
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
#include <stdint.h>
#include "LMI_Fan.h"
#include "fan.h"

static const CMPIBroker* _cb = NULL;

static void LMI_FanInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
    init_linux_fan_module();
}

static CMPIStatus LMI_FanCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FanEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_FanEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    const char *ns = KNameSpace(cop);
    CMPIStatus status;

    char buf[BUFLEN];
    struct fanlist *lptr = NULL;
    struct fanlist *fans = NULL;
    struct cim_fan *sptr;

    if (enum_all_fans(&fans)) {
        KReturn2(_cb, ERR_FAILED, "Could not list fans.");
    }

    // iterate fan list
    lptr = fans;
    while (lptr != NULL) {
        sptr = lptr->f;
        LMI_Fan w;
        LMI_Fan_Init(&w, _cb, ns);
        LMI_Fan_Set_CreationClassName(&w, "LMI_Fan");
        LMI_Fan_Set_SystemCreationClassName(&w, lmi_get_system_creation_class_name());
        LMI_Fan_Set_SystemName(&w, lmi_get_system_name_safe(cc));
        LMI_Fan_Set_DeviceID(&w, sptr->device_id);

        LMI_Fan_Set_Caption(&w, "Computer's fan");
        LMI_Fan_Set_Description(&w,"Computer's fan.");
        snprintf(buf, BUFLEN, "Fan \"%s\" on chip \"%s\"", sptr->name, sptr->chip_name);
        LMI_Fan_Set_ElementName(&w, buf);

        // ManagedSystemElement
        LMI_Fan_Set_Name(&w, sptr->name);
        LMI_Fan_Init_OperationalStatus(&w, 2);
        LMI_Fan_Set_OperationalStatus(&w, 0, sptr->fault ?
                LMI_Fan_OperationalStatus_Error :
                LMI_Fan_OperationalStatus_OK);
        if (sptr->alarm || sptr->alarm_min || sptr->alarm_max) {
            LMI_Fan_Set_OperationalStatus(&w, 1, LMI_Fan_OperationalStatus_Stressed);
        }

        LMI_Fan_Init_StatusDescriptions(&w, 2);
        LMI_Fan_Set_StatusDescriptions(&w, 0, sptr->fault ?
                "Chip indicates, that fan is in fault state."
                " Possible causes are open diodes, unconnected fan etc."
                " Thus the measurement for this channel should not be trusted."
                : "Fan seems to be functioning correctly.");
        if (sptr->alarm || sptr->alarm_min || sptr->alarm_max) {
            snprintf(buf, BUFLEN, "These alarm flags are set by the fan's chip:"
                     "  alarm=%s, min_alarm=%s, max_alarm=%s",
                     sptr->alarm ? "1":"0",
                     sptr->alarm_min ? "1":"0",
                     sptr->alarm_max ? "1":"0");
            LMI_Fan_Set_StatusDescriptions(&w, 1, buf);
        }


        LMI_Fan_Set_HealthState(&w, sptr->fault ?
                LMI_Fan_HealthState_Major_failure :
                LMI_Fan_HealthState_OK);

        LMI_Fan_Set_OperatingStatus(&w, sptr->fault ?
                LMI_Fan_OperatingStatus_Stopped :
                LMI_Fan_OperatingStatus_In_Service);

        LMI_Fan_Set_PrimaryStatus(&w, sptr->fault ?
                LMI_Fan_PrimaryStatus_Error :
                LMI_Fan_PrimaryStatus_OK);

        // EnabledLogicalElement
        LMI_Fan_Init_OtherIdentifyingInfo(&w, 2);
        LMI_Fan_Set_OtherIdentifyingInfo(&w, 0, sptr->chip_name);
        LMI_Fan_Set_OtherIdentifyingInfo(&w, 1, sptr->sys_path);

        LMI_Fan_Init_IdentifyingDescriptions(&w, 2);
        LMI_Fan_Set_IdentifyingDescriptions(&w, 0, "ChipName - name of fan's chip.");
        LMI_Fan_Set_IdentifyingDescriptions(&w, 1, "SysPath - system path of fan's chip.");

        LMI_Fan_Set_ActiveCooling(&w, true);

        uint32_t i = 1;
        int index = 0;
        lmi_debug("accessible_features: %d", sptr->accessible_features);
        LMI_Fan_Init_AccessibleFeatures(&w, 8);
        while (i <= CIM_FAN_AF_FEATURE_MAX) {
            if (i & sptr->accessible_features) {
                LMI_Fan_Set_AccessibleFeatures(&w, index++, i);
            }
            i = i << 1;
        }
        if (sptr->accessible_features & CIM_FAN_AF_MIN_SPEED) {
            LMI_Fan_Set_MinSpeed(&w, (uint64_t) sptr->min_speed);
        }
        if (sptr->accessible_features & CIM_FAN_AF_MAX_SPEED) {
            LMI_Fan_Set_MaxSpeed(&w, (uint64_t) sptr->max_speed);
        }
        if (sptr->accessible_features & CIM_FAN_AF_DIV) {
            LMI_Fan_Set_Divisor(&w, sptr->divisor);
        }
        if (sptr->accessible_features & CIM_FAN_AF_PULSES) {
            LMI_Fan_Set_Pulses(&w, sptr->pulses);
        }
        if (sptr->accessible_features & CIM_FAN_AF_BEEP) {
            LMI_Fan_Set_Beep(&w, sptr->beep);
        }
        if (sptr->accessible_features & CIM_FAN_AF_ALARM) {
            LMI_Fan_Set_Alarm(&w, sptr->alarm);
        }
        if (sptr->accessible_features & CIM_FAN_AF_ALARM_MIN) {
            LMI_Fan_Set_MinAlarm(&w, sptr->alarm_min);
        }
        if (sptr->accessible_features & CIM_FAN_AF_ALARM_MAX) {
            LMI_Fan_Set_MaxAlarm(&w, sptr->alarm_max);
        }

        status = __KReturnInstance((cr), &(w).__base);
        if (!KOkay(status)) {
            free_fanlist(fans);
            return status;
        }

        lptr = lptr->next;
    }
    free_fanlist(fans);
    KReturn(OK);
}

static CMPIStatus LMI_FanGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_FanCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FanModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FanDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FanExecQuery(
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
    LMI_Fan,
    LMI_Fan,
    _cb,
    LMI_FanInitialize(ctx))

static CMPIStatus LMI_FanMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FanInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Fan_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_Fan,
    LMI_Fan,
    _cb,
    LMI_FanInitialize(ctx))

KUint32 LMI_Fan_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Fan_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Fan_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Fan_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Fan_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Fan_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Fan_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Fan_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Fan_SetSpeed(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_FanRef* self,
    const KUint64* DesiredSpeed,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_Fan",
    "LMI_Fan",
    "instance method")
