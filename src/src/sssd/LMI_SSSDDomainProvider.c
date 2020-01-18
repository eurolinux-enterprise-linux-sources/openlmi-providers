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
#include "LMI_SSSDDomain.h"
#include "utils.h"
#include "sssd_domains.h"

static const CMPIBroker* _cb = NULL;

static void LMI_SSSDDomainInitialize(const CMPIContext *ctx)
{
    lmi_init(PROVIDER_NAME, _cb, ctx, NULL);
}

static CMPIStatus LMI_SSSDDomainCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDDomainEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SSSDDomainEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_SSSDDomain instance;
    const char *namespace = KNameSpace(cop);
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_error error;
    char **paths = NULL;
    sssd_method_error mret;
    CMPIrc ret;
    int i;

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    error = sss_sifp_invoke_list(sifp_ctx, SSSD_DBUS_LIST_DOMAINS, &paths,
                                 DBUS_TYPE_INVALID);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    for (i = 0; paths[i] != NULL; i++) {
        mret = sssd_domain_set_instance(sifp_ctx, paths[i], _cb, namespace,
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

static CMPIStatus LMI_SSSDDomainGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_SSSDDomain instance;
    LMI_SSSDDomainRef ref;
    const char *namespace = KNameSpace(cop);
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_error error;
    const char *name = NULL;
    char *path = NULL;
    sssd_method_error mret;
    CMPIrc ret;

    LMI_SSSDDomainRef_InitFromObjectPath(&ref, _cb, cop);
    name = ref.Name.chars;

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    error = sss_sifp_invoke_find(sifp_ctx, SSSD_DBUS_FIND_DOMAIN, &path,
                                 DBUS_TYPE_STRING, &name,
                                 DBUS_TYPE_INVALID);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_NOT_FOUND, done);

    mret = sssd_domain_set_instance(sifp_ctx, path, _cb, namespace,
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

static CMPIStatus LMI_SSSDDomainCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDDomainModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDDomainDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDDomainExecQuery(
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
    LMI_SSSDDomain,
    LMI_SSSDDomain,
    _cb,
    LMI_SSSDDomainInitialize(ctx))

static CMPIStatus LMI_SSSDDomainMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDDomainInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SSSDDomain_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SSSDDomain,
    LMI_SSSDDomain,
    _cb,
    LMI_SSSDDomainInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SSSDDomain",
    "LMI_SSSDDomain",
    "instance method")
