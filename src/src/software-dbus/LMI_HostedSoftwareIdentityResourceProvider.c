/*
 * Copyright (C) 2013-2014 Red Hat, Inc. All rights reserved.
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
 * Authors: Peter Schiffer <pschiffe@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_HostedSoftwareIdentityResource.h"
#include "sw-utils.h"

static const CMPIBroker* _cb;

static void LMI_HostedSoftwareIdentityResourceInitialize(const CMPIContext *ctx)
{
    software_init(LMI_HostedSoftwareIdentityResource_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_HostedSoftwareIdentityResource_ClassName);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    GPtrArray *array = NULL;
    gchar *repo_id = NULL;
    unsigned i;
    char error_msg[BUFLEN] = "";

    get_pk_repos(REPO_STATE_ENUM_ANY, &array, error_msg, BUFLEN);
    if (!array) {
        goto done;
    }

    for (i = 0; i < array->len; i++) {
        g_object_get(g_ptr_array_index(array, i), "repo-id", &repo_id, NULL);

        LMI_SoftwareIdentityResourceRef sir;
        LMI_SoftwareIdentityResourceRef_Init(&sir, _cb, KNameSpace(cop));
        LMI_SoftwareIdentityResourceRef_Set_SystemName(&sir, lmi_get_system_name_safe(cc));
        LMI_SoftwareIdentityResourceRef_Set_CreationClassName(&sir,
                LMI_SoftwareIdentityResource_ClassName);
        LMI_SoftwareIdentityResourceRef_Set_SystemCreationClassName(&sir,
                lmi_get_system_creation_class_name());
        LMI_SoftwareIdentityResourceRef_Set_Name(&sir, repo_id);

        g_free(repo_id);
        repo_id = NULL;

        LMI_HostedSoftwareIdentityResource w;
        LMI_HostedSoftwareIdentityResource_Init(&w, _cb, KNameSpace(cop));
        LMI_HostedSoftwareIdentityResource_SetObjectPath_Antecedent(&w,
                lmi_get_computer_system_safe(cc));
        LMI_HostedSoftwareIdentityResource_Set_Dependent(&w, &sir);

        KReturnInstance(cr, w);
    }

done:
    if (array) {
        g_ptr_array_unref(array);
    }

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceAssociators(
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
        LMI_HostedSoftwareIdentityResource_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceAssociatorNames(
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
        LMI_HostedSoftwareIdentityResource_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceReferences(
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
        LMI_HostedSoftwareIdentityResource_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_HostedSoftwareIdentityResourceReferenceNames(
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
        LMI_HostedSoftwareIdentityResource_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_HostedSoftwareIdentityResource,
    LMI_HostedSoftwareIdentityResource,
    _cb,
    LMI_HostedSoftwareIdentityResourceInitialize(ctx))

CMAssociationMIStub(
    LMI_HostedSoftwareIdentityResource,
    LMI_HostedSoftwareIdentityResource,
    _cb,
    LMI_HostedSoftwareIdentityResourceInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_HostedSoftwareIdentityResource",
    "LMI_HostedSoftwareIdentityResource",
    "instance association")
