/*
 * Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
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

#include <stdio.h>
#include <strings.h>
#include "openlmi.h"
#include "sw-utils.h"
#include "LMI_SystemSoftwareCollection.h"
#include "LMI_SoftwareInstallationJob.h"
#include "lmi_sw_job.h"

G_DEFINE_TYPE(LmiSwInstallationJob, lmi_sw_installation_job, LMI_TYPE_JOB)

static void lmi_sw_installation_job_class_init(LmiSwInstallationJobClass *klass)
{
    G_OBJECT_CLASS(klass);
}

static void lmi_sw_installation_job_init(LmiSwInstallationJob *self) {}

G_DEFINE_TYPE(LmiSwVerificationJob, lmi_sw_verification_job, LMI_TYPE_JOB)

static void lmi_sw_verification_job_class_init(LmiSwVerificationJobClass *klass)
{
    G_OBJECT_CLASS(klass);
}

static void lmi_sw_verification_job_init(LmiSwVerificationJob *self) {}

static gchar *make_job_description(const LmiJob *job)
{
    guint number = lmi_job_get_number(job);
    gchar *jobid = lmi_job_get_jobid(job);
    gchar *template = NULL;
    gchar *dest = NULL;
    gchar *result = NULL;
    GVariant *variant = NULL;
    guint install_options;

    if (!jobid)
        goto memory_err;
    if (LMI_IS_SW_INSTALLATION_JOB(job)) {
        if (lmi_job_has_in_param(job, IN_PARAM_INSTALL_OPTIONS_NAME)) {
            if ((variant = lmi_job_get_in_param(job,
                            IN_PARAM_INSTALL_OPTIONS_NAME)) == NULL)
                goto memory_err;
            install_options = g_variant_get_uint32(variant);
            if (install_options & INSTALLATION_OPERATION_INSTALL) {
                template = "Software package installation job #%u (id=%s) for %s.";
            } else if (install_options & INSTALLATION_OPERATION_UPDATE) {
                template = "Software package update job #%u (id=%s) for %s.";
            } else if (install_options & INSTALLATION_OPERATION_REMOVE) {
                template = "Software package removal job #%u (id=%s) for %s.";
            } else {
                lmi_warn("Failed to describe InstallOptions of job \"%s\": %u!",
                        jobid, install_options);
                template = "Software job #%u (id=%s) for %s.";
            }
        } else {
            template = "Newly created job %u (id=%s) for %s.";
        }
    } else if (LMI_IS_SW_VERIFICATION_JOB(job)) {
        template = "Software verification job %u (id=%s) for %s.";
    } else {
        lmi_error("Can not describe job of type \"%s\".",
                G_OBJECT_TYPE_NAME(job));
        goto err;
    }

    if (lmi_job_has_in_param(job, IN_PARAM_SOURCE_NAME)) {
        if ((variant = lmi_job_get_in_param(job, IN_PARAM_SOURCE_NAME)) == NULL)
            goto memory_err;
        dest = g_strdup_printf("package %s", g_variant_get_string(variant, NULL));
    } else if (lmi_job_has_in_param(job, IN_PARAM_URI_NAME)) {
        if ((variant = lmi_job_get_in_param(job, IN_PARAM_URI_NAME)) == NULL)
            goto memory_err;
        dest = g_strdup_printf("uri %s", g_variant_get_string(variant, NULL));
    } else {
        dest = g_strdup_printf("unknown source");
    }
    if (variant)
        g_variant_unref(variant);
    if (dest == NULL) {
        goto memory_err;
    }

    g_assert(template);
    result = g_strdup_printf(template, number, jobid, dest);
    g_free(dest);
    g_free(jobid);
    jobid = NULL;
    if (!result)
        goto memory_err;
    return result;

memory_err:
    lmi_error("Memory allocation failed");
err:
    g_free(jobid);
    return result;
}

CMPIStatus lmi_sw_job_to_cim_instance(const CMPIBroker *cb,
                                      const CMPIContext *ctx,
                                      const LmiJob *job,
                                      CMPIInstance *instance)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    gchar buf[BUFLEN];
    CMPIValue value;
    gchar *description = NULL;

    JOB_CRITICAL_BEGIN(job);

    g_snprintf(buf, BUFLEN, "Software installation job #%u",
            lmi_job_get_number(job));

    if ((value.string = CMNewString(cb, buf, &status)) == NULL)
        goto err;
    if ((status = CMSetProperty(instance, "Caption",
                    &value, CMPI_string)).rc)
        goto string_err;

    value.uint16 = LMI_SoftwareInstallationJob_CommunicationStatus_Communication_OK;
    if (lmi_job_get_state(job) == LMI_JOB_STATE_ENUM_EXCEPTION &&
            lmi_job_has_out_param(job, "PkError"))
    {
        GVariant *variant = lmi_job_get_out_param(job, "PkError");
        if (variant) {
            switch (g_variant_get_uint32(variant)) {
            case PK_ERROR_ENUM_NO_NETWORK:
            case PK_ERROR_ENUM_PACKAGE_DOWNLOAD_FAILED:
            case PK_ERROR_ENUM_NO_MORE_MIRRORS_TO_TRY:
            case PK_ERROR_ENUM_CANNOT_FETCH_SOURCES:
                value.uint16 = LMI_SoftwareInstallationJob_CommunicationStatus_Lost_Communication;
                break;
            default:
                break;
            }
        }
        g_variant_unref(variant);
    }
    /* This property is already prefilled by jobmanager. Thus no allocation is
     * necessary. */
    CMSetProperty(instance, "CommunicationStatus", &value, CMPI_uint16);

    if ((description = make_job_description(job)) != NULL) {
        if ((value.string = CMNewString(cb, description, &status)) == NULL)
            goto description_err;
        if ((status = CMSetProperty(instance, "Description",
                    &value, CMPI_string)).rc)
            goto string_err;
        g_free(description);
        description = NULL;
    }

    JOB_CRITICAL_END(job);
    return status;

string_err:
    CMRelease(value.string);
description_err:
    if (description)
        g_free(description);
err:
    JOB_CRITICAL_END(job);
    lmi_error("Memory allocation failed!");
    return status;
}

CMPIStatus lmi_sw_job_make_job_parameters(const CMPIBroker *cb,
                                          const CMPIContext *ctx,
                                          const LmiJob *job,
                                          gboolean include_input,
                                          gboolean include_output,
                                          CMPIInstance *instance)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    gchar *namespace = NULL;
    CMPIValue value, elem;
    GVariant *variant;
    CMPIObjectPath *op = NULL;
    guint32 options, tmp, count = 0;
    char instance_id[BUFLEN];

    if (include_input) {
        if ((namespace = lmi_read_config("CIM", "Namespace")) == NULL)
            goto err;
        if (lmi_job_has_in_param(job, IN_PARAM_SOURCE_NAME)) {
            op = CMNewObjectPath(cb, namespace,
                            LMI_SoftwareIdentity_ClassName, &status);
            if (op == NULL)
                goto err;
            variant = lmi_job_get_in_param(job, IN_PARAM_SOURCE_NAME);
            create_instance_id(LMI_SoftwareIdentity_ClassName,
                    g_variant_get_string(variant, NULL), instance_id, BUFLEN);
            value.string = CMNewString(cb, instance_id, &status);
            g_variant_unref(variant);
            if (value.string == NULL)
                goto err;
            status = CMAddKey(op, "InstanceID", &value, CMPI_string);
            if (status.rc)
                goto op_err;
            value.ref = op;
            status = CMSetProperty(instance, IN_PARAM_SOURCE_NAME,
                            &value, CMPI_ref);
            if (status.rc)
                goto op_err;
        }

        if (lmi_job_has_in_param(job, IN_PARAM_URI_NAME)) {
            variant = lmi_job_get_in_param(job, IN_PARAM_URI_NAME);
            value.string = CMNewString(cb,
                            g_variant_get_string(variant, NULL), &status);
            g_variant_unref(variant);
            if (!value.string || status.rc)
                goto err;
            status = CMSetProperty(instance, IN_PARAM_URI_NAME,
                    &value, CMPI_string);
            if (status.rc)
                goto string_err;
        }

        if (lmi_job_has_in_param(job, IN_PARAM_INSTALL_OPTIONS_NAME)) {
            variant = lmi_job_get_in_param(job, IN_PARAM_INSTALL_OPTIONS_NAME);
            options = g_variant_get_uint32(variant);
            g_variant_unref(variant);
            tmp = options;
            while (tmp) {
                count += tmp & 1;
                tmp = tmp >> 1;
            }
            value.array = CMNewArray(cb, count, CMPI_uint16, &status);
            if (value.array == NULL || status.rc)
                goto err;
            if (options & INSTALLATION_OPERATION_INSTALL) {
                elem.uint16 = INSTALL_OPTION_INSTALL;
            } else if (options & INSTALLATION_OPERATION_UPDATE) {
                elem.uint16 = INSTALL_OPTION_UPDATE;
            } else {
                elem.uint16 = INSTALL_OPTION_UNINSTALL;
            }
            tmp = 0;
            CMSetArrayElementAt(value.array, tmp++, &elem, CMPI_uint16);
            if (options & INSTALLATION_OPERATION_FORCE) {
                elem.uint16 = INSTALL_OPTION_FORCE_INSTALLATION;
                CMSetArrayElementAt(value.array, tmp++, &elem, CMPI_uint16);
            }
            if (options & INSTALLATION_OPERATION_REPAIR) {
                elem.uint16 = INSTALL_OPTION_REPAIR;
                CMSetArrayElementAt(value.array, tmp++, &elem, CMPI_uint16);
            }
            status = CMSetProperty(instance, IN_PARAM_INSTALL_OPTIONS_NAME,
                            &value, CMPI_uint16A);
            if (status.rc)
                goto array_err;
        }

        if (lmi_job_has_in_param(job, IN_PARAM_TARGET_NAME)) {
            variant = lmi_job_get_in_param(job, IN_PARAM_TARGET_NAME);
            if (g_variant_get_boolean(variant)) {
                g_variant_unref(variant);
                if ((op = lmi_get_computer_system_safe(ctx)) == NULL)
                    goto err;
                op = CMClone(op, &status);
                if (op == NULL)
                    goto err;
                value.ref = op;
                status = CMSetProperty(instance, IN_PARAM_TARGET_NAME,
                                &value, CMPI_ref);
                if (status.rc)
                    goto op_err;
            } else {
                g_variant_unref(variant);
            }
        }

        if (lmi_job_has_in_param(job, IN_PARAM_COLLECTION_NAME)) {
            variant = lmi_job_get_in_param(job, IN_PARAM_COLLECTION_NAME);
            if (g_variant_get_boolean(variant)) {
                g_variant_unref(variant);
                create_instance_id(LMI_SystemSoftwareCollection_ClassName, NULL,
                        instance_id, BUFLEN);
                op = CMNewObjectPath(cb, namespace,
                                LMI_SystemSoftwareCollection_ClassName, &status);
                if (op == NULL || status.rc)
                    goto err;
                value.string = CMNewString(cb, instance_id, &status);
                if (status.rc)
                    goto op_err;
                status = CMAddKey(op, "InstanceID", &value, CMPI_string);
                if (status.rc) {
                    CMRelease(value.string);
                    goto op_err;
                }
                value.ref = op;
                status = CMSetProperty(instance, IN_PARAM_COLLECTION_NAME,
                                &value, CMPI_ref);
                if (status.rc)
                    goto op_err;
            } else {
                g_variant_unref(variant);
            }
        }
        g_free(namespace);
    }

    if (include_output) {
        /* No output parameters that could be filled yet */
        variant = lmi_job_get_result(job);
        if (variant) {
            value.uint32 = g_variant_get_uint32(variant);
            status = CMSetProperty(instance, "__ReturnValue", &value, CMPI_uint32);
            g_variant_unref(variant);
            if (status.rc)
                goto err;
        }
    }

    return status;

op_err:
    CMRelease(op);
    goto err;
array_err:
    CMRelease(value.array);
    goto err;
string_err:
    CMRelease(value.string);
err:
    g_free(namespace);
    if (!status.rc) {
        lmi_error("Memory allocation failed");
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    }
    return status;
}

static void finish_partially_completed_installation_job(
    LmiJob *job,
    GCancellable *cancellable)
{
    gchar *jobid = lmi_job_get_jobid(job);
    lmi_warn("TODO: finish partially completed job \"%s\".", jobid);
    g_free(jobid);
}

static const gchar *pk_progress_type_to_string(PkProgressType ptype)
{
    switch (ptype) {
        case PK_PROGRESS_TYPE_PACKAGE_ID:
            return "package-id";
        case PK_PROGRESS_TYPE_TRANSACTION_ID:
            return "transaction-id";
        case PK_PROGRESS_TYPE_PERCENTAGE:
            return "percentage";
        case PK_PROGRESS_TYPE_ALLOW_CANCEL:
            return "allow-cancel";
        case PK_PROGRESS_TYPE_STATUS:
            return "status";
        case PK_PROGRESS_TYPE_ROLE:
            return "role";
        case PK_PROGRESS_TYPE_CALLER_ACTIVE:
            return "caller-active";
        case PK_PROGRESS_TYPE_ELAPSED_TIME:
            return "elapsed-time";
        case PK_PROGRESS_TYPE_REMAINING_TIME:
            return "remaining-time";
        case PK_PROGRESS_TYPE_SPEED:
            return "speed";
        case PK_PROGRESS_TYPE_DOWNLOAD_SIZE_REMAINING:
            return "download-size-remaining";
        case PK_PROGRESS_TYPE_UID:
            return "uid";
        case PK_PROGRESS_TYPE_PACKAGE:
            return "package";
        case PK_PROGRESS_TYPE_ITEM_PROGRESS:
            return "item-progress";
        case PK_PROGRESS_TYPE_TRANSACTION_FLAGS:
            return "transaction-flags";
        case PK_PROGRESS_TYPE_INVALID:
            return "invalid";
        default:
            return "unknown";
    }
}

static gboolean update_affected_packages(LmiJob *job, const char *package_id)
{
    GVariant *variant;
    const gchar * const *affected_orig = NULL;
    gchar **affected = NULL;
    gboolean found = FALSE;
    gsize length;
    gsize pkg_id_len;
    gboolean missing_version_info = FALSE;
    g_assert(LMI_IS_JOB(job));

    if (!pk_package_id_check(package_id)) {
        lmi_warn("Ignoring invalid package_id \"%s\".", package_id);
        return FALSE;
    }

    pkg_id_len = strlen(package_id);
    if (pkg_id_len >= 3 && strncmp(package_id + pkg_id_len - 3, ";;;", 3) == 0)
        missing_version_info = TRUE;

    JOB_CRITICAL_BEGIN(job);
    if (lmi_job_has_out_param(job, OUT_PARAM_AFFECTED_PACKAGES)) {
        variant = lmi_job_get_out_param(job, OUT_PARAM_AFFECTED_PACKAGES);
        affected_orig = g_variant_get_strv(variant, &length);
        if ((affected = g_new0(gchar *, length + 2)) == NULL)
            goto err;
        for (gsize i=0; i < length; ++i) {
            if ((affected[i] = g_strdup(affected_orig[i])) == NULL) {
                g_variant_unref(variant);
                goto affected_err;
            }
            if (!g_strcmp0(affected[i], package_id) ||
                    (missing_version_info &&
                     !strncmp(affected[i], package_id, pkg_id_len - 2)))
            {
                lmi_debug("Package id \"%s\" is already present in "
                        OUT_PARAM_AFFECTED_PACKAGES ".", package_id);
                found = TRUE;
                break;
            }
        }
        g_variant_unref(variant);
        if (!found) {
            if (missing_version_info) {
                lmi_warn("Not appending package id \"%s\" to "
                        OUT_PARAM_AFFECTED_PACKAGES
                        " which is missing version information.", package_id);
            } else {
                affected[length] = g_strdup(package_id);
                if ((variant = g_variant_new_strv((gchar const * const *) affected,
                                length + 1)) == NULL)
                    goto affected_err;
                lmi_job_set_out_param(job, OUT_PARAM_AFFECTED_PACKAGES, variant);
                lmi_debug("Appended new pkgid=%s to "
                        OUT_PARAM_AFFECTED_PACKAGES ".", package_id);
            }
        }
        g_strfreev(affected);
    } else {
        lmi_debug("Creating new " OUT_PARAM_AFFECTED_PACKAGES " with pkgid=%s.",
                package_id);
        if ((variant = g_variant_new_strv(&package_id, 1)) == NULL)
            goto err;
        lmi_job_set_out_param(job, OUT_PARAM_AFFECTED_PACKAGES, variant);
    }
    JOB_CRITICAL_END(job);

    return TRUE;

affected_err:
    g_strfreev(affected);
err:
    lmi_error("Memory allocation failed!");
    JOB_CRITICAL_END(job);
    return FALSE;
}

/**
 * Progress callback for packagekit functions. It updates job object.
 *
 * @param user_data Object of LmiJob.
 * */
static void progress_callback(PkProgress *progress,
                              PkProgressType type,
                              gpointer user_data)
{
    LmiJob *job = LMI_JOB(user_data);
    gchar *jobid = NULL;
    gchar *pkg_id = NULL, *pkg_id_item = NULL;
    gchar *tid = NULL;
    gint percent, percent_item;
    guint status, status_item;
    PkItemProgress *itemp = NULL;
    GVariant *variant = NULL;

    if (type != PK_PROGRESS_TYPE_PERCENTAGE &&
            type != PK_PROGRESS_TYPE_TRANSACTION_ID &&
            type != PK_PROGRESS_TYPE_ITEM_PROGRESS &&
            type != PK_PROGRESS_TYPE_STATUS &&
            type != PK_PROGRESS_TYPE_PACKAGE_ID)
        return;

    if ((jobid = lmi_job_get_jobid(job)) == NULL) {
        lmi_error("Memory allocation failed!");
        goto err;
    }
    g_object_get(progress,
            "status", &status,
            "percentage", &percent,
            "transaction-id", &tid,
            "item-progress", &itemp,
            "package-id", &pkg_id,
            "status", &status,
            NULL);
    if (!tid) {
        lmi_warn("Missing transaction-id property in progress object"
               " for job \"%s\". Ignoring.", jobid);
        goto err;
    }

    lmi_debug("Processing progress update of \"%s\" for job \"%s\""
            " (%s, %3d%% complete).",
            pk_progress_type_to_string(type), tid,
            pk_status_enum_to_string(status), percent);

    lmi_job_set_jobid(job, tid);
    switch (type) {
        case PK_PROGRESS_TYPE_STATUS:
            variant = g_variant_new_uint32(status);
            lmi_job_set_out_param(job, "Status", variant);
            lmi_debug("Status of job \"%s\" changed to \"%s\".",
                    tid, pk_status_enum_to_string(status));
            break;

        case PK_PROGRESS_TYPE_ITEM_PROGRESS:
            if (!itemp)
                break;
            g_object_get(itemp,
                    "status", &status_item,
                    "percentage", &percent_item,
                    "package-id", &pkg_id_item,
                    NULL);
            lmi_debug("Changed item progress of job \"%s\" regarding package"
                   " \"%s\" to \"%s\" with %3d%% completion.",
                    tid, pkg_id_item,
                    pk_status_enum_to_string(status_item), percent);
            if (!update_affected_packages(job, pkg_id_item))
                goto err;
            g_free(pkg_id_item);
            break;

        case PK_PROGRESS_TYPE_PACKAGE_ID:
            lmi_debug("Changed package id of job \"%s\" to %s.", tid, pkg_id);
            if (!update_affected_packages(job, pkg_id))
                goto err;
            break;

        case PK_PROGRESS_TYPE_PERCENTAGE:
            if (!lmi_job_is_finished(job))
                lmi_job_set_percent_complete(job, percent);
            break;

        default:
            break;
    }
    g_free(jobid);
    return;

err:
    g_free(pkg_id_item);
    g_free(tid);
    g_free(pkg_id);
    g_clear_object(&itemp);
    g_free(jobid);
    return;
}

/**
 * Process package installation or update request. First it checks whether the
 * package is already installed or not.
 *
 * @param package_ids Is an array of package ids at least 2 pointers long.
 *      First pointer shall point to an id of package that shall be installed.
 *      Second must be `NULL`.
 * @param sw_pkg Must be prefilled with information corresponding to the same
 *      package as in *package_ids*.
 * @param update Whether an installed package shall be updated.
 * @param force Whether to forcibly reinstall already installed package. Or
 *      downgrade if a desired one is older than installed.
 * @param repair Whether to reinstall already installed package.
 * @param status_code Will be set to corresponding code if the installation
 *      fails.
 * @return Results object returned by PackageKit function used to install the
 *      package.
 */
static PkResults *install_or_update_package(PkTask *task,
                                  LmiJob *job,
                                  const gchar * const *package_ids,
                                  const SwPackage *sw_pkg,
                                  gboolean update,
                                  gboolean force,
                                  gboolean repair,
                                  GCancellable *cancellable,
                                  LmiJobStatusCodeEnum *status_code,
                                  gchar *err_buf,
                                  guint err_buflen)
{
    PkResults *results = NULL;
    GError *gerror = NULL;
    PkTask *tmp_task = pk_task_new();
    PkPackage *installed_pkg = NULL;
    gchar *values[] = {NULL, NULL};
    GPtrArray *array;
    PkBitfield filter = 0;
    gchar *jobid = lmi_job_get_jobid(job);
    g_assert(PK_IS_TASK(task));
    g_assert(package_ids);
    g_assert(package_ids[0] != NULL);
    g_assert(package_ids[1] == NULL);
    g_assert(sw_pkg);
    g_assert(status_code);
    g_assert(err_buf);

    values[0] = sw_pkg->name;
    pk_bitfield_add(filter, PK_FILTER_ENUM_INSTALLED);
    results = pk_task_search_names_sync(tmp_task, filter,
           values, cancellable, NULL, NULL, &gerror);
    if (check_and_create_error_msg(results, gerror,
                "Lookup of installed package failed", err_buf, err_buflen))
    {
        if (results != NULL &&
                pk_results_get_exit_code(results) == PK_EXIT_ENUM_CANCELLED)
        {
            lmi_warn("Installation job \"%s\" has been terminated.", jobid);
            err_buf[0] = 0;
        }
        goto err;
    }

    if (cancellable && g_cancellable_is_cancelled(cancellable)) {
        g_clear_object(&results);
        goto err;
    }

    if ((array = pk_results_get_package_array(results)) == NULL) {
        lmi_error("Memory allocation failed");
        *status_code = LMI_JOB_STATUS_CODE_ENUM_FAILED;
        goto err;
    }
    for (gsize i = 0; i < array->len; ++i) {
        installed_pkg = g_object_ref(g_ptr_array_index(array, i));
        if (!g_strcmp0(pk_package_get_name(installed_pkg), sw_pkg->name) &&
                !g_strcmp0(pk_package_get_arch(installed_pkg), sw_pkg->arch))
            break;
        installed_pkg = NULL;
    }
    g_clear_object(&results);

    if (!update && installed_pkg != NULL) {
        if (pk_package_id_equal_fuzzy_arch(
                    pk_package_get_id(installed_pkg),
                    package_ids[0]))
        {
            if (force || repair) {
                /* TODO: reinstall with rpmlib */
                lmi_debug("Reinstalling installed package \"%s\".", package_ids[0]);
                snprintf(err_buf, err_buflen,
                        "Package reinstallation is currently not supported.");
                *status_code = LMI_JOB_STATUS_CODE_ENUM_NOT_SUPPORTED;
            } else {
                snprintf(err_buf, err_buflen,
                        "Package \"%s\" is already installed.", package_ids[0]);
                *status_code = LMI_JOB_STATUS_CODE_ENUM_ALREADY_EXISTS;
            }
            goto array_err;
        }
        /* TODO: Check whether we downgrade or not. Downgrade is not yet
         * supported by PackageKit. It will return "The packages are already
         * all installed" for such an attempt. This will be difficult because
         * there is no generic function making comparison of two arbitrary
         * package ids.
         */
    } else if (update && installed_pkg == NULL) {
        snprintf(err_buf, err_buflen,
                "Package \"%s\" is not installed.", package_ids[0]);
        *status_code = LMI_JOB_STATUS_CODE_ENUM_NOT_FOUND;
        goto array_err;
    }

    if (update) {
        results = pk_task_update_packages_sync(
                task,
                (gchar **) package_ids,     /* no modification is done */
                cancellable, progress_callback, job, &gerror);
    } else {
        results = pk_task_install_packages_sync(
                task,
                (gchar **) package_ids,     /* no modification is done */
                cancellable, progress_callback, job, &gerror);
    }
    g_ptr_array_unref(array);

    if (check_and_create_error_msg(results, gerror, "Installation failed",
                err_buf, err_buflen))
    {
        if (results != NULL &&
                pk_results_get_exit_code(results) == PK_EXIT_ENUM_CANCELLED)
        {
            lmi_warn("Installation job \"%s\" has been terminated.", jobid);
            err_buf[0] = 0;
        }
        goto err;
    }
    g_free(jobid);
    return results;

array_err:
    g_ptr_array_unref(array);
err:
    g_clear_object(&results);
    g_clear_error(&gerror);
    g_free(jobid);
    return results;
}

void lmi_sw_installation_job_process(LmiJob *job,
                                     GCancellable *cancellable)
{
    PkTask *task;
    guint32 op;
    GVariant *variant;
    gchar const *source;
    SwPackage sw_pkg;
    PkPackage *pk_pkg = NULL;
    gchar const *package_ids[] = {NULL, NULL};
    PkResults *results;
    GPtrArray *packages;
    LmiJobStatusCodeEnum status_code = LMI_JOB_STATUS_CODE_ENUM_FAILED;
    gchar *error = NULL;
    GError *gerror = NULL;
    gchar error_msg[BUFLEN] = "";
    gchar *jobid = NULL;
    PkBitfield filter = 0L;
    g_assert(LMI_IS_SW_INSTALLATION_JOB(job));

    lmi_debug("Started processing installation job %u.",
            lmi_job_get_number(job));

    if (cancellable && g_cancellable_is_cancelled(cancellable))
        return;
    if (lmi_job_has_own_jobid(job)) {
        finish_partially_completed_installation_job(job, cancellable);
    } else {
        if ((jobid = lmi_job_get_jobid(job)) == NULL) {
            error = "Memory allocation failed!";
            goto err;
        }
        if (!lmi_job_has_in_param(job, IN_PARAM_INSTALL_OPTIONS_NAME) ||
                !lmi_job_has_in_param(job, IN_PARAM_SOURCE_NAME))
        {
            lmi_error("Missing one of { %s, %s } input arguments in job \"%s\"!"
                    " Terminating job processing.",
                            IN_PARAM_INSTALL_OPTIONS_NAME,
                            IN_PARAM_SOURCE_NAME, jobid);
            g_snprintf(error_msg, BUFLEN,
                    "Missing one of { %s, %s } input arguments.",
                        IN_PARAM_INSTALL_OPTIONS_NAME, IN_PARAM_SOURCE_NAME);
            status_code = LMI_JOB_STATUS_CODE_ENUM_INVALID_PARAMETER;
            goto err;
        }

        task = pk_task_new();
        g_object_set(task, "interactive", FALSE, NULL);
        variant = lmi_job_get_in_param(job, IN_PARAM_INSTALL_OPTIONS_NAME);
        op = g_variant_get_uint32(variant);
        g_variant_unref(variant);
        variant = lmi_job_get_in_param(job, IN_PARAM_SOURCE_NAME);
        source = g_variant_get_string(variant, NULL);
        g_variant_unref(variant);

        init_sw_package(&sw_pkg);

        if (create_sw_package_from_elem_name(source, &sw_pkg) != 0) {
            status_code = LMI_JOB_STATUS_CODE_ENUM_INVALID_PARAMETER;
            error = "Failed to parse InstanceID.";
            goto err;
        }

        /* Given package must exist. Try to find it. */
        if (op & INSTALLATION_OPERATION_REMOVE) {
            pk_bitfield_add(filter, PK_FILTER_ENUM_INSTALLED);
        }
        if (!get_pk_pkg_from_sw_pkg(&sw_pkg, filter, &pk_pkg)) {
            clean_sw_package(&sw_pkg);
            status_code = LMI_JOB_STATUS_CODE_ENUM_NOT_FOUND;
            error = "Failed to find matching package.";
            goto err;
        }

        if (cancellable && g_cancellable_is_cancelled(cancellable))
            goto sw_pkg_err;

        *package_ids = pk_package_get_id(pk_pkg);
        if (!update_affected_packages(job, *package_ids))
        {
            error = "Memory allocation failed!";
            goto sw_pkg_err;
        }

        if (op & (INSTALLATION_OPERATION_INSTALL | INSTALLATION_OPERATION_UPDATE)) {
            lmi_debug("Starting synchronous package %s task for"
                    " job \"%s\".", (op & INSTALLATION_OPERATION_UPDATE) ?
                    "update" : "installation", jobid);
            results = install_or_update_package(task, job, package_ids, &sw_pkg,
                    op & INSTALLATION_OPERATION_UPDATE,
                    op & INSTALLATION_OPERATION_FORCE,
                    op & INSTALLATION_OPERATION_REPAIR,
                    cancellable, &status_code, error_msg, BUFLEN);
            if (results == NULL)
                goto sw_pkg_err;
        } else {
            lmi_debug("Starting synchronous package removal task for"
                    " job \"%s\".", jobid);
            results = pk_task_remove_packages_sync(
                    task,
                    (gchar **) package_ids,     /* no modification is done */
                    op & INSTALLATION_OPERATION_FORCE,  /* allow dependencies */
                    TRUE,                               /* auto remove */
                    cancellable, progress_callback, job, &gerror);
            error = "Uninstallation failed";
        }
        clean_sw_package(&sw_pkg);
        g_clear_object(&pk_pkg);

        if (check_and_create_error_msg(results, gerror, error,
                    error_msg, BUFLEN))
        {
            if (results != NULL &&
                    pk_results_get_exit_code(results) == PK_EXIT_ENUM_CANCELLED)
            {
                lmi_warn("Installation job \"%s\" has been terminated.", jobid);
                error = NULL;
                g_clear_error(&gerror);
            }
            goto sw_pkg_err;
        }

        if ((packages = pk_results_get_package_array(results)) == NULL) {
            error = "Memory allocation failed!";
            goto results_err;
        }
        g_clear_object(&results);
        for (gsize i=0; i < packages->len; ++i) {
            pk_pkg = g_ptr_array_index(packages, i);
            if (!update_affected_packages(job, pk_package_get_id(pk_pkg)))
            {
                error = "Memory allocation failed!";
                goto packages_err;
            }
        }
        g_ptr_array_unref(packages);
        lmi_job_finish_ok_with_code(job,
                INSTALL_METHOD_RESULT_JOB_COMPLETED_WITH_NO_ERROR);
    }

    return;

packages_err:
    g_ptr_array_unref(packages);
results_err:
    g_clear_object(&results);
    goto err;
sw_pkg_err:
    clean_sw_package(&sw_pkg);
err:
    if (gerror)
        error = gerror->message;
    if (error_msg[0])
        error = error_msg;
    if (error) {
        lmi_error("Installation job \"%s\" failed: %s", jobid, error);
        lmi_job_finish_exception(job, status_code, error);
    } else {
        lmi_warn("Terminating installation job \"%s\".", jobid);
        lmi_job_finish_terminate(job);
    }
    g_clear_error(&gerror);
    g_free(jobid);
}

void lmi_sw_verification_job_process(LmiJob *job,
                                     GCancellable *cancellable)
{
    g_assert(LMI_IS_SW_VERIFICATION_JOB(job));
    gchar *jobid = lmi_job_get_jobid(job);
    lmi_warn("TODO: process verification job \"%s\".", jobid);
    g_free(jobid);
}
