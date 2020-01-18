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
#include <dbus/dbus.h>
#include "sssd_domains.h"
#include "utils.h"

struct sssd_domain_attrs {
    const char *name;
    const char *provider;
    const char * const *primary_servers;
    unsigned int num_primary;
    const char * const *backup_servers;
    unsigned int num_backup;
    uint32_t min_id;
    uint32_t max_id;
    const char *realm;
    const char *forest;
    const char *login_format;
    const char *fqn_format;
    bool enumerable;
    bool use_fqn;
    bool subdomain;
    const char *parent_domain;
};

static sssd_method_error
sssd_domain_get_attrs(sss_sifp_attr **attrs,
                      const char *path,
                      struct sssd_domain_attrs *out)
{
    sss_sifp_error error;
    sssd_method_error ret;

    error = sss_sifp_find_attr_as_string(attrs, "name", &out->name);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_string(attrs, "provider", &out->provider);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_uint32(attrs, "min_id", &out->min_id);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_uint32(attrs, "max_id", &out->max_id);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_string(attrs, "realm", &out->realm);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_string(attrs, "forest", &out->forest);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_string(attrs, "login_format",
                                         &out->login_format);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_string(attrs, "fully_qualified_name_format",
                                         &out->fqn_format);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_bool(attrs, "enumerable", &out->enumerable);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_bool(attrs, "use_fully_qualified_names",
                                       &out->use_fqn);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_bool(attrs, "subdomain", &out->subdomain);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_string(attrs, "parent_domain",
                                        &out->parent_domain);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    /* server list may be empty */
    error = sss_sifp_find_attr_as_string_array(attrs, "primary_servers",
                               &out->num_primary, &out->primary_servers);
    if (error == SSS_SIFP_ATTR_NULL) {
        out->num_primary = 0;
        out->primary_servers = NULL;
    } else if (error != SSS_SIFP_OK) {
        ret = SSSD_METHOD_ERROR_FAILED;
        goto done;
    }

    error = sss_sifp_find_attr_as_string_array(attrs, "backup_servers",
                                &out->num_backup, &out->backup_servers);
    if (error == SSS_SIFP_ATTR_NULL) {
            out->num_backup = 0;
            out->backup_servers = NULL;
    } else if (error != SSS_SIFP_OK) {
        ret = SSSD_METHOD_ERROR_FAILED;
        goto done;
    }

    ret = SSSD_METHOD_ERROR_OK;

done:
    return ret;
}

sssd_method_error
sssd_domain_set_instance(sss_sifp_ctx *sifp_ctx,
                         const char *path,
                         const CMPIBroker* cb,
                         const char *namespace,
                         LMI_SSSDDomain *instance)
{
    sss_sifp_attr **attrs = NULL;
    sss_sifp_attr **parent = NULL;
    sss_sifp_error error;
    struct sssd_domain_attrs values;
    sssd_method_error ret;
    int i;

    error = sss_sifp_fetch_all_attrs(sifp_ctx, path,
                                     SSS_SIFP_IFACE_DOMAINS, &attrs);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_IO, done);

    ret = sssd_domain_get_attrs(attrs, path, &values);
    if (ret != SSSD_METHOD_ERROR_OK) {
        goto done;
    }

    LMI_SSSDDomain_Init(instance, cb, namespace);

    /* CIM_ManagedElement */
    LMI_SSSDDomain_Set_Caption(instance, "SSSD Domain");
    LMI_SSSDDomain_Set_Description(instance, "SSSD Domain");
    LMI_SSSDDomain_Set_ElementName(instance, values.name);

    /* LMI_SSSDDomain */
    LMI_SSSDDomain_Set_Name(instance, values.name);
    LMI_SSSDDomain_Set_Provider(instance, values.provider);
    LMI_SSSDDomain_Set_MinId(instance, values.min_id);
    LMI_SSSDDomain_Set_MaxId(instance, values.max_id);
    LMI_SSSDDomain_Set_Realm(instance, values.realm);
    LMI_SSSDDomain_Set_Forest(instance, values.forest);
    LMI_SSSDDomain_Set_LoginFormat(instance, values.login_format);
    LMI_SSSDDomain_Set_FullyQualifiedNameFormat(instance, values.fqn_format);
    LMI_SSSDDomain_Set_Enumerate(instance, values.enumerable);
    LMI_SSSDDomain_Set_UseFullyQualifiedNames(instance, values.use_fqn);
    LMI_SSSDDomain_Set_IsSubdomain(instance, values.subdomain);

    LMI_SSSDDomain_Init_PrimaryServers(instance, values.num_primary);
    for (i = 0; i < values.num_primary; i++) {
        LMI_SSSDDomain_Set_PrimaryServers(instance, i,
                                          values.primary_servers[i]);
    }

    LMI_SSSDDomain_Init_BackupServers(instance, values.num_backup);
    for (i = 0; i < values.num_backup; i++) {
        LMI_SSSDDomain_Set_BackupServers(instance, i,
                                         values.backup_servers[i]);
    }

    if (strcmp(values.parent_domain, "/") == 0) {
        LMI_SSSDDomain_Null_ParentDomain(instance);
    } else {
        error = sss_sifp_fetch_attr(sifp_ctx, values.parent_domain,
                                    SSS_SIFP_IFACE_DOMAINS, "name", &parent);
        check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_IO, done);

        error = sss_sifp_find_attr_as_string(parent, "name",
                                             &values.parent_domain);
        check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

        LMI_SSSDDomain_Set_ParentDomain(instance, values.parent_domain);
    }

    ret = SSSD_METHOD_ERROR_OK;

done:
    sss_sifp_free_attrs(sifp_ctx, &attrs);
    sss_sifp_free_attrs(sifp_ctx, &parent);
    return ret;
}
