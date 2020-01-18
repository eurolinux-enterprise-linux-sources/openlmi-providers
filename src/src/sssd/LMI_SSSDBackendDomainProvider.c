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
#include "LMI_SSSDBackendDomain.h"
#include "utils.h"

static const CMPIBroker* _cb;

static void LMI_SSSDBackendDomainInitialize(const CMPIContext *ctx)
{
    lmi_init(PROVIDER_NAME, _cb, ctx, NULL);
}

static CMPIStatus LMI_SSSDBackendDomainCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDBackendDomainEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SSSDBackendDomainEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    const char *namespace = KNameSpace(cop);
    LMI_SSSDBackendDomain association;
    LMI_SSSDBackendRef ref_backend;
    LMI_SSSDDomainRef ref_domain;
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_attr **attrs = NULL;
    sss_sifp_error error;
    char **paths = NULL;
    const char *name = NULL;
    bool is_subdomain;
    CMPIrc ret;
    int i;

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    error = sss_sifp_invoke_list(sifp_ctx, SSSD_DBUS_LIST_DOMAINS, &paths,
                                 DBUS_TYPE_INVALID);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    for (i = 0; paths[i] != NULL; i++) {
        error = sss_sifp_fetch_all_attrs(sifp_ctx, paths[i],
                                         SSS_SIFP_IFACE_DOMAINS, &attrs);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        error = sss_sifp_find_attr_as_string(attrs, "name", &name);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        error = sss_sifp_find_attr_as_bool(attrs, "subdomain", &is_subdomain);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        if (!is_subdomain) {
            LMI_SSSDBackendRef_Init(&ref_backend, _cb, namespace);
            LMI_SSSDBackendRef_Set_Name(&ref_backend, name);

            LMI_SSSDDomainRef_Init(&ref_domain, _cb, namespace);
            LMI_SSSDDomainRef_Set_Name(&ref_domain, name);

            LMI_SSSDBackendDomain_Init(&association, _cb, namespace);
            LMI_SSSDBackendDomain_Set_Backend(&association, &ref_backend);
            LMI_SSSDBackendDomain_Set_Domain(&association, &ref_domain);


            KReturnInstance(cr, association);
        }

        sss_sifp_free_attrs(sifp_ctx, &attrs);
    }

    ret = CMPI_RC_OK;

done:
    sss_sifp_free_string_array(sifp_ctx, &paths);
    sss_sifp_free_attrs(sifp_ctx, &attrs);
    sss_sifp_free(&sifp_ctx);
    CMReturn(ret);
}

static CMPIStatus LMI_SSSDBackendDomainGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SSSDBackendDomainCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendDomainModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendDomainDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendDomainExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDBackendDomainAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDBackendDomainAssociators(
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
        LMI_SSSDBackendDomain_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_SSSDBackendDomainAssociatorNames(
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
        LMI_SSSDBackendDomain_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_SSSDBackendDomainReferences(
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
        LMI_SSSDBackendDomain_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_SSSDBackendDomainReferenceNames(
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
        LMI_SSSDBackendDomain_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_SSSDBackendDomain,
    LMI_SSSDBackendDomain,
    _cb,
    LMI_SSSDBackendDomainInitialize(ctx))

CMAssociationMIStub( 
    LMI_SSSDBackendDomain,
    LMI_SSSDBackendDomain,
    _cb,
    LMI_SSSDBackendDomainInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SSSDBackendDomain",
    "LMI_SSSDBackendDomain",
    "instance association")
