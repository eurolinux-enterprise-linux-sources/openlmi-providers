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

#include <konkret/konkret.h>
#include "LMI_PowerManagementCapabilities.h"

#include "power.h"

static const CMPIBroker* _cb = NULL;

static void LMI_PowerManagementCapabilitiesInitialize(CMPIInstanceMI *mi,
        const CMPIContext *ctx)
{
    mi->hdl = power_ref(_cb, ctx);
}

static void LMI_PowerManagementCapabilitiesInitializeMethod(CMPIMethodMI *mi,
        const CMPIContext *ctx)
{
    mi->hdl = power_ref(_cb, ctx);
}

static CMPIStatus LMI_PowerManagementCapabilitiesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    power_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PowerManagementCapabilitiesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PowerManagementCapabilitiesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    const char *ns = KNameSpace(cop);

    LMI_PowerManagementCapabilities w;
    LMI_PowerManagementCapabilities_Init(&w, _cb, ns);
    // TODO: make it unique
    LMI_PowerManagementCapabilities_Set_InstanceID(&w, ORGID ":LMI_PowerManagementCapabilities");
    LMI_PowerManagementCapabilities_Set_ElementName(&w, "PowerManagementCapabilities");
    LMI_PowerManagementCapabilities_Set_Caption(&w, "Power Management Capabilities");

    int count;
    unsigned short *list = power_available_requested_power_states(mi->hdl, &count);
    if (list == NULL) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    LMI_PowerManagementCapabilities_Init_PowerStatesSupported(&w, count);
    for (int i = 0; i < count; i++) {
        LMI_PowerManagementCapabilities_Set_PowerStatesSupported(&w, i, list[i]);
    }
    free(list);

    // TODO: get this list dynamically from PowerStatesSupported (see SMASH)
    LMI_PowerManagementCapabilities_Init_PowerChangeCapabilities(&w, 3);
    LMI_PowerManagementCapabilities_Set_PowerChangeCapabilities(&w, 0, LMI_PowerManagementCapabilities_PowerChangeCapabilities_Power_State_Settable);
    LMI_PowerManagementCapabilities_Set_PowerChangeCapabilities(&w, 1, LMI_PowerManagementCapabilities_PowerChangeCapabilities_Power_Cycling_Supported);
    LMI_PowerManagementCapabilities_Set_PowerChangeCapabilities(&w, 2, LMI_PowerManagementCapabilities_PowerChangeCapabilities_Graceful_Shutdown_Supported);
    KReturnInstance(cr, w);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PowerManagementCapabilitiesGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PowerManagementCapabilitiesCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PowerManagementCapabilitiesModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PowerManagementCapabilitiesDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PowerManagementCapabilitiesExecQuery(
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
    LMI_PowerManagementCapabilities,
    LMI_PowerManagementCapabilities,
    _cb,
    LMI_PowerManagementCapabilitiesInitialize(&mi, ctx))

static CMPIStatus LMI_PowerManagementCapabilitiesMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    power_unref(mi->hdl);
    mi->hdl = NULL;

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PowerManagementCapabilitiesInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_PowerManagementCapabilities_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_PowerManagementCapabilities,
    LMI_PowerManagementCapabilities,
    _cb,
    LMI_PowerManagementCapabilitiesInitializeMethod(&mi, ctx))

KUint16 LMI_PowerManagementCapabilities_CreateGoalSettings(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerManagementCapabilitiesRef* self,
    const KInstanceA* TemplateGoalSettings,
    KInstanceA* SupportedGoalSettings,
    CMPIStatus* status)
{
    KUint16 result = KUINT16_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PowerManagementCapabilities",
    "LMI_PowerManagementCapabilities",
    "instance method")
