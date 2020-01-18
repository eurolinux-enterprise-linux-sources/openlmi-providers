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
 * Authors: Radek Novacek <rnovacek@redhat.com>
 */

#include "LMI_PowerManagementService.h"

#include "power.h"
#include "globals.h"

static const CMPIBroker* _cb = NULL;

static void LMI_PowerManagementServiceInitialize(CMPIInstanceMI *mi,
        const CMPIContext *ctx)
{
    mi->hdl = power_ref(_cb, ctx);
}

static void LMI_PowerManagementServiceMethodInitialize(CMPIMethodMI *mi,
        const CMPIContext *ctx)
{
    mi->hdl = power_ref(_cb, ctx);
}


static CMPIStatus LMI_PowerManagementServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    power_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PowerManagementServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PowerManagementServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_PowerManagementService w;

    LMI_PowerManagementService_Init(&w, _cb, KNameSpace(cop));
    LMI_PowerManagementService_Set_CreationClassName(&w, "LMI_PowerManagementService");
    LMI_PowerManagementService_Set_Name(&w, get_system_name());
    LMI_PowerManagementService_Set_SystemCreationClassName(&w, get_system_creation_class_name());
    LMI_PowerManagementService_Set_SystemName(&w, get_system_name());

    /* EnabledState is an integer enumeration that indicates the enabled
     * and disabled states of an element. It can also indicate the transitions
     * between these requested states.
     */
    LMI_PowerManagementService_Set_EnabledState(&w, LMI_PowerManagementService_EnabledDefault_Enabled);


    /* RequestedState is an integer enumeration that indicates the last
     * requested or desired state for the element, irrespective of the mechanism
     * through which it was requested. The actual state of the element is
     * represented by EnabledState. This property is provided to compare the
     * last requested and current enabled or disabled states.
     */
    LMI_PowerManagementService_Set_RequestedState(&w, LMI_PowerManagementService_RequestedState_No_Change);

    LMI_PowerManagementService_Init_AvailableRequestedStates(&w, 2);
    LMI_PowerManagementService_Set_AvailableRequestedStates(&w, 0, 2); // Enabled
    LMI_PowerManagementService_Set_AvailableRequestedStates(&w, 1, 3); // Disabled

    KReturnInstance(cr, w);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PowerManagementServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PowerManagementServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PowerManagementServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PowerManagementServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PowerManagementServiceExecQuery(
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
    LMI_PowerManagementService,
    LMI_PowerManagementService,
    _cb,
    LMI_PowerManagementServiceInitialize(&mi, ctx))

static CMPIStatus LMI_PowerManagementServiceMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    power_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PowerManagementServiceInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_PowerManagementService_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_PowerManagementService,
    LMI_PowerManagementService,
    _cb,
    LMI_PowerManagementServiceMethodInitialize(&mi, ctx))

KUint32 LMI_PowerManagementService_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerManagementServiceRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;

}

KUint32 LMI_PowerManagementService_StartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerManagementServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PowerManagementService_StopService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerManagementServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PowerManagementService_ChangeAffectedElementsAssignedSequence(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerManagementServiceRef* self,
    const KRefA* ManagedElements,
    const KUint16A* AssignedSequence,
    KRef* Job,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PowerManagementService_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerManagementServiceRef* self,
    const KUint16* PowerState,
    const KRef* ManagedElement,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PowerManagementService_RequestPowerStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerManagementServiceRef* self,
    const KUint16* PowerState,
    const KRef* ManagedElement,
    const KDateTime* Time,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    if (Time->exists && Time->null && TimeoutPeriod->exists && TimeoutPeriod->null) {
        /* SMASH says: The TimeoutPeriod and Time parameters shall not be
         * supported for the same invocation of the RequestPowerStateChange( )
         * method. When the TimeoutPeriod and Time parameters are specified
         * for the same method invocation, the method shall return a value of 2.
         */
        KUint32_Set(&result, 2);
        return result;
    }

    // Time argument is not handled because we don't support powering on systems

    if (!PowerState->exists || PowerState->null) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "PowerState argument is missing");
        return result;
    }
    status->rc = power_request_power_state(mi->hdl, PowerState->value);
    if (status->rc != CMPI_RC_OK) {
        KUint32_Set(&result, 4); // Failed
    }
    KUint32_Set(&result, 4096);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PowerManagementService",
    "LMI_PowerManagementService",
    "instance method")
