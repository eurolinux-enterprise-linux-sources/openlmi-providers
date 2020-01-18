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
#include <sss_sifp_dbus.h>
#include "LMI_SSSDBackend.h"
#include "utils.h"
#include "sssd_components.h"

static const CMPIBroker* _cb = NULL;

static void LMI_SSSDBackendInitialize(const CMPIContext *ctx)
{
    lmi_init(PROVIDER_NAME, _cb, ctx, NULL);
}

static CMPIStatus LMI_SSSDBackendCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDBackendEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SSSDBackendEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_SSSDBackend instance;
    const char *namespace = KNameSpace(cop);
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_error error;
    char **paths = NULL;
    sssd_method_error mret;
    CMPIrc ret;
    int i;

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    error = sss_sifp_invoke_list(sifp_ctx, SSSD_DBUS_LIST_BACKENDS, &paths,
                                 DBUS_TYPE_INVALID);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    for (i = 0; paths[i] != NULL; i++) {
        mret = sssd_backend_set_instance(sifp_ctx, paths[i], _cb, namespace,
                                         &instance);
        if (mret != SSSD_METHOD_ERROR_OK) {
            ret = CMPI_RC_ERR_FAILED;
            goto done;
        }

        KReturnInstance(cr, instance);
    }

    ret = CMPI_RC_OK;

done:
    sss_sifp_free_string_array(sifp_ctx, &paths);
    sss_sifp_free(&sifp_ctx);
    CMReturn(ret);
}

static CMPIStatus LMI_SSSDBackendGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_SSSDBackend instance;
    LMI_SSSDBackendRef ref;
    const char *namespace = KNameSpace(cop);
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_error error;
    const char *name = NULL;
    char *path = NULL;
    sssd_method_error mret;
    CMPIrc ret;

    LMI_SSSDBackendRef_InitFromObjectPath(&ref, _cb, cop);
    name = ref.Name.chars;

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    error = sss_sifp_invoke_find(sifp_ctx, SSSD_DBUS_FIND_BACKEND, &path,
                                 DBUS_TYPE_STRING, &name,
                                 DBUS_TYPE_INVALID);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_NOT_FOUND, done);

    mret = sssd_backend_set_instance(sifp_ctx, path, _cb, namespace,
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

static CMPIStatus LMI_SSSDBackendCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendExecQuery(
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
    LMI_SSSDBackend,
    LMI_SSSDBackend,
    _cb,
    LMI_SSSDBackendInitialize(ctx))

static CMPIStatus LMI_SSSDBackendMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDBackendInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SSSDBackend_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SSSDBackend,
    LMI_SSSDBackend,
    _cb,
    LMI_SSSDBackendInitialize(ctx))

KUint32 LMI_SSSDBackend_SetDebugLevelPermanently(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SSSDBackendRef* self,
    const KUint16* debug_level,
    CMPIStatus* status)
{
    return sssd_component_set_debug_permanently(self->Name.chars,
                                                SSSD_COMPONENT_BACKEND,
                                                debug_level, status);
}

KUint32 LMI_SSSDBackend_SetDebugLevelTemporarily(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SSSDBackendRef* self,
    const KUint16* debug_level,
    CMPIStatus* status)
{
    return sssd_component_set_debug_temporarily(self->Name.chars,
                                                SSSD_COMPONENT_BACKEND,
                                                debug_level, status);
}

KUint32 LMI_SSSDBackend_Enable(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SSSDBackendRef* self,
    CMPIStatus* status)
{
    return sssd_component_enable(self->Name.chars, SSSD_COMPONENT_BACKEND,
                                 status);
}

KUint32 LMI_SSSDBackend_Disable(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SSSDBackendRef* self,
    CMPIStatus* status)
{
    return sssd_component_disable(self->Name.chars, SSSD_COMPONENT_BACKEND,
                                  status);
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SSSDBackend",
    "LMI_SSSDBackend",
    "instance method")
