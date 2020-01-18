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
#include <dbus/dbus.h>
#include <sss_sifp_dbus.h>
#include "LMI_SSSDMonitor.h"
#include "utils.h"
#include "sssd_components.h"

static const CMPIBroker* _cb = NULL;

static void LMI_SSSDMonitorInitialize(const CMPIContext *ctx)
{
    lmi_init(PROVIDER_NAME, _cb, ctx, NULL);
}

static CMPIStatus LMI_SSSDMonitorCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDMonitorEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SSSDMonitorEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_SSSDMonitor instance;
    const char *namespace = KNameSpace(cop);
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_error error;
    char *path = NULL;
    sssd_method_error mret;
    CMPIrc ret;

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    error = sss_sifp_invoke_find(sifp_ctx, "Monitor", &path, DBUS_TYPE_INVALID);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_NOT_FOUND, done);

    mret = sssd_monitor_set_instance(sifp_ctx, path, _cb, namespace,
                                     &instance);
    if (mret != SSSD_METHOD_ERROR_OK) {
        ret = CMPI_RC_ERR_FAILED;
        goto done;
    }

    KReturnInstance(cr, instance);
    ret = CMPI_RC_OK;

done:
    sss_sifp_free_string(sifp_ctx, &path);
    sss_sifp_free(&sifp_ctx);
    CMReturn(ret);
}

static CMPIStatus LMI_SSSDMonitorGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SSSDMonitorCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDMonitorModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDMonitorDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDMonitorExecQuery(
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
    LMI_SSSDMonitor,
    LMI_SSSDMonitor,
    _cb,
    LMI_SSSDMonitorInitialize(ctx))

static CMPIStatus LMI_SSSDMonitorMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDMonitorInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SSSDMonitor_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SSSDMonitor,
    LMI_SSSDMonitor,
    _cb,
    LMI_SSSDMonitorInitialize(ctx))

KUint32 LMI_SSSDMonitor_SetDebugLevelPermanently(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SSSDMonitorRef* self,
    const KUint16* debug_level,
    CMPIStatus* status)
{
    return sssd_component_set_debug_permanently(self->Name.chars,
                                                SSSD_COMPONENT_MONITOR,
                                                debug_level, status);
}

KUint32 LMI_SSSDMonitor_SetDebugLevelTemporarily(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SSSDMonitorRef* self,
    const KUint16* debug_level,
    CMPIStatus* status)
{
    return sssd_component_set_debug_temporarily(self->Name.chars,
                                                SSSD_COMPONENT_MONITOR,
                                                debug_level, status);
}

KUint32 LMI_SSSDMonitor_Enable(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SSSDMonitorRef* self,
    CMPIStatus* status)
{
    return sssd_component_enable(self->Name.chars, SSSD_COMPONENT_MONITOR,
                                 status);
}

KUint32 LMI_SSSDMonitor_Disable(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SSSDMonitorRef* self,
    CMPIStatus* status)
{
    return sssd_component_disable(self->Name.chars, SSSD_COMPONENT_MONITOR,
                                  status);
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SSSDMonitor",
    "LMI_SSSDMonitor",
    "instance method")
