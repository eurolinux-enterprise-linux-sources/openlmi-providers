/*
 * Copyright (C) 2014 Red Hat, Inc.  All rights reserved.
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
 * Authors: Pavel Březina <pbrezina@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_SSSDProvider.h"
#include "utils.h"

static const CMPIBroker* _cb = NULL;

static void LMI_SSSDProviderInitialize(const CMPIContext *ctx)
        {
            lmi_init(PROVIDER_NAME, _cb, ctx, NULL);
        }

static CMPIStatus LMI_SSSDProviderCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDProviderEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SSSDProviderEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    /* no instances to enumerate */

    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDProviderGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    const char *namespace = KNameSpace(cop);
    LMI_SSSDProvider instance;
    LMI_SSSDProviderRef ref;
    CMPIStatus ret;

    ret = LMI_SSSDProviderRef_InitFromObjectPath(&ref, _cb, cop);
    if (!KOkay(ret)) {
        return ret;
    }

    LMI_SSSDProvider_Init(&instance, _cb, namespace);
    LMI_SSSDProvider_Set_Type(&instance, ref.Type.chars);
    LMI_SSSDProvider_Set_Module(&instance, ref.Module.chars);

    LMI_SSSDProvider_Set_Caption(&instance, "SSSD Provider Module");
    LMI_SSSDProvider_Set_Description(&instance, "SSSD Provider Module");
    LMI_SSSDProvider_Set_ElementName(&instance, ref.Type.chars);

    KReturnInstance(cr, instance);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDProviderCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDProviderModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDProviderDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDProviderExecQuery(
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
    LMI_SSSDProvider,
    LMI_SSSDProvider,
    _cb,
    LMI_SSSDProviderInitialize(ctx))

static CMPIStatus LMI_SSSDProviderMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDProviderInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SSSDProvider_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SSSDProvider,
    LMI_SSSDProvider,
    _cb,
    LMI_SSSDProviderInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SSSDProvider",
    "LMI_SSSDProvider",
    "instance method")
