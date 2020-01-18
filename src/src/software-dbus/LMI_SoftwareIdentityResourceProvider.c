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
#include "LMI_SoftwareIdentityResource.h"
#include "sw-utils.h"

static const CMPIBroker* _cb = NULL;

static void LMI_SoftwareIdentityResourceInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SoftwareIdentityResource_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_SoftwareIdentityResourceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_SoftwareIdentityResource_ClassName);
}

static CMPIStatus LMI_SoftwareIdentityResourceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SoftwareIdentityResourceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    GPtrArray *array = NULL;
    gboolean repo_enabled;
    gchar *repo_id = NULL, *repo_desc = NULL;
    unsigned i;
    char error_msg[BUFLEN] = "", dsc[BUFLEN] = "",
            instance_id[BUFLEN] = "";

    get_pk_repos(REPO_STATE_ENUM_ANY, &array, error_msg, BUFLEN);
    if (!array) {
        goto done;
    }

    for (i = 0; i < array->len; i++) {
        g_object_get(g_ptr_array_index(array, i), "repo-id", &repo_id,
                "description", &repo_desc, "enabled", &repo_enabled, NULL);
        snprintf(dsc, BUFLEN, "[%s] - %s", repo_id, repo_desc);
        create_instance_id(LMI_SoftwareIdentityResource_ClassName, repo_id,
                instance_id, BUFLEN);

        LMI_SoftwareIdentityResource w;
        LMI_SoftwareIdentityResource_Init(&w, _cb, KNameSpace(cop));
        LMI_SoftwareIdentityResource_Set_SystemName(&w,
                lmi_get_system_name_safe(cc));
        LMI_SoftwareIdentityResource_Set_CreationClassName(&w,
                LMI_SoftwareIdentityResource_ClassName);
        LMI_SoftwareIdentityResource_Set_SystemCreationClassName(&w,
                lmi_get_system_creation_class_name());
        LMI_SoftwareIdentityResource_Set_Name(&w, repo_id);

        LMI_SoftwareIdentityResource_Set_AccessContext(&w,
                LMI_SoftwareIdentityResource_AccessContext_Other);
        LMI_SoftwareIdentityResource_Init_AvailableRequestedStates(&w, 2);
        LMI_SoftwareIdentityResource_Set_AvailableRequestedStates(&w, 0,
                LMI_SoftwareIdentityResource_AvailableRequestedStates_Enabled);
        LMI_SoftwareIdentityResource_Set_AvailableRequestedStates(&w, 1,
                LMI_SoftwareIdentityResource_AvailableRequestedStates_Disabled);
        LMI_SoftwareIdentityResource_Set_EnabledDefault(&w,
                LMI_SoftwareIdentityResource_EnabledDefault_Not_Applicable);
        LMI_SoftwareIdentityResource_Set_ExtendedResourceType(&w,
                LMI_SoftwareIdentityResource_ExtendedResourceType_Linux_RPM);
        LMI_SoftwareIdentityResource_Set_InfoFormat(&w,
                LMI_SoftwareIdentityResource_InfoFormat_URL);
        LMI_SoftwareIdentityResource_Set_OtherAccessContext(&w,
                "YUM package repository");
        LMI_SoftwareIdentityResource_Set_OtherResourceType(&w,
                "RPM Software Package");
        LMI_SoftwareIdentityResource_Set_ResourceType(&w,
                LMI_SoftwareIdentityResource_ResourceType_Other);
        LMI_SoftwareIdentityResource_Set_TransitioningToState(&w,
                LMI_SoftwareIdentityResource_TransitioningToState_Not_Applicable);
        LMI_SoftwareIdentityResource_Init_StatusDescriptions(&w, 0);

        LMI_SoftwareIdentityResource_Set_Caption(&w, repo_desc);
        LMI_SoftwareIdentityResource_Set_Description(&w, dsc);
        LMI_SoftwareIdentityResource_Set_ElementName(&w, repo_id);
        LMI_SoftwareIdentityResource_Set_InstanceID(&w, instance_id);

        if (repo_enabled) {
            LMI_SoftwareIdentityResource_Set_EnabledState(&w,
                    LMI_SoftwareIdentityResource_EnabledState_Enabled);
            LMI_SoftwareIdentityResource_Set_RequestedState(&w,
                    LMI_SoftwareIdentityResource_RequestedState_Enabled);
        } else {
            LMI_SoftwareIdentityResource_Set_EnabledState(&w,
                    LMI_SoftwareIdentityResource_EnabledState_Disabled);
            LMI_SoftwareIdentityResource_Set_RequestedState(&w,
                    LMI_SoftwareIdentityResource_RequestedState_Disabled);
        }

        KReturnInstance(cr, w);

        g_free(repo_id);
        repo_id = NULL;
        g_free(repo_desc);
        repo_desc = NULL;
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

static CMPIStatus LMI_SoftwareIdentityResourceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SoftwareIdentityResourceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityResourceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityResourceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityResourceExecQuery(
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
    LMI_SoftwareIdentityResource,
    LMI_SoftwareIdentityResource,
    _cb,
    LMI_SoftwareIdentityResourceInitialize(ctx))

static CMPIStatus LMI_SoftwareIdentityResourceMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareIdentityResourceInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SoftwareIdentityResource_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SoftwareIdentityResource,
    LMI_SoftwareIdentityResource,
    _cb,
    LMI_SoftwareIdentityResourceInitialize(ctx))

KUint32 LMI_SoftwareIdentityResource_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareIdentityResourceRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    PkTask *task = NULL;
    PkResults *results = NULL;
    GError *gerror = NULL;
    gboolean enable;
    char error_msg[BUFLEN] = "";
    unsigned ret = 1;

    if (RequestedState->null || !RequestedState->exists) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER,
                "Missing requested state");
        goto done;
    }

    if (TimeoutPeriod->exists && !TimeoutPeriod->null) {
        KSetStatus2(_cb, status, ERR_NOT_SUPPORTED,
                "Use of Timeout parameter is not supported");
        goto done;
    }

    if (RequestedState->value == \
            LMI_SoftwareIdentityResource_RequestedState_Enabled) {
        enable = TRUE;
    } else if (RequestedState->value == \
            LMI_SoftwareIdentityResource_RequestedState_Disabled) {
        enable = FALSE;
    } else {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER,
                "Invalid state requested");
        goto done;
    }

    if (!get_pk_repo(self->Name.chars, NULL, error_msg, BUFLEN)) {
        KSetStatus2(_cb, status, ERR_NOT_FOUND, "Repository not found");
        goto done;
    }

    task = pk_task_new();

    results = pk_task_repo_enable_sync(task, self->Name.chars, enable, NULL,
            NULL, NULL, &gerror);
    if (check_and_create_error_msg(results, gerror,
            "Failed to set repository state", error_msg, BUFLEN)) {
        goto done;
    }

    ret = 0;

done:
    g_clear_error(&gerror);

    if (task) {
        g_object_unref(task);
    }
    if (results) {
        g_object_unref(results);
    }

    if (*error_msg) {
        KSetStatus2(_cb, status, ERR_FAILED, error_msg);
    }

    if (ret == 0) {
        KSetStatus(status, OK);
    }

    KUint32_Set(&result, ret);

    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SoftwareIdentityResource",
    "LMI_SoftwareIdentityResource",
    "instance method")
