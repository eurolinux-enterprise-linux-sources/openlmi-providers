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
 * Authors: Roman Rakus <rrakus@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_EnabledAccountCapabilities.h"

#include "macros.h"
#include "account_globals.h"
#include "globals.h"

#include <stdbool.h>

static const CMPIBroker* _cb = NULL;

static void LMI_EnabledAccountCapabilitiesInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_EnabledAccountCapabilitiesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_EnabledAccountCapabilitiesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_EnabledAccountCapabilitiesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_EnabledAccountCapabilities leac;

    const char *nameSpace = KNameSpace(cop);

    LMI_EnabledAccountCapabilities_Init(&leac, _cb, nameSpace);
    LMI_EnabledAccountCapabilities_Set_InstanceID(&leac, ORGID":"LEACNAME);
    LMI_EnabledAccountCapabilities_Set_ElementName(&leac, LEACNAME);
    LMI_EnabledAccountCapabilities_Init_RequestedStatesSupported(&leac, 0);
    LMI_EnabledAccountCapabilities_Set_ElementNameEditSupported(&leac,
      false);

    KReturnInstance(cr, leac);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_EnabledAccountCapabilitiesGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_EnabledAccountCapabilitiesCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_EnabledAccountCapabilitiesModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_EnabledAccountCapabilitiesDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_EnabledAccountCapabilitiesExecQuery(
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
    LMI_EnabledAccountCapabilities,
    LMI_EnabledAccountCapabilities,
    _cb,
    LMI_EnabledAccountCapabilitiesInitialize(ctx))

static CMPIStatus LMI_EnabledAccountCapabilitiesMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_EnabledAccountCapabilitiesInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_EnabledAccountCapabilities_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_EnabledAccountCapabilities,
    LMI_EnabledAccountCapabilities,
    _cb,
    LMI_EnabledAccountCapabilitiesInitialize(ctx))

KUint16 LMI_EnabledAccountCapabilities_CreateGoalSettings(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_EnabledAccountCapabilitiesRef* self,
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
    "LMI_EnabledAccountCapabilities",
    "LMI_EnabledAccountCapabilities",
    "instance method")
