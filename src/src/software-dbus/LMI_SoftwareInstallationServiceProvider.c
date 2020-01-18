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
 * Authors: Michal Minar <miminar@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_SoftwareIdentity.h"
#include "LMI_SoftwareInstallationService.h"
#include "job_manager.h"
#include "sw-utils.h"
#include "lmi_sw_job.h"

#define FIND_IDENTITY_FOUND 0
#define FIND_IDENTITY_NO_MATCH 1

static const CMPIBroker* _cb = NULL;

static gboolean is_power_of_two(guint32 x) {
     return ((x != 0) && !(x & (x - 1)));
}

static void LMI_SoftwareInstallationServiceInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SoftwareInstallationService_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_SoftwareInstallationServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_SoftwareInstallationService_ClassName);
}

static CMPIStatus LMI_SoftwareInstallationServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
            _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SoftwareInstallationServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    char instance_id[BUFLEN] = "";

    create_instance_id(LMI_SoftwareInstallationService_ClassName, NULL, instance_id,
                    BUFLEN);

    LMI_SoftwareInstallationService w;
    LMI_SoftwareInstallationService_Init(&w, _cb, KNameSpace(cop));
    LMI_SoftwareInstallationService_Set_CreationClassName(&w,
            LMI_SoftwareInstallationService_ClassName);
    LMI_SoftwareInstallationService_Set_SystemCreationClassName(&w,
            lmi_get_system_creation_class_name());
    LMI_SoftwareInstallationService_Set_SystemName(&w,
            lmi_get_system_name_safe(cc));
    LMI_SoftwareInstallationService_Set_Name(&w, instance_id);
    LMI_SoftwareInstallationService_Set_Caption(&w,
            "Software installation service for this system.");
    LMI_SoftwareInstallationService_Set_CommunicationStatus_Not_Available(&w);
    LMI_SoftwareInstallationService_Set_Description(&w,
            "Software installation service using YUM package manager.");
    LMI_SoftwareInstallationService_Set_DetailedStatus_Not_Available(&w);
    LMI_SoftwareInstallationService_Set_EnabledDefault_Not_Applicable(&w);
    LMI_SoftwareInstallationService_Set_EnabledState_Not_Applicable(&w);
    LMI_SoftwareInstallationService_Set_HealthState_OK(&w);
    LMI_SoftwareInstallationService_Set_InstanceID(&w, instance_id);
    if (LMI_SoftwareInstallationService_Init_OperationalStatus(&w, 1))
        LMI_SoftwareInstallationService_Set_OperationalStatus_OK(&w, 0);
    LMI_SoftwareInstallationService_Set_OperatingStatus_Servicing(&w);
    LMI_SoftwareInstallationService_Set_PrimaryStatus_OK(&w);
    LMI_SoftwareInstallationService_Set_RequestedState_Not_Applicable(&w);
    LMI_SoftwareInstallationService_Set_Started(&w, TRUE);
    LMI_SoftwareInstallationService_Set_TransitioningToState_Not_Applicable(&w);
    KReturnInstance(cr, w);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareInstallationServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SoftwareInstallationServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareInstallationServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareInstallationServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareInstallationServiceExecQuery(
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
    LMI_SoftwareInstallationService,
    LMI_SoftwareInstallationService,
    _cb,
    LMI_SoftwareInstallationServiceInitialize(ctx))

static CMPIStatus LMI_SoftwareInstallationServiceMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareInstallationServiceInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SoftwareInstallationService_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SoftwareInstallationService,
    LMI_SoftwareInstallationService,
    _cb,
    LMI_SoftwareInstallationServiceInitialize(ctx))

KUint32 LMI_SoftwareInstallationService_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_SoftwareInstallationService_StartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_SoftwareInstallationService_StopService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_SoftwareInstallationService_ChangeAffectedElementsAssignedSequence(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceRef* self,
    const KRefA* ManagedElements,
    const KUint16A* AssignedSequence,
    KRef* Job,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_SoftwareInstallationService_CheckSoftwareIdentity(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceRef* self,
    const KRef* Source,
    const KRef* Target,
    const KRef* Collection,
    KUint16A* InstallCharacteristics,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KUint32_Set(&result, INSTALL_METHOD_RESULT_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_SoftwareInstallationService_InstallFromSoftwareIdentity(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceRef* self,
    KRef* Job,
    const KUint16A* InstallOptions,
    const KStringA* InstallOptionsValues,
    const KRef* Source,
    const KRef* Target,
    const KRef* Collection,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint16 opt;
    guint32 op = 0;
    LmiJob *job = NULL;
    GVariant *variant;
    gchar *jobid;
    CMPIObjectPath *cop;
    LMI_SoftwareIdentityRef pkgop;

    if (!KHasValue(Source)) {
        lmi_error("Missing Source argument!");
        KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
        return result;
    }

    if (KHasValue(InstallOptions)) {
        for (size_t i=0; i < InstallOptions->count; ++i) {
            opt = KUint16A_Get(InstallOptions, i);
            switch (opt.value) {
                case INSTALL_OPTION_INSTALL:
                    op |= INSTALLATION_OPERATION_INSTALL;
                    break;
                case INSTALL_OPTION_UPDATE:
                    op |= INSTALLATION_OPERATION_UPDATE;
                    break;
                case INSTALL_OPTION_UNINSTALL:
                    op |= INSTALLATION_OPERATION_REMOVE;
                    break;
                case INSTALL_OPTION_FORCE_INSTALLATION:
                    op |= INSTALLATION_OPERATION_FORCE;
                    break;
                case INSTALL_OPTION_REPAIR:
                    op |= INSTALLATION_OPERATION_REPAIR;
                    break;
                default:
                    lmi_error("Given unsupported install option %u!", opt);
                    KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
                    return result;
            }
        }
        if (!is_power_of_two(op & INSTALLATION_OPERATION_EXCLUSIVE_GROUP)) {
            lmi_error("Conflicting install options given!"
                   " Choose one of Install, Remove, Update.");
            KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
            return result;
        }
    }
    if (!(op & INSTALLATION_OPERATION_EXCLUSIVE_GROUP)) {
        op |= INSTALLATION_OPERATION_INSTALL;
    }

    if (KHasValue(InstallOptionsValues) && InstallOptionsValues->count) {
        if (InstallOptionsValues->count > InstallOptions->count) {
            lmi_error("InstallOptionsValues can not have more"
                   " values than InstallOptions!");
            KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
            return result;
        }
        for (size_t i=0; i < InstallOptionsValues->count; ++i) {
            if (KHasValue(KStringA_Get(InstallOptionsValues, i))) {
                lmi_error("None of supported install option can have associated"
                        " install option value!");
                KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
                return result;
            }
        }
    }

    if (KHasValue(Target)) {
        if (!lmi_check_computer_system(context, Target->value)) {
            lmi_error("Target does not match this system!");
            KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
            return result;
        }
    }
    if (KHasValue(Collection)) {
        if (!check_system_software_collection(cb, Collection->value)) {
            lmi_error("Invalid system software collection!");
            KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
            return result;
        }
    }
    if (KHasValue(Target) && KHasValue(Collection)) {
        lmi_error("Only one of Target and Collection parameters cen be given"
                " at the same time!");
        KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
        return result;
    }

    if (!KHasValue(Target) && !KHasValue(Collection)) {
        lmi_error("Either Target or Collection parameter must be specified!");
        KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
        return result;
    }

    *status = LMI_SoftwareIdentityRef_InitFromObjectPath(
            &pkgop, cb, Source->value);
    if (status->rc) {
        lmi_error("Invalid Source parameter!");
        KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
        return result;
    }
    if (g_ascii_strncasecmp(pkgop.InstanceID.chars,
                SW_IDENTITY_INSTANCE_ID_PREFIX,
                SW_IDENTITY_INSTANCE_ID_PREFIX_LEN) != 0)
    {
        lmi_error("Invalid InstanceID of Source!");
        KUint32_Set(&result, INSTALL_METHOD_RESULT_INVALID_PARAMETER);
        return result;
    }

    if ((job = jobmgr_new_job(LMI_TYPE_SW_INSTALLATION_JOB)) == NULL) {
        lmi_error("Memory allocation failed!");
        KUint32_Set(&result, INSTALL_METHOD_RESULT_NOT_ENOUGH_MEMORY);
        return result;
    }

    lmi_job_set_method_name(job, "InstallFromSoftwareIdentity");

    variant = g_variant_new_boolean(KHasValue(Target));
    lmi_job_set_in_param(job, IN_PARAM_TARGET_NAME, variant);
    variant = g_variant_new_boolean(KHasValue(Collection));
    lmi_job_set_in_param(job, IN_PARAM_COLLECTION_NAME, variant);
    variant = g_variant_new_string(pkgop.InstanceID.chars
            + SW_IDENTITY_INSTANCE_ID_PREFIX_LEN);
    lmi_job_set_in_param(job, IN_PARAM_SOURCE_NAME, variant);
    variant = g_variant_new_uint32(op);
    lmi_job_set_in_param(job, IN_PARAM_INSTALL_OPTIONS_NAME, variant);

    if ((*status = jobmgr_run_job(job)).rc) {
        lmi_error("Failed to run job on SoftwareIdentity \"%s\"!",
                pkgop.InstanceID.chars);
        KUint32_Set(&result, INSTALL_METHOD_RESULT_UNSPECIFIED_ERROR);
        return result;
    }

    if ((*status = jobmgr_job_to_cim_op(job, &cop)).rc)
        return result;

    KRef_SetObjectPath(Job, cop);
    jobid = lmi_job_get_jobid(job);
    lmi_info("Installation job \"%s\" started on SoftwareIdentity \"%s\".",
            jobid, pkgop.InstanceID.chars);
    KUint32_Set(&result, INSTALL_METHOD_RESULT_JOB_STARTED);
    g_free(jobid);

    return result;
}

KUint32 LMI_SoftwareInstallationService_InstallFromByteStream(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceRef* self,
    KRef* Job,
    const KUint8A* Image,
    const KRef* Target,
    const KUint16A* InstallOptions,
    const KStringA* InstallOptionsValues,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KUint32_Set(&result, INSTALL_METHOD_RESULT_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_SoftwareInstallationService_InstallFromURI(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceRef* self,
    KRef* Job,
    const KString* URI,
    const KRef* Target,
    const KUint16A* InstallOptions,
    const KStringA* InstallOptionsValues,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_SoftwareInstallationService_VerifyInstalledIdentity(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceRef* self,
    const KRef* Source,
    const KRef* Target,
    KRef* Job,
    KRefA* Failed,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

static gboolean traverse_add_matching_elem_name(gpointer key,
                                                gpointer value,
                                                gpointer data)
{
    PkPackage *pk_pkg = PK_PACKAGE(key);
    GPtrArray *matched_elem_names = data;
    SwPackage sw_pkg;
    gchar elem_name[BUFLEN];
    gchar *tmp;

    init_sw_package(&sw_pkg);
    if (create_sw_package_from_pk_pkg(pk_pkg, &sw_pkg))
        return TRUE;
    sw_pkg_get_element_name(&sw_pkg, elem_name, BUFLEN);
    tmp = g_strdup(elem_name);
    if (tmp == NULL) {
        lmi_error("Memory allocation failed");
        return TRUE;
    }
    g_ptr_array_add(matched_elem_names, elem_name);
    return FALSE;
}

KUint32 LMI_SoftwareInstallationService_FindIdentity(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceRef* self,
    const KString* Name,
    const KUint32* Epoch,
    const KString* Version,
    const KString* Release,
    const KString* Architecture,
    const KRef* Repository,
    const KBoolean* AllowDuplicates,
    const KBoolean* ExactMatch,
    KRefA* Matches,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    GError *gerror = NULL;
    GPtrArray *array = NULL;
    gchar buf[BUFLEN];
    gchar err_msg[BUFLEN];
    SwPackage sw_package;
    gchar *repo_id = NULL;
    CMPIObjectPath *op = NULL;
    gchar *tmp;
    const gchar *match_repo_id = NULL;
    GPtrArray *matched_elem_names = NULL;
    GPtrArray *installed_pk_pkgs = NULL;
    PkResults *results = NULL;
    PkPackage *pk_pkg;
    GTree *pkg_repo_map = NULL;
    GPtrArray *pk_repo_dets = NULL;
    gchar *namespace = NULL;
    CMPIValue value;
    gboolean exact_match = KHasValue(ExactMatch) && ExactMatch->value;
    PkBitfield filters = (!KHasValue(AllowDuplicates) || !AllowDuplicates->value)
                         ? pk_bitfield_value(PK_FILTER_ENUM_NEWEST) : 0L;

    if (KHasValue(Architecture) &&
            g_strcmp0(Architecture->chars, get_distro_arch()) != 0 &&
            g_strcmp0(Architecture->chars, "noarch") != 0)
        pk_bitfield_add(filters, PK_FILTER_ENUM_NOT_ARCH);

    if (KHasValue(Repository)) {
        *status = check_software_identity_resource(
                cb, context, Repository->value, NULL);
        if (status->rc)
            goto err;
        match_repo_id = lmi_get_string_property_from_objectpath(
                Repository->value, "Name");
    }

    if (KHasValue(Name) && strlen(Name->chars) > 0) {
        /* get from packagekit just those packages containing given name */
        PkTask *task = pk_task_new();
        /* values shall not be changed */
        gchar *values[2] = { (gchar *) Name->chars, NULL };

        results = pk_task_search_names_sync(task, filters, values,
                NULL, NULL, NULL, &gerror);
        if (check_and_create_error_msg(results, gerror, "Resolving package failed",
                err_msg, BUFLEN))
        {
            g_object_unref(task);
            lmi_warn(err_msg);
            goto err;
        }

        array = pk_results_get_package_array(results);
        g_object_unref(task);
    } else {
        /* list all packages and filter them later */
        get_pk_packages(filters, &array, err_msg, BUFLEN);
        if (!array)
            goto err;
    }

    matched_elem_names = g_ptr_array_new_with_free_func(g_free);
    if (matched_elem_names == NULL)
        goto mem_err;
    if (match_repo_id) {
        installed_pk_pkgs = g_ptr_array_new_with_free_func(g_object_unref);
        if (installed_pk_pkgs == NULL)
            goto mem_err;
    }

    /* filter obtained packages */
    for (guint i = 0; i < array->len; i++) {
        pk_pkg = g_ptr_array_index(array, i);
        init_sw_package(&sw_package);
        if (create_sw_package_from_pk_pkg(pk_pkg, &sw_package))
            goto next_package;
        if ((exact_match && strcmp(sw_package.name, Name->chars) != 0) ||
            (KHasValue(Version) &&
              g_strcmp0(sw_package.version, Version->chars) != 0) ||
            (KHasValue(Release) &&
              g_strcmp0(sw_package.release, Release->chars) != 0) ||
            (KHasValue(Architecture) && g_strcmp0(sw_package.arch,
                  Architecture->chars) != 0) ||
            (KHasValue(Epoch) &&
             (guint64) Epoch->value != g_ascii_strtoull(
                 sw_package.epoch, NULL, 10)))
        {
            goto next_package;
        }

        if (match_repo_id != NULL) {
            /* Filter by repository id. */
            if (pk_pkg_is_installed(pk_pkg) && !pk_pkg_is_available(pk_pkg)) {
                /* Installed packages need to be processed later because they
                 * don't always carry the information of their source
                 * repository. */
                g_ptr_array_add(installed_pk_pkgs, g_object_ref(pk_pkg));
                goto next_package;
            } else if (!pk_pkg_belongs_to_repo(pk_pkg, match_repo_id)) {
                goto next_package;
            }
        }

        /* Package meet all the requirements. */
        sw_pkg_get_element_name(&sw_package, buf, BUFLEN);
        tmp = g_strdup(buf);
        if (tmp == NULL) {
            clean_sw_package(&sw_package);
            goto mem_err;
        }
        g_ptr_array_add(matched_elem_names, tmp);

next_package:
        clean_sw_package(&sw_package);
        g_clear_pointer(&repo_id, g_free);
    }
    g_clear_object(&results);

    /* Process installed packages. Generate one result for each
     * (matched_package X repository where it's available). */
    if (installed_pk_pkgs && installed_pk_pkgs->len > 0) {
        PkRepoDetail *match_repo_det = NULL;
        pk_repo_dets = g_ptr_array_new_full(1, g_object_unref);
        if (pk_repo_dets == NULL)
            goto mem_err;
        get_pk_repo(match_repo_id, &match_repo_det, err_msg, BUFLEN);
        if (match_repo_det == NULL)
            goto err;
        g_ptr_array_add(pk_repo_dets, match_repo_det);
        get_repo_dets_for_pk_pkgs(installed_pk_pkgs, pk_repo_dets,
                &pkg_repo_map, err_msg, BUFLEN);
        if (pkg_repo_map == NULL)
            goto err;
        g_tree_foreach(pkg_repo_map, traverse_add_matching_elem_name,
                matched_elem_names);
        g_clear_pointer(&pkg_repo_map, g_tree_unref);
        g_clear_pointer(&pk_repo_dets, g_ptr_array_unref);
    }

    /* create and fill the Matches output parameters */
    KRefA_Init(Matches, cb, matched_elem_names->len);
    if (matched_elem_names->len > 0) {
        namespace = lmi_read_config("CIM", "Namespace");
        if (namespace == NULL)
            goto mem_err;
        if (matched_elem_names->len > 1) {
            lmi_debug("Found %d matching packages.", matched_elem_names->len);
        } else {
            lmi_debug("Found 1 matching package: %s",
                    g_ptr_array_index(matched_elem_names, 0));
        }
        KUint32_Set(&result, FIND_IDENTITY_FOUND);
    } else {
        lmi_warn("No matching package found.");
        KUint32_Set(&result, FIND_IDENTITY_NO_MATCH);
    }
    for (guint i=0; i < matched_elem_names->len; ++i) {
        op = CMNewObjectPath(cb, namespace,
                LMI_SoftwareIdentity_ClassName, status);
        if (status->rc)
            goto err;
        create_instance_id(LMI_SoftwareIdentity_ClassName,
                g_ptr_array_index(matched_elem_names, i), buf, BUFLEN);
        value.string = CMNewString(cb, buf, status);
        if (status->rc)
            goto err;
        *status = CMAddKey(op, "InstanceID", &value, CMPI_string);
        if (status->rc) {
            CMRelease(value.string);
            goto mem_err;
        }
        KRefA_Set(Matches, i, op);
        op = NULL;
    }

    g_ptr_array_unref(matched_elem_names);
    g_free(namespace);
    return result;

mem_err:
    lmi_error("Memory allocation failed");
err:
    g_clear_pointer(&matched_elem_names, g_ptr_array_unref);
    g_clear_pointer(&pkg_repo_map, g_tree_unref);
    g_clear_pointer(&pk_repo_dets, g_ptr_array_unref);
    g_clear_object(&results);
    if (op != NULL)
        CMRelease(op);
    g_free(namespace);
    if (!status->rc)
        KSetStatus(status, ERR_FAILED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SoftwareInstallationService",
    "LMI_SoftwareInstallationService",
    "instance method")
