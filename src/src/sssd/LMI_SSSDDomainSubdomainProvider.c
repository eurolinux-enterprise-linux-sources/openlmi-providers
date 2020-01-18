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
#include <konkret/konkret.h>
#include <sss_sifp_dbus.h>
#include <glib.h>
#include "LMI_SSSDDomainSubdomain.h"
#include "utils.h"

static const CMPIBroker* _cb;

static void LMI_SSSDDomainSubdomainInitialize(const CMPIContext *ctx)
{
    lmi_init(PROVIDER_NAME, _cb, ctx, NULL);
}

static CMPIStatus LMI_SSSDDomainSubdomainCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDDomainSubdomainEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SSSDDomainSubdomainEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    const char *namespace = KNameSpace(cop);
    LMI_SSSDDomainSubdomain association;
    LMI_SSSDDomainRef ref_parent;
    LMI_SSSDDomainRef ref_sub;
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_attr **subdomain = NULL;
    sss_sifp_attr **parent = NULL;
    sss_sifp_error error;
    char **paths = NULL;
    const char *parent_path = NULL;
    const char *parent_name = NULL;
    const char *sub_name = NULL;
    bool is_subdomain;
    GHashTable *table = NULL;
    char *key = NULL;
    char *value = NULL;
    CMPIrc ret;
    int i;

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    error = sss_sifp_invoke_list(sifp_ctx, SSSD_DBUS_LIST_DOMAINS, &paths,
                                 DBUS_TYPE_INVALID);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    table = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    if (table == NULL) {
        ret = CMPI_RC_ERR_FAILED;
        goto done;
    }

    for (i = 0; paths[i] != NULL; i++) {
        /* fetch information about potential subdomain */
        error = sss_sifp_fetch_all_attrs(sifp_ctx, paths[i],
                                         SSS_SIFP_IFACE_DOMAINS, &subdomain);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        error = sss_sifp_find_attr_as_bool(subdomain, "subdomain",
                                           &is_subdomain);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        /* if it is not subdomain just continue with the next one */
        if (!is_subdomain) {
            sss_sifp_free_attrs(sifp_ctx, &subdomain);
            continue;
        }

        /* get subdomain name and parent */
        error = sss_sifp_find_attr_as_string(subdomain, "name",
                                             &sub_name);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        error = sss_sifp_find_attr_as_string(subdomain, "parent_domain",
                                             &parent_path);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        /* first try to lookup the parent in the hash table */
        key = strdup(parent_path);
        if (key == NULL) {
            ret = CMPI_RC_ERR_FAILED;
            goto done;
        }

        value = (char*)g_hash_table_lookup(table, key);
        if (value != NULL) {
            parent_name = (const char*)value;
            free(key);

            /* forget the pointers */
            key = NULL;
            value = NULL;
        } else {
            /* fetch the parent and store it in the hash table */
            error = sss_sifp_fetch_attr(sifp_ctx, parent_path,
                                        SSS_SIFP_IFACE_DOMAINS, "name",
                                        &parent);
            check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

            error = sss_sifp_find_attr_as_string(parent, "name",
                                                 &parent_name);
            check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

            value = strdup(parent_name);
            if (value == NULL) {
                ret = CMPI_RC_ERR_FAILED;
                goto done;
            }

            g_hash_table_insert(table, key, value);

            /* the entry is now owned by the hash table */
            key = NULL;
            value= NULL;
        }

        /* create association */
        LMI_SSSDDomainRef_Init(&ref_parent, _cb, namespace);
        LMI_SSSDDomainRef_Set_Name(&ref_parent, parent_name);

        LMI_SSSDDomainRef_Init(&ref_sub, _cb, namespace);
        LMI_SSSDDomainRef_Set_Name(&ref_sub, sub_name);

        LMI_SSSDDomainSubdomain_Init(&association, _cb, namespace);
        LMI_SSSDDomainSubdomain_Set_ParentDomain(&association, &ref_parent);
        LMI_SSSDDomainSubdomain_Set_Subdomain(&association, &ref_sub);

        KReturnInstance(cr, association);

        sss_sifp_free_attrs(sifp_ctx, &subdomain);
        sss_sifp_free_attrs(sifp_ctx, &parent);
    }

    ret = CMPI_RC_OK;

done:
    if (key != NULL) {
        free(key);
    }

    if (value != NULL) {
        free(value);
    }

    if (table != NULL) {
        g_hash_table_destroy(table);
    }

    if (subdomain != NULL) {
        sss_sifp_free_attrs(sifp_ctx, &subdomain);
    }

    if (parent != NULL) {
        sss_sifp_free_attrs(sifp_ctx, &parent);
    }

    sss_sifp_free_string_array(sifp_ctx, &paths);
    sss_sifp_free(&sifp_ctx);
    CMReturn(ret);
}

static CMPIStatus LMI_SSSDDomainSubdomainGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SSSDDomainSubdomainCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDDomainSubdomainModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDDomainSubdomainDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDDomainSubdomainExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDDomainSubdomainAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDDomainSubdomainAssociators(
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
        LMI_SSSDDomainSubdomain_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_SSSDDomainSubdomainAssociatorNames(
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
        LMI_SSSDDomainSubdomain_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_SSSDDomainSubdomainReferences(
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
        LMI_SSSDDomainSubdomain_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_SSSDDomainSubdomainReferenceNames(
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
        LMI_SSSDDomainSubdomain_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_SSSDDomainSubdomain,
    LMI_SSSDDomainSubdomain,
    _cb,
    LMI_SSSDDomainSubdomainInitialize(ctx))

CMAssociationMIStub( 
    LMI_SSSDDomainSubdomain,
    LMI_SSSDDomainSubdomain,
    _cb,
    LMI_SSSDDomainSubdomainInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SSSDDomainSubdomain",
    "LMI_SSSDDomainSubdomain",
    "instance association")
