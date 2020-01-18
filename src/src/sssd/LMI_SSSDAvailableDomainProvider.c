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
#include "LMI_SSSDAvailableDomain.h"
#include "utils.h"

static const CMPIBroker* _cb;

static void LMI_SSSDAvailableDomainInitialize(const CMPIContext *ctx)
{
    lmi_init(PROVIDER_NAME, _cb, ctx, NULL);
}

static CMPIStatus LMI_SSSDAvailableDomainCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDAvailableDomainEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SSSDAvailableDomainEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    const char *namespace = KNameSpace(cop);
    LMI_SSSDAvailableDomain association;
    LMI_SSSDServiceRef ref_sssd;
    LMI_SSSDDomainRef ref_domain;
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_attr **attrs = NULL;
    sss_sifp_error error;
    char **paths = NULL;
    const char *name = NULL;
    CMPIrc ret;
    int i;

    LMI_SSSDService_Get_Ref(cc, _cb, namespace, &ref_sssd);

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    error = sss_sifp_invoke_list(sifp_ctx, SSSD_DBUS_LIST_DOMAINS, &paths,
                                 DBUS_TYPE_INVALID);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    for (i = 0; paths[i] != NULL; i++) {
        error = sss_sifp_fetch_attr(sifp_ctx, paths[i], SSS_SIFP_IFACE_DOMAINS,
                                    "name", &attrs);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        error = sss_sifp_find_attr_as_string(attrs, "name", &name);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        LMI_SSSDDomainRef_Init(&ref_domain, _cb, namespace);
        LMI_SSSDDomainRef_Set_Name(&ref_domain, name);

        LMI_SSSDAvailableDomain_Init(&association, _cb, namespace);
        LMI_SSSDAvailableDomain_Set_SSSD(&association, &ref_sssd);
        LMI_SSSDAvailableDomain_Set_Domain(&association, &ref_domain);

        KReturnInstance(cr, association);

        sss_sifp_free_attrs(sifp_ctx, &attrs);
    }

    ret = CMPI_RC_OK;

done:
    sss_sifp_free_string_array(sifp_ctx, &paths);
    sss_sifp_free_attrs(sifp_ctx, &attrs);
    sss_sifp_free(&sifp_ctx);
    CMReturn(ret);
}

static CMPIStatus LMI_SSSDAvailableDomainGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SSSDAvailableDomainCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDAvailableDomainModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDAvailableDomainDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDAvailableDomainExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDAvailableDomainAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDAvailableDomainAssociators(
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
        LMI_SSSDAvailableDomain_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_SSSDAvailableDomainAssociatorNames(
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
        LMI_SSSDAvailableDomain_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_SSSDAvailableDomainReferences(
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
        LMI_SSSDAvailableDomain_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_SSSDAvailableDomainReferenceNames(
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
        LMI_SSSDAvailableDomain_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_SSSDAvailableDomain,
    LMI_SSSDAvailableDomain,
    _cb,
    LMI_SSSDAvailableDomainInitialize(ctx))

CMAssociationMIStub( 
    LMI_SSSDAvailableDomain,
    LMI_SSSDAvailableDomain,
    _cb,
    LMI_SSSDAvailableDomainInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SSSDAvailableDomain",
    "LMI_SSSDAvailableDomain",
    "instance association")
