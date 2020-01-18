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

#include <stdlib.h>
#include <string.h>
#include <konkret/konkret.h>
#include <sss_sifp_dbus.h>
#include "LMI_SSSDBackendProvider.h"
#include "utils.h"

static const CMPIBroker* _cb;

static void LMI_SSSDBackendProviderInitialize(const CMPIContext *ctx)
{
    lmi_init(PROVIDER_NAME, _cb, ctx, NULL);
}

static CMPIStatus LMI_SSSDBackendProviderCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDBackendProviderEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SSSDBackendProviderEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_SSSDBackendProvider association;
    LMI_SSSDBackendRef ref_backend;
    LMI_SSSDProviderRef ref_provider;
    const char *namespace = KNameSpace(cop);
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_attr **attrs = NULL;
    sss_sifp_error error;
    char **paths = NULL;
    const char *backend_name = NULL;
    const char * const *providers = NULL;
    char *provider_type = NULL;
    char *provider_module = NULL;
    unsigned int num_providers;
    CMPIrc ret;
    unsigned int j;
    int i;

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    error = sss_sifp_invoke_list(sifp_ctx, SSSD_DBUS_LIST_BACKENDS, &paths,
                                 DBUS_TYPE_INVALID);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    for (i = 0; paths[i] != NULL; i++) {
        error = sss_sifp_fetch_all_attrs(sifp_ctx, paths[i],
                                         SSS_SIFP_IFACE_COMPONENTS, &attrs);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        error = sss_sifp_find_attr_as_string(attrs, "name", &backend_name);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        error = sss_sifp_find_attr_as_string_array(attrs, "providers",
                                                   &num_providers, &providers);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        if (num_providers == 0) {
            sss_sifp_free_attrs(sifp_ctx, &attrs);
            continue;
        }

        LMI_SSSDBackendRef_Init(&ref_backend, _cb, namespace);
        LMI_SSSDBackendRef_Set_Name(&ref_backend, backend_name);

        for (j = 0; j < num_providers; j++) {
            /* provider is a key=value pair */
            provider_type = strdup(providers[j]);
            if (provider_type == NULL) {
                goto done;
            }

            provider_module = strchr(provider_type, '=');
            if (provider_module == NULL) {
                provider_module = "";
            } else {
                *provider_module = '\0';
                provider_module++;
            }

            LMI_SSSDProviderRef_Init(&ref_provider, _cb, namespace);
            LMI_SSSDProviderRef_Set_Type(&ref_provider, provider_type);
            LMI_SSSDProviderRef_Set_Module(&ref_provider, provider_module);
            free(provider_type);

            LMI_SSSDBackendProvider_Init(&association, _cb, namespace);
            LMI_SSSDBackendProvider_Set_Backend(&association, &ref_backend);
            LMI_SSSDBackendProvider_Set_Provider(&association, &ref_provider);

            KReturnInstance(cr, association);
        }

        sss_sifp_free_attrs(sifp_ctx, &attrs);
    }

    ret = CMPI_RC_OK;

done:
    if (attrs != NULL) {
        sss_sifp_free_attrs(sifp_ctx, &attrs);
    }

    sss_sifp_free_string_array(sifp_ctx, &paths);
    sss_sifp_free(&sifp_ctx);
    CMReturn(ret);
}

static CMPIStatus LMI_SSSDBackendProviderGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SSSDBackendProviderCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendProviderModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendProviderDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendProviderExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendProviderAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDBackendProviderAssociators(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole,
    const char** properties)
{
    return KDefaultAssociators(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_SSSDBackendProvider_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_SSSDBackendProviderAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return KDefaultAssociatorNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_SSSDBackendProvider_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_SSSDBackendProviderReferences(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    return KDefaultReferences(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_SSSDBackendProvider_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_SSSDBackendProviderReferenceNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role)
{
    return KDefaultReferenceNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_SSSDBackendProvider_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_SSSDBackendProvider,
    LMI_SSSDBackendProvider,
    _cb,
    LMI_SSSDBackendProviderInitialize(ctx))

CMAssociationMIStub( 
    LMI_SSSDBackendProvider,
    LMI_SSSDBackendProvider,
    _cb,
    LMI_SSSDBackendProviderInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SSSDBackendProvider",
    "LMI_SSSDBackendProvider",
    "instance association")
