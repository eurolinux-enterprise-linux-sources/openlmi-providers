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
 * Authors: Michal Minar <miminar@redhat.com>
 */

#include <pthread.h>
#include "openlmi-utils.h"
#include "sw-utils.h"

#include "ind_sender.h"
#include "lmi_sw_job.h"
#include "job_manager.h"

#include "LMI_SystemSoftwareCollection.h"
#include "LMI_SoftwareInstallationJob.h"
#include "LMI_SoftwareVerificationJob.h"
#include "LMI_SoftwareIdentityResource.h"

#define PKG_ID_DATA_INSTALLED_PREFIX "installed"
#define PKG_ID_DATA_INSTALLED_PREFIX_LEN 9

const char *profile_name = "software";
const ConfigEntry *provider_config_defaults = NULL;
PkControl *pkctrl = NULL;
static const gchar *_distro_id = NULL;
static const gchar *_thread_name = "packagekit-listener";
/**
 * Guard for initialization and cleanup functions.
 */
static pthread_mutex_t _init_lock = PTHREAD_MUTEX_INITIALIZER;
/**
 * Number of calls to `software_init()` minus calls to `software_cleanup()`.
 * This is to ensure that static variables are initialised upon initialization
 * of the first software provider invoked and cleaned up at the clean up of the
 * last one. */
static gint _initialized = 0;
/**
 * CMPI context for `_main_thread`.
 */
static const CMPIContext *_cmpi_ctx = NULL;
/**
 * Context used in `_main_loop`. `pktrl` needs to be created with this context
 * being the default in order for events to be handled in the `_main_loop`.
 */
static GMainContext *_main_ctx = NULL;
/**
 * Runs the `_main_loop`.
 */
static GThread *_main_thread = NULL;
/**
 * Running in its own thread which is attached to a broker until the last
 * provider is cleaned up. It listens and processes events from packagekit.
 */
static GMainLoop *_main_loop = NULL;

static JobTypeEntry _sw_job_type_entries[] = \
    { { 0 // needs to be set later with LMI_TYPE_SW_INSTALLATION_JOB
      , LMI_SoftwareInstallationJob_ClassName
      , lmi_sw_job_to_cim_instance
      , lmi_sw_job_make_job_parameters
      , METHOD_RESULT_VALUE_TYPE_UINT32
      , lmi_sw_installation_job_process
      , TRUE, TRUE }
    , { 0 // needs to be set later with LMI_TYPE_SW_VERIFICATION_JOB
      , LMI_SoftwareVerificationJob_ClassName
      , lmi_sw_job_to_cim_instance
      , lmi_sw_job_make_job_parameters
      , METHOD_RESULT_VALUE_TYPE_UINT32
      , lmi_sw_verification_job_process
      , TRUE, TRUE }
    };
#define JOB_TYPE_ENTRIES_COUNT 2

/**
 * Container for parameters of `_watch_packagekit()` function.
 */
struct _WatchPackageKitParams {
    const CMPIBroker *cb;
    pthread_mutex_t mutex;
    pthread_cond_t thread_started;
};

/******************************************************************************
 * Provider init and cleanup
 *****************************************************************************/
/**
 * Thread running `_main_loop` which listens for event from PackageKit.
 * For example packagekit daemon has been terminated triggers an action
 * in PkControl object that destroys a proxy object. Without this being
 * triggered, all the following requests from us would fail due to
 * obsolete proxy object.
 *
 * @param data Pointer to `struct _WatchPackageKitParams`.
 */
static gpointer _watch_packagekit(gpointer data) {
    struct _WatchPackageKitParams *params = data;
    g_assert(pkctrl == NULL);
    g_assert(_main_loop != NULL);
    g_assert(_main_ctx != NULL);

    lmi_debug("PackageKit listener thread started");

    CBAttachThread(params->cb, _cmpi_ctx);
    g_main_context_push_thread_default(_main_ctx);

    pthread_mutex_lock(&params->mutex);
    pkctrl = pk_control_new();
    pthread_cond_signal(&params->thread_started);
    pthread_mutex_unlock(&params->mutex);
    g_main_loop_run(_main_loop);

    lmi_debug("PackageKit listener terminating");

    g_clear_object(&pkctrl);
    CBDetachThread(params->cb, _cmpi_ctx);

    return NULL;
}

void software_init(const gchar *provider_name,
                   const CMPIBroker *cb,
                   const CMPIContext *ctx,
                   gboolean is_indication_provider,
                   const ConfigEntry *provider_config_defaults)
{
    CMPIStatus status;

    pthread_mutex_lock(&_init_lock);
    if (_initialized < 1) {
        /* Run the `_main_loop` in `_main_thread`. */
        struct _WatchPackageKitParams params = {
            cb,
            PTHREAD_MUTEX_INITIALIZER,
            PTHREAD_COND_INITIALIZER
        };
        lmi_init(profile_name, cb, ctx, provider_config_defaults);
        _main_ctx = g_main_context_new();
        if (_main_ctx == NULL)
            goto err;
        _main_loop = g_main_loop_new(_main_ctx, FALSE);
        if (_main_loop == NULL)
            goto main_ctx_err;
        _cmpi_ctx = CBPrepareAttachThread(cb, ctx);
        if (_cmpi_ctx == NULL)
            goto main_loop_err;

        pthread_mutex_lock(&params.mutex);
        _main_thread = g_thread_new(_thread_name,
                _watch_packagekit, (gpointer) &params);

        /* Wait for the `_main_thread` to initialize objects. */
        pthread_cond_wait(&params.thread_started, &params.mutex);
        pthread_mutex_unlock(&params.mutex);
        pthread_cond_destroy(&params.thread_started);
        pthread_mutex_destroy(&params.mutex);
    }

    /* Start the job manager. Needs to be called for every provider. */
    _sw_job_type_entries[0].job_type = LMI_TYPE_SW_INSTALLATION_JOB;
    _sw_job_type_entries[1].job_type = LMI_TYPE_SW_VERIFICATION_JOB;
    if ((status = jobmgr_init(cb, ctx,
                    "Software",     /* profile_name */
                    provider_name,
                    is_indication_provider,
                    false,          /* concurrent_processing */
                    _sw_job_type_entries, JOB_TYPE_ENTRIES_COUNT)).rc)
    {
        lmi_error("Failed to initialize provider %s: %s",
                provider_name, status.msg ?
                     CMGetCharsPtr(status.msg, NULL) : "unknown error");
        goto cmpi_ctx_err;
    }

    ++_initialized;
    pthread_mutex_unlock(&_init_lock);
    lmi_debug("Software provider %s initialized.", provider_name);
    return;

cmpi_ctx_err:
    _cmpi_ctx = NULL;
main_loop_err:
    g_main_loop_unref(_main_loop);
    _main_loop = NULL;
main_ctx_err:
    g_main_context_unref(_main_ctx);
    _main_ctx = NULL;
err:
    pthread_mutex_unlock(&_init_lock);
    lmi_debug("Failed to initialized software provider %s.", provider_name);
}

CMPIStatus software_cleanup(const gchar *provider_name)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    lmi_debug("Software provider %s cleanup started.", provider_name);
    pthread_mutex_lock(&_init_lock);

    /* Cleanup job manager. Needs to be called for every provider. */
    status = jobmgr_cleanup(provider_name);

    if (_initialized == 1) {
        /* Stop the `_main_loop`. */
        g_clear_pointer(&_distro_id, g_free);
        g_main_loop_quit(_main_loop);
        g_thread_join(_main_thread);
        g_main_loop_unref(_main_loop);
        _main_thread = NULL;
        _main_loop = NULL;
        g_main_context_unref(_main_ctx);
        _main_ctx = NULL;
        _cmpi_ctx = NULL;
    }

    --_initialized;
    pthread_mutex_unlock(&_init_lock);
    lmi_debug("Software provider %s cleanup finished.", provider_name);
    return status;
}

/*******************************************************************************
 * SwPackage & related functions
 ******************************************************************************/

void init_sw_package(SwPackage *pkg)
{
    pkg->name = NULL;
    pkg->epoch = NULL;
    pkg->version = NULL;
    pkg->release = NULL;
    pkg->arch = NULL;
    pkg->pk_version = NULL;
}

void clean_sw_package(SwPackage *pkg)
{
    free(pkg->name);
    free(pkg->epoch);
    free(pkg->version);
    free(pkg->release);
    free(pkg->arch);
    free(pkg->pk_version);

    init_sw_package(pkg);
}

void free_sw_package(SwPackage *pkg)
{
    clean_sw_package(pkg);
    g_free(pkg);
}

short create_sw_package_from_pk_pkg_id(const char *pk_pkg_id, SwPackage *sw_pkg)
{
    short ret = -1;
    char *delim;
    const gchar *id, *name, *arch, *ver;
    gchar **split_id = NULL;

    init_sw_package(sw_pkg);

    if (!pk_pkg_id || !*pk_pkg_id) {
        lmi_warn("Empty package ID!");
        goto done;
    }

    id = pk_pkg_id;
    split_id = pk_package_id_split(id);
    if (!split_id) {
        lmi_warn("Invalid package ID: %s!", id);
        goto done;
    }

    if (!(name = split_id[PK_PACKAGE_ID_NAME])) {
        lmi_warn("Package with ID: %s doesn't have name!", id);
        goto done;
    }
    if (!(ver = split_id[PK_PACKAGE_ID_VERSION])) {
        lmi_warn("Package with ID: %s doesn't have version!", id);
        goto done;
    }
    if (!(arch = split_id[PK_PACKAGE_ID_ARCH])) {
        lmi_warn("Package with ID: %s doesn't have architecture!", id);
        goto done;
    }

    sw_pkg->name = strdup(name);
    sw_pkg->arch = strdup(arch);
    sw_pkg->pk_version = strdup(ver);

    if ((delim = strchr(ver, ':'))) {
        sw_pkg->epoch = strndup(ver, delim - ver);
        ver = delim + 1;
    } else {
        sw_pkg->epoch = strdup("0");
    }
    if ((delim = strrchr(ver, '-'))) {
        sw_pkg->version = strndup(ver, delim - ver);
        sw_pkg->release = strdup(delim + 1);
    } else {
        sw_pkg->version = strdup(ver);
        sw_pkg->release = strdup("0");
        lmi_warn("Package with ID: %s doesn't have release number! Using '0' instead.",
                id);
    }

    if (!sw_pkg->name || !sw_pkg->arch || !sw_pkg->epoch || !sw_pkg->version
            || !sw_pkg->release || !sw_pkg->pk_version) {
        lmi_warn("Memory allocation failed.");
        goto done;
    }

    ret = 0;

done:
    if (ret != 0) {
        clean_sw_package(sw_pkg);
    }

    return ret;
}

short create_sw_package_from_pk_pkg(PkPackage *pk_pkg, SwPackage *sw_pkg)
{
    return create_sw_package_from_pk_pkg_id(pk_package_get_id(pk_pkg), sw_pkg);
}

short create_sw_package_from_elem_name(const char *elem_name, SwPackage *sw_pkg)
{
    short ret = -1;
    char *en = NULL, *delim;

    init_sw_package(sw_pkg);

    if (!elem_name || !*elem_name) {
        lmi_warn("Empty element name.");
        goto done;
    }

    if (!(en = strdup(elem_name))) {
        lmi_warn("Memory allocation failed.");
        goto done;
    }

    if (!(delim = strrchr(en, '.'))) {
        lmi_warn("Invalid element name of the package: %s", elem_name);
        goto done;
    }
    sw_pkg->arch = strdup(delim + 1);
    delim[0] = '\0';

    if (!(delim = strrchr(en, '-'))) {
        lmi_warn("Invalid element name of the package: %s", elem_name);
        goto done;
    }
    sw_pkg->release = strdup(delim + 1);
    delim[0] = '\0';

    if ((delim = strrchr(en, ':'))) {
        sw_pkg->version = strdup(delim + 1);
        delim[0] = '\0';

        if (!(delim = strrchr(en, '-'))) {
            lmi_warn("Invalid element name of the package: %s", elem_name);
            goto done;
        }
        sw_pkg->epoch = strdup(delim + 1);
        delim[0] = '\0';
    } else {
        if (!(delim = strrchr(en, '-'))) {
            lmi_warn("Invalid element name of the package: %s", elem_name);
            goto done;
        }
        sw_pkg->version = strdup(delim + 1);
        delim[0] = '\0';

        sw_pkg->epoch = strdup("0");
    }

    sw_pkg->name = strdup(en);

    if (!sw_pkg->name || !sw_pkg->arch || !sw_pkg->epoch || !sw_pkg->version
            || !sw_pkg->release) {
        lmi_warn("Memory allocation failed.");
        goto done;
    }

    if (strcmp(sw_pkg->epoch, "0") == 0) {
        if (asprintf(&sw_pkg->pk_version, "%s-%s", sw_pkg->version,
                sw_pkg->release) < 0) {
            lmi_warn("Memory allocation failed.");
            goto done;
        }
    } else {
        if (asprintf(&sw_pkg->pk_version, "%s:%s-%s", sw_pkg->epoch,
                sw_pkg->version, sw_pkg->release) < 0) {
            lmi_warn("Memory allocation failed.");
            goto done;
        }
    }

    ret = 0;

done:
    if (en)
	free(en);

    if (ret != 0) {
        clean_sw_package(sw_pkg);
    }

    return ret;
}

void sw_pkg_get_version_str(const SwPackage *pkg, char *ver_str,
        const unsigned ver_str_len)
{
    snprintf(ver_str, ver_str_len, "%s:%s-%s.%s", pkg->epoch, pkg->version,
            pkg->release, pkg->arch);
}

void sw_pkg_get_element_name(const SwPackage *pkg, char *elem_name,
        const unsigned elem_name_len)
{
    snprintf(elem_name, elem_name_len, "%s-%s:%s-%s.%s", pkg->name, pkg->epoch,
            pkg->version, pkg->release, pkg->arch);
}

/*******************************************************************************
 * Functions related to single PkPackage
 ******************************************************************************/

short is_elem_name_installed(const char *elem_name)
{
    SwPackage sw_pkg;
    short ret = 0;

    init_sw_package(&sw_pkg);

    if (create_sw_package_from_elem_name(elem_name, &sw_pkg) != 0)
        goto done;
    ret = get_pk_pkg_from_sw_pkg(&sw_pkg,
            pk_bitfield_value(PK_FILTER_ENUM_INSTALLED), NULL);

done:
    clean_sw_package(&sw_pkg);

    return ret;
}

gboolean get_pk_pkg_from_sw_pkg(const SwPackage *sw_pkg,
                                PkBitfield filters,
                                PkPackage **pk_pkg)
{
    PkTask *task = NULL;
    PkPackage *item = NULL;
    PkPackage *result = NULL;
    PkPackage *installed = NULL;
    PkResults *results = NULL;
    GPtrArray *array = NULL;
    GError *gerror = NULL;
    gchar **values = NULL;
    unsigned i;
    char error_msg[BUFLEN] = "";

    task = pk_task_new();

    values = g_new0(gchar*, 2);
    values[0] = g_strdup(sw_pkg->name);

    results = pk_task_search_names_sync(task, filters, values,
            NULL, NULL, NULL, &gerror);
    if (check_and_create_error_msg(results, gerror, "Resolving package failed",
            error_msg, BUFLEN)) {
        lmi_warn(error_msg);
        goto err;
    }

    array = pk_results_get_package_array(results);
    g_clear_object(&results);
    for (i = 0; i < array->len; i++) {
        item = g_ptr_array_index(array, i);
        if (g_strcmp0(pk_package_get_name(item), sw_pkg->name) == 0 &&
               g_strcmp0(pk_package_get_arch(item), sw_pkg->arch) == 0)
        {
            if (g_strcmp0(pk_package_get_version(item),
                        sw_pkg->pk_version) == 0)
            {
                result = g_object_ref(item);
                break;
            } else if (pk_pkg_is_installed(item)) {
                /* Package we're looking for can't be obtained because
                 * the same package of different version is already installed.
                 * We need to query package details to get its package-id.
                 */
                installed = g_object_ref(item);
            }
        }
    }
    g_clear_pointer(&values, g_strfreev);
    g_clear_pointer(&array, g_ptr_array_unref);

    if (result == NULL && installed != NULL &&
            !pk_bitfield_contain(filters, PK_FILTER_ENUM_INSTALLED))
    {   /* Package was not found but the same one with different version is
         * installed. The only way how to check whether it is available or not
         * is to query package details with full package-id. Let's try it for
         * all enabled repositories whose ids are part of package-ids. */
        gchar *repo_id = NULL;

        get_pk_repos(REPO_STATE_ENUM_ENABLED, &array, error_msg, BUFLEN);
        if (!array) {
            lmi_warn(error_msg);
            goto err;
        }

        /* Build a list of package-ids of the same package varying only in
         * repository id. */
        if ((values = g_new0(gchar*, array->len + 1)) == NULL)
            goto err;
        for (guint j = 0, i = 0; i < array->len; i++) {
            g_object_get(g_ptr_array_index(array, i),
                    "repo-id", &repo_id,
                    NULL);
            values[j++] = pk_package_id_build(sw_pkg->name,
                    sw_pkg->pk_version, sw_pkg->arch, repo_id);
            g_clear_pointer(&repo_id, g_free);
        }
        g_clear_pointer(&array, g_ptr_array_unref);

        /* Resolve them into package details. We expect to get 1 match at most.
         */
        if (values[0] != NULL) {
            results = pk_task_get_details_sync(task, values,
                    NULL, NULL, NULL, &gerror);
            if (check_and_create_error_msg(results, gerror,
                    "Getting package details failed", error_msg, BUFLEN)) {
                lmi_warn(error_msg);
                goto err;
            }
        }

        array = pk_results_get_details_array(results);
        g_clear_object(&results);
        if (array && array->len > 0) {
            /* The package is available. Let's build PkPackage object ourselves
             * with known informations. */
            GParamSpec **pspecs = NULL;
            guint nprops, skipped = 0;
            GParameter *params = NULL;
            PkDetails *pk_det = NULL;
            gchar *pkg_id = NULL;

            pk_det = g_ptr_array_index(array, 0);
            /* Get a list of properties of PkPackageClass. */
            pspecs = g_object_class_list_properties(
                    G_OBJECT_GET_CLASS(installed), &nprops);
            params = g_new0(GParameter, nprops);
            for (guint i = 0, j = 0; i < nprops; ++i) {
                /* Build a list of properties . */
                if (!(pspecs[i]->flags & G_PARAM_WRITABLE)) {
                    /* This is a case of "package-id" property. */
                    ++skipped;
                    continue;
                }
                params[j].name = pspecs[i]->name;
                g_value_init(&params[j].value, pspecs[i]->value_type);
                if (g_object_class_find_property(
                            G_OBJECT_GET_CLASS(pk_det), pspecs[i]->name))
                {   /* Get from PkDetails object what we can. */
                    g_object_get_property(G_OBJECT(pk_det),
                            pspecs[i]->name, &params[j++].value);
                } else {
                    /* Get the rest from installed package. */
                    g_object_get_property(G_OBJECT(installed),
                            pspecs[i]->name, &params[j++].value);
                }
            }
            /* Create PkPackage object. */
            result = g_object_newv(PK_TYPE_PACKAGE, nprops - skipped, params);
            for (guint i = 0; i < nprops - skipped; ++i) {
                g_value_unset(&params[i].value);
            }
            g_free(pspecs);
            /* *package-id* needs to be set manually since it's not writable
             * property. */
            g_object_get(pk_det, "package-id", &pkg_id, NULL);
            if (!pk_package_set_id(result, pkg_id, &gerror)) {
                lmi_error("Failed to set package-id (%s) property to"
                       " PkPackage object.", pkg_id);
                g_free(pkg_id);
                goto err;
            }
            g_free(pkg_id);
        }
        g_clear_pointer(&array, g_ptr_array_unref);
        g_strfreev(values);
    }
    g_clear_object(&installed);
    g_object_unref(task);

    if (pk_pkg != NULL && result != NULL) {
        *pk_pkg = result;
    } else if (result) {
        g_object_unref(result);
    }

    return result != NULL;

err:
    g_clear_pointer(&values, g_strfreev);
    g_clear_error(&gerror);
    g_clear_pointer(&array, g_ptr_array_unref);
    g_clear_object(&results);
    g_clear_object(&task);
    g_clear_object(&result);
    g_clear_object(&installed);
    return FALSE;
}

void get_pk_det_from_pk_pkg(PkPackage *pk_pkg, PkDetails **pk_det, PkTask *task_p)
{
    PkDetails *item = NULL;
    GPtrArray *array = NULL;
    gchar **values = NULL;
    unsigned i;

    values = g_new0(gchar*, 2);
    values[0] = g_strdup(pk_package_get_id(pk_pkg));

    get_pk_det_from_array(values, &array, task_p);
    if (!array) {
        goto done;
    }

    for (i = 0; i < array->len; i++) {
        item = g_ptr_array_index(array, i);
        gchar *pkg_id = NULL;
        g_object_get(item, "package-id", &pkg_id, NULL);
        if (pk_pkg_id_cmp(pkg_id, (gpointer) pk_package_get_id(pk_pkg)) == 0) {
            g_free(pkg_id);
            *pk_det = g_object_ref(item);
            break;
        }
        g_free(pkg_id);
        pkg_id = NULL;
    }

done:
    g_strfreev(values);

    if (array) {
        g_ptr_array_unref(array);
    }

    return;
}

void create_instance_from_pkgkit_data(PkPackage *pk_pkg, PkDetails *pk_det,
        SwPackage *sw_pkg, const CMPIBroker *cb, const char *ns,
        LMI_SoftwareIdentity *w)
{
    const gchar *summary;
    gchar *desc = NULL;
    char elem_name[BUFLEN] = "", ver_str[BUFLEN] = "",
            instance_id[BUFLEN] = "";

    summary = pk_package_get_summary(pk_pkg);
    if (pk_det) {
        g_object_get(pk_det, "description", &desc, NULL);
    }

    sw_pkg_get_element_name(sw_pkg, elem_name, BUFLEN);
    sw_pkg_get_version_str(sw_pkg, ver_str, BUFLEN);

    create_instance_id(LMI_SoftwareIdentity_ClassName, elem_name, instance_id,
            BUFLEN);

    LMI_SoftwareIdentity_Init(w, cb, ns);
    LMI_SoftwareIdentity_Set_InstanceID(w, instance_id);

    LMI_SoftwareIdentity_Init_Classifications(w, 1);
    LMI_SoftwareIdentity_Set_Classifications(w, 0, 0);
    LMI_SoftwareIdentity_Set_IsEntity(w, 1);

    LMI_SoftwareIdentity_Set_Architecture(w, sw_pkg->arch);
    LMI_SoftwareIdentity_Set_ElementName(w, elem_name);
    LMI_SoftwareIdentity_Set_Epoch(w, atoi(sw_pkg->epoch));
    LMI_SoftwareIdentity_Set_Name(w, sw_pkg->name);
    LMI_SoftwareIdentity_Set_Release(w, sw_pkg->release);
    LMI_SoftwareIdentity_Set_Version(w, sw_pkg->version);
    LMI_SoftwareIdentity_Set_VersionString(w, ver_str);

    if (summary) {
        LMI_SoftwareIdentity_Set_Caption(w, summary);
    }
    if (desc) {
        LMI_SoftwareIdentity_Set_Description(w, desc);
        g_free(desc);
    }

    return;
}

void get_files_of_pk_pkg(PkPackage *pk_pkg, GStrv *files, char *error_msg,
        const unsigned error_msg_len)
{
    PkTask *task = NULL;
    PkResults *results = NULL;
    GError *gerror = NULL;
    GPtrArray *array = NULL;
    gchar **values = NULL;

    task = pk_task_new();

    values = g_new0(gchar*, 2);
    values[0] = g_strdup(pk_package_get_id(pk_pkg));

    results = pk_task_get_files_sync(task, values, NULL, NULL, NULL, &gerror);
    if (check_and_create_error_msg(results, gerror,
            "Getting files of package failed", error_msg, error_msg_len)) {
        goto done;
    }

    array = pk_results_get_files_array(results);
    if (array->len != 1) {
        snprintf(error_msg, error_msg_len, "Getting files of package failed");
        goto done;
    }

    g_object_get(g_ptr_array_index(array, 0), "files", files, NULL);

done:
    g_strfreev(values);
    g_clear_error(&gerror);

    if (array) {
        g_ptr_array_unref(array);
    }
    if (results) {
        g_object_unref(results);
    }
    if (task) {
        g_object_unref(task);
    }

    return;
}

short is_file_part_of_pk_pkg(const char *file, PkPackage *pk_pkg)
{
    short ret = 0;
    unsigned i = 0;
    GStrv files = NULL;
    char error[BUFLEN] = "";

    get_files_of_pk_pkg(pk_pkg, &files, error, BUFLEN);
    if (!files) {
        goto done;
    }

    while (files[i]) {
        if (strcmp(files[i++], file) == 0) {
            ret = 1;
            break;
        }
    }

done:
    if (files) {
        g_strfreev(files);
    }

    if (*error) {
        lmi_warn(error);
    }

    return ret;
}

short is_file_part_of_elem_name(const char *file, const char *elem_name)
{
    short ret = 0;
    SwPackage sw_pkg;
    PkPackage *pk_pkg = NULL;

    init_sw_package(&sw_pkg);

    if (create_sw_package_from_elem_name(elem_name, &sw_pkg) != 0) {
        lmi_warn("Failed to create package from element name: %s", elem_name);
        goto done;
    }

    get_pk_pkg_from_sw_pkg(&sw_pkg, pk_bitfield_value(PK_FILTER_ENUM_INSTALLED),
            &pk_pkg);
    if (!pk_pkg) {
        lmi_warn("Failed to get PkPackage, probably because it's not installed");
        goto done;
    }

    ret = is_file_part_of_pk_pkg(file, pk_pkg);

done:
    clean_sw_package(&sw_pkg);

    if (pk_pkg) {
        g_object_unref(pk_pkg);
    }

    return ret;
}

gint make_package_id_with_custom_data(const PkPackage *pk_pkg,
                                      const gchar *data,
                                      gchar *buf,
                                      guint buflen)
{
    return g_snprintf(buf, buflen, "%s;%s;%s;%s",
            pk_package_get_name((PkPackage *) pk_pkg),
            pk_package_get_version((PkPackage *) pk_pkg),
            pk_package_get_arch((PkPackage *) pk_pkg),
            data);
}

const gchar *pk_pkg_id_get_repo_id(const gchar *pkg_id)
{
    const gchar *tmp;
    const gchar *repo_id = rindex(pkg_id, ';');

    if (repo_id == NULL) {
        lmi_error("Got invalid package id: \"%s\"!", pkg_id);
        return NULL;
    }

    tmp = index(repo_id + 1, ':');
    if (tmp == NULL) {
        repo_id += 1;
    } else {
        repo_id = tmp + 1;
    }
    return repo_id;
}

const gchar *pk_pkg_get_repo_id(const PkPackage *pk_pkg)
{
    const gchar *tmp;
    const gchar *repo_id = pk_package_get_data((PkPackage *) pk_pkg);

    tmp = index(repo_id, ':');
    /* skip the installed prefix */
    if (tmp != NULL)
        repo_id = tmp + 1;
    return repo_id;
}

gboolean pk_pkg_belongs_to_repo(const PkPackage *pk_pkg, const gchar *repo_id)
{
    const gchar *pkg_id = pk_package_get_id((PkPackage *) pk_pkg);
    return g_strcmp0(pk_pkg_id_get_repo_id(pkg_id), repo_id) == 0;
}

gboolean pk_pkg_is_installed(const PkPackage *pk_pkg)
{
    const gchar *data = pk_package_get_data((PkPackage *) pk_pkg);
    return (strncmp(data, PKG_ID_DATA_INSTALLED_PREFIX,
            PKG_ID_DATA_INSTALLED_PREFIX_LEN) == 0 &&
        (data[PKG_ID_DATA_INSTALLED_PREFIX_LEN] == ':' ||
         data[PKG_ID_DATA_INSTALLED_PREFIX_LEN] == '\0'));
}

gboolean pk_pkg_is_available(const PkPackage *pk_pkg)
{
    const gchar *repo_id = pk_pkg_get_repo_id(pk_pkg);
    if (repo_id == NULL)
        return FALSE;
    return g_strcmp0(repo_id, PKG_ID_DATA_INSTALLED_PREFIX) != 0 && \
            g_strcmp0(repo_id, "local") != 0;
}

/*******************************************************************************
 * Functions related to multiple PkPackages
 ******************************************************************************/

void get_pk_packages(PkBitfield filters, GPtrArray **garray, char *error_msg,
        const unsigned error_msg_len)
{
    PkTask *task = NULL;
    PkResults *results = NULL;
    GError *gerror = NULL;
    GPtrArray *array = NULL;

    task = pk_task_new();

    results = pk_task_get_packages_sync(task, filters, NULL, NULL, NULL, &gerror);
    if (check_and_create_error_msg(results, gerror,
            "Getting list of packages failed", error_msg, error_msg_len)) {
        goto done;
    }

    array = pk_results_get_package_array(results);
    g_ptr_array_sort(array, (GCompareFunc) pk_pkg_cmp);
    *garray = g_ptr_array_ref(array);

done:
    g_clear_error(&gerror);

    if (array) {
        g_ptr_array_unref(array);
    }
    if (results) {
        g_object_unref(results);
    }
    if (task) {
        g_object_unref(task);
    }

    return;
}

void get_pk_det_from_array(char **values_p, GPtrArray **garray,
        PkTask *task_p)
{
    PkTask *task = NULL;
    PkResults *results = NULL;
    GPtrArray *array = NULL, *array2 = NULL;
    GError *gerror = NULL;
    gchar **values = NULL;
    unsigned i, j, values_p_count = 0;
    char error_msg[BUFLEN] = "";

    if (!values_p || !*values_p) {
        lmi_warn("No package IDs given.");
        goto done;
    }

    while (values_p[++values_p_count]);

    if (task_p) {
        task = task_p;
    } else {
        task = pk_task_new();
    }

    if (values_p_count < PK_DETAILS_LIMIT) {
        values = g_new0(gchar*, values_p_count + 1);
    } else {
        values = g_new0(gchar*, PK_DETAILS_LIMIT + 1);
    }

    for (i = 0; i < values_p_count / PK_DETAILS_LIMIT + 1; i++) {
        j = 0;
        while (values[j]) {
            values[j++] = NULL;
        }

        for (j = i * PK_DETAILS_LIMIT;
                j < (i + 1) * PK_DETAILS_LIMIT && j < values_p_count; j++) {
            values[j - i * PK_DETAILS_LIMIT] = values_p[j];
        }

        results = pk_task_get_details_sync(task, values, NULL, NULL, NULL, &gerror);
        if (check_and_create_error_msg(results, gerror,
                "Getting package details failed", error_msg, BUFLEN)) {
            lmi_warn(error_msg);
            goto done;
        }

        array = pk_results_get_details_array(results);
        gc_gobject_ptr_array_append(&array2, array);

        if (results) {
            g_object_unref(results);
            results = NULL;
        }
        if (array) {
            g_ptr_array_unref(array);
            array = NULL;
        }
    }

    g_ptr_array_sort(array2, (GCompareFunc) pk_det_cmp);
    *garray = g_ptr_array_ref(array2);

done:
    g_clear_error(&gerror);

    g_free(values);
    if (array) {
        g_ptr_array_unref(array);
    }
    if (array2) {
        g_ptr_array_unref(array2);
    }
    g_clear_object(&results);
    if (!task_p)
        g_clear_object(&task);

    return;
}

CMPIStatus k_return_sw_identity_op_from_pkg_id(const char *pkg_id,
        const CMPIBroker *cb, const CMPIContext *ctx, const char *ns,
        const CMPIResult* cr)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    SwPackage sw_pkg;
    char elem_name[BUFLEN] = "", instance_id[BUFLEN] = "";

    init_sw_package(&sw_pkg);

    if (create_sw_package_from_pk_pkg_id(pkg_id, &sw_pkg) != 0) {
        KSetStatus(&status, ERR_FAILED);
        goto done;
    }

    sw_pkg_get_element_name(&sw_pkg, elem_name, BUFLEN);

    create_instance_id(LMI_SoftwareIdentity_ClassName, elem_name, instance_id,
            BUFLEN);

    LMI_SoftwareIdentityRef w;
    LMI_SoftwareIdentityRef_Init(&w, cb, ns);
    LMI_SoftwareIdentityRef_Set_InstanceID(&w, instance_id);
    KReturnObjectPath(cr, w);

done:
    clean_sw_package(&sw_pkg);
    return status;
}

void enum_sw_identity_instance_names(PkBitfield filters, const CMPIBroker *cb,
        const CMPIContext *ctx, const char *ns, const CMPIResult* cr,
        char *error_msg, const unsigned error_msg_len)
{
    GPtrArray *array = NULL;
    unsigned i;

    get_pk_packages(filters, &array, error_msg, error_msg_len);
    if (!array) {
        goto done;
    }

    for (i = 0; i < array->len; i++) {
        k_return_sw_identity_op_from_pkg_id(
                pk_package_get_id(g_ptr_array_index(array, i)), cb, ctx, ns, cr);
    }

done:
    if (array) {
        g_ptr_array_unref(array);
    }
}

CMPIStatus enum_sw_identity_instances(PkBitfield filters, const CMPIBroker *cb,
        const char *ns, const CMPIResult* cr, char *error_msg,
        const unsigned error_msg_len)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    LMI_SoftwareIdentity w;
    PkPackage *pk_pkg = NULL;
    PkDetails *pk_det = NULL;
    GPtrArray *pkg_array = NULL, *det_array = NULL;
    gchar **values = NULL;
    gchar *pkg_id = NULL;
    SwPackage sw_pkg;
    int cmpres;
    unsigned i, det_a_i = 0;

    init_sw_package(&sw_pkg);

    get_pk_packages(filters, &pkg_array, error_msg, error_msg_len);
    if (!pkg_array) {
        KSetStatus2(cb, &status, ERR_FAILED, error_msg);
        goto done;
    }

    values = g_new0(gchar*, pkg_array->len + 1);
    for (i = 0; i < pkg_array->len; i++) {
        values[i] = g_strdup(pk_package_get_id(g_ptr_array_index(pkg_array, i)));
    }
    get_pk_det_from_array(values, &det_array, NULL);
    g_strfreev(values);
    values = NULL;

    for (i = 0; i < pkg_array->len; i++) {
        pk_pkg = g_ptr_array_index(pkg_array, i);
        if (create_sw_package_from_pk_pkg(pk_pkg, &sw_pkg) != 0) {
            continue;
        }

        cmpres = -1;
        if (det_array) {
            while (cmpres < 0 && det_a_i < det_array->len) {
                pk_det = g_ptr_array_index(det_array, det_a_i);
                g_object_get(pk_det, "package-id", &pkg_id, NULL);
                cmpres = strcmp(pkg_id, pk_package_get_id(pk_pkg));
                g_free(pkg_id);
                pkg_id = NULL;
                if (cmpres < 0) {
                    /* this should not happen -
                     * we have spare unmatched package details */
                    det_a_i++;
                } else if (cmpres > 0) {
                    /* no matching package details for current pk_pkg */
                    pk_det = NULL;
                } else {
                    /* found a match */
                    det_a_i++;
                }
            }
        }

        create_instance_from_pkgkit_data(pk_pkg, pk_det, &sw_pkg, cb, ns, &w);
        KReturnInstance(cr, w);

        clean_sw_package(&sw_pkg);
    }

done:
    if (det_array) {
        g_ptr_array_unref(det_array);
    }
    if (pkg_array) {
        g_ptr_array_unref(pkg_array);
    }

    return status;
}

/*******************************************************************************
 * Functions related to PkRepos
 ******************************************************************************/

gboolean get_pk_repo(const char *repo_id_p, PkRepoDetail **repo_p, char *error_msg,
        const unsigned error_msg_len)
{
    guint i;
    gchar *repo_id = NULL;
    PkRepoDetail *r;
    GPtrArray *array = NULL;
    gboolean ret = FALSE;

    get_pk_repos(REPO_STATE_ENUM_ANY, &array, error_msg, error_msg_len);
    if (!array) {
        lmi_warn(error_msg);
        goto done;
    }

    for (i = 0; i < array->len; i++) {
        r = g_ptr_array_index(array, i);
        g_object_get(r, "repo-id", &repo_id, NULL);
        if (!repo_id) {
            continue;
        }
        if (strcmp(repo_id, repo_id_p) == 0) {
            if (repo_p) {
                *repo_p = g_object_ref(r);
            }
            ret = TRUE;
            break;
        }
        g_free(repo_id);
        repo_id = NULL;
    }

done:
    g_free(repo_id);

    if (array) {
        g_ptr_array_unref(array);
    }

    return ret;
}

void get_pk_repos(RepoStateEnum repo_state,
                  GPtrArray **garray,
                  char *error_msg,
                  const unsigned error_msg_len)
{
    PkTask *task = NULL;
    PkResults *results = NULL;
    GError *gerror = NULL;
    GPtrArray *array = NULL;
    g_assert(garray != NULL);

    task = pk_task_new();

    results = pk_task_get_repo_list_sync(task, 0, NULL, NULL, NULL, &gerror);
    if (check_and_create_error_msg(results, gerror,
            "Getting list of repositories failed", error_msg, error_msg_len)) {
        goto done;
    }

    array = pk_results_get_repo_detail_array(results);
    g_ptr_array_sort(array, (GCompareFunc) pk_repo_cmp);
    if (repo_state != REPO_STATE_ENUM_ANY) {
        GPtrArray *tmpa = g_ptr_array_new_with_free_func(g_object_unref);
        PkRepoDetail *pk_det;
        gboolean enabled;
        for (guint i = 0; i < array->len; ++i) {
            pk_det = g_ptr_array_index(array, i);
            g_object_get(pk_det, "enabled", &enabled, NULL);
            if ((repo_state == REPO_STATE_ENUM_ENABLED && !enabled) ||
                    (repo_state == REPO_STATE_ENUM_DISABLED && enabled))
                continue;
            g_ptr_array_add(tmpa, g_object_ref(pk_det));
        }
        g_ptr_array_unref(array);
        array = tmpa;
    }
    *garray = g_ptr_array_ref(array);

done:
    g_clear_error(&gerror);

    if (array) {
        g_ptr_array_unref(array);
    }
    g_clear_object(&results);
    g_clear_object(&task);
}

void get_repo_id_from_sw_pkg(const SwPackage *sw_pkg, gchar **repo_id_p,
        char *error_msg, const unsigned error_msg_len)
{
    PkPackage *pk_pkg = NULL;
    GPtrArray *repos = NULL, *det_array = NULL;
    gchar *repo_id = NULL, *pkg_id = NULL, **values = NULL, **pkg_id_parts = NULL;
    const gchar *repo_id_pkg = NULL;
    const gchar *tmp = NULL;
    unsigned i, j;

    get_pk_repos(REPO_STATE_ENUM_ENABLED, &repos, error_msg, error_msg_len);
    if (!repos) {
        goto done;
    }

    get_pk_pkg_from_sw_pkg(sw_pkg, 0, &pk_pkg);
    if (!pk_pkg) {
        goto done;
    }

    repo_id_pkg = pk_pkg_get_repo_id(pk_pkg);
    if (!repo_id_pkg) {
        lmi_warn("Invalid PackageKit package.");
        goto done;
    }

    for (i = 0; i < repos->len; i++) {
        g_object_get(g_ptr_array_index(repos, i), "repo-id", &repo_id, NULL);
        if (strcmp(repo_id, repo_id_pkg) == 0) {
            *repo_id_p = repo_id;
            goto done;
        }
        g_free(repo_id);
        repo_id = NULL;
    }

    values = g_new0(gchar*, repos->len + 1);
    /* We didn't find the match - package is probably installed. */
    if (pk_pkg_is_installed(pk_pkg) && !pk_pkg_is_available(pk_pkg)) {
        /* The repo_id isn't present in data field - let's try to guess it. */
        j = 0;
        for (i = 0; i < repos->len; i++) {
            g_object_get(g_ptr_array_index(repos, i),
                    "repo-id", &repo_id,
                    NULL);
            pkg_id = pk_package_id_build(pk_package_get_name(pk_pkg),
                    pk_package_get_version(pk_pkg), pk_package_get_arch(pk_pkg),
                    repo_id);
            values[j++] = pkg_id;
            g_clear_pointer(&repo_id, g_free);
        }
    }

    if (values[0] == NULL)
        goto done;
    get_pk_det_from_array(values, &det_array, NULL);

    if (det_array && det_array->len > 0) {
        for (i = 0; i < det_array->len; i++) {
            g_object_get(g_ptr_array_index(det_array, i), "package-id",
                    &pkg_id, NULL);
            pkg_id_parts = pk_package_id_split(pkg_id);
            g_free(pkg_id);
            pkg_id = NULL;
            if (!pkg_id_parts) {
                continue;
            }
            tmp = index(pkg_id_parts[PK_PACKAGE_ID_DATA], ':');
            if (tmp != NULL) {
                *repo_id_p = g_strdup(tmp + 1);
            } else {
                *repo_id_p = g_strdup(pkg_id_parts[PK_PACKAGE_ID_DATA]);
            }
            if (repo_id_p == NULL) {
                lmi_warn("Memory allocation failed.");
            }
            goto done;
        }
    }

done:
    g_strfreev(values);
    g_strfreev(pkg_id_parts);

    if (pk_pkg) {
        g_object_unref(pk_pkg);
    }
    if (repos) {
        g_ptr_array_unref(repos);
    }
    if (det_array) {
        g_ptr_array_unref(det_array);
    }

    return;
}

/**
 * Append `PkRepoDetail` pointer to an array of all repositories providing
 * given package.
 *
 * @param tree Contains nodes of `(PkPackage *, `GPtrArray` of `PkRepoDetail`s.
 */
static gboolean associate_repo_det_to_available_package(
    GTree *tree,
    PkPackage *pk_pkg,
    PkRepoDetail *pk_repo_det)
{
    PkPackage *orig;
    GPtrArray *array = NULL;

    if (g_tree_lookup_extended(tree, pk_pkg,
                (gpointer *) &orig, (gpointer *) &array))
    {
        for (guint i = 0; i < array->len; ++i) {
            if (g_ptr_array_index(array, i) == pk_repo_det)
                return FALSE;
        }
    } else {
        array = g_ptr_array_new_full(1, g_object_unref);
        g_tree_insert(tree, g_object_ref(pk_pkg), array);
    }
    g_ptr_array_add(array, g_object_ref(pk_repo_det));
    return TRUE;
}

void get_repo_dets_for_pk_pkgs(GPtrArray *pk_pkgs,
                               GPtrArray *pk_repo_dets,
                               GTree **avail_pkg_repos,
                               gchar *error_msg,
                               const unsigned error_msg_len)
{
    GTree *tree = NULL;
    PkPackage *pk_pkg = NULL;
    PkRepoDetail *pk_repo = NULL;
    gboolean pk_repo_dets_original = TRUE;
    GPtrArray *det_array = NULL;
    /* maps pkg_id -> PkPackage */
    GHashTable *pkg_dict = NULL;
    /* maps repo_id -> PkRepoDetail */
    GHashTable *repo_dict = NULL;
    GHashTableIter iter;
    gchar *repo_id = NULL, *pkg_id = NULL, **values = NULL;
    gchar buf[BUFLEN];
    g_assert(avail_pkg_repos);

    if (pk_repo_dets == NULL) {
        /* get just enabled repositories if caller did not supply any */
        get_pk_repos(REPO_STATE_ENUM_ENABLED, &pk_repo_dets,
                error_msg, error_msg_len);
        if (!pk_repo_dets)
            goto err;
        pk_repo_dets_original = FALSE;
    }

    /* filter out disabled repositories and create a dictionary */
    repo_dict = g_hash_table_new_full(g_str_hash, lmi_str_equal,
            g_free, g_object_unref);
    if (repo_dict == NULL)
        goto mem_err;
    for (guint ri=0; ri < pk_repo_dets->len; ++ri) {
        gboolean enabled;
        pk_repo = g_ptr_array_index(pk_repo_dets, ri);
        g_object_get(pk_repo,
                "repo-id", &repo_id,
                "enabled", &enabled,
                NULL);
        if (enabled) {
            g_hash_table_insert(repo_dict, repo_id,
                    g_object_ref(pk_repo));
        } else {
            g_free(repo_id);
        }
    }
    if (!pk_repo_dets_original)
        g_clear_pointer(&pk_repo_dets, g_ptr_array_unref);

    /* create resulting tree */
    tree = g_tree_new_full(lmi_tree_ptrcmp_func, NULL,
            g_object_unref, (GDestroyNotify) g_ptr_array_unref);
    if (tree == NULL)
        goto mem_err;

    /* Associate PKRepoDetail with PkPackage for uninstalled packages.
     * Save installed packages to `pkg_dict` for later processing. */
    pkg_dict = g_hash_table_new_full(g_str_hash,
            lmi_str_equal, g_free, g_object_unref);
    if (pkg_dict == NULL)
        goto mem_err;
    for (guint pi=0; pi < pk_pkgs->len; ++pi) {
        pk_pkg = g_ptr_array_index(pk_pkgs, pi);
        /* won't get modified */
        repo_id = (gchar *) pk_pkg_get_repo_id(pk_pkg);
        pk_repo = g_hash_table_lookup(repo_dict, repo_id);
        if (pk_repo == NULL) {
            if (g_strcmp0(repo_id, PKG_ID_DATA_INSTALLED_PREFIX) == 0) {
                g_hash_table_iter_init(&iter, repo_dict);
                /* create all possible combinations of
                 * `pkg.name;pkg.verison;pkg.arch;repo.repo_id` for currently
                 * processed package and `repo` from `repo_dict`
                 * and store them for later processing */
                while (g_hash_table_iter_next(&iter,
                            (gpointer *) &repo_id, NULL))
                {
                    make_package_id_with_custom_data(pk_pkg, repo_id,
                            buf, BUFLEN);
                    g_hash_table_insert(pkg_dict, g_strdup(buf),
                            g_object_ref(pk_pkg));
                }
            } else {
                lmi_warn("Data \"%s\" of package \"%s\" does not match any"
                       " enabled repository.", repo_id, pk_package_get_id(pk_pkg));
            }
        } else {
            associate_repo_det_to_available_package(tree, pk_pkg, pk_repo);
        }
    }

    /* Process installed packages. Get all repositories where they are
     * available for each of them. */
    if (g_hash_table_size(pkg_dict) > 0) {
        guint i = 0;
        values = g_new(gchar *, g_hash_table_size(pkg_dict) + 1);
        if (values == NULL)
            goto mem_err;
        g_hash_table_iter_init(&iter, pkg_dict);
        while (g_hash_table_iter_next(&iter, (gpointer *) &pkg_id, NULL)) {
            values[i++] = pkg_id;
        }
        values[i] = NULL;
        get_pk_det_from_array(values, &det_array, NULL);
        g_clear_pointer(&values, g_free);
        if (det_array && det_array->len > 0) {
            for (guint ri=0; ri < det_array->len; ++ri) {
                PkDetails *pk_det = g_ptr_array_index(det_array, ri);
                g_object_get(pk_det, "package-id", &pkg_id, NULL);
                if (!g_hash_table_lookup_extended(pkg_dict, pkg_id,
                        NULL, (gpointer *) &pk_pkg))
                {
                    lmi_warn("Got unexpected package details for \"%s\"!", pkg_id);
                    g_free(pkg_id);
                    continue;
                }
                pk_repo = g_hash_table_lookup(repo_dict,
                        pk_pkg_id_get_repo_id(pkg_id));
                g_assert(pk_repo != NULL);
                associate_repo_det_to_available_package(tree, pk_pkg, pk_repo);
                g_free(pkg_id);
            }
        }
        g_clear_pointer(&det_array, g_ptr_array_unref);
    }
    g_hash_table_unref(pkg_dict);
    g_hash_table_unref(repo_dict);

    *avail_pkg_repos = tree;
    return;

mem_err:
    lmi_error("Memory allocation failed");
err:
    g_clear_pointer(&tree, g_tree_unref);
    g_clear_pointer(&det_array, g_ptr_array_unref);
    g_clear_pointer(&repo_dict, g_hash_table_unref);
    g_clear_pointer(&values, g_free);
    if (!pk_repo_dets_original)
        g_ptr_array_unref(pk_repo_dets);
    return;

}

/*******************************************************************************
 * Functions related to PackageKit
 ******************************************************************************/

short check_and_create_error_msg(PkResults *results, GError *gerror,
        const char *custom_msg, char *error_msg, const unsigned error_msg_len)
{
    short ret = 0;
    PkError *error_code = NULL;

    if (results) {
        error_code = pk_results_get_error_code(results);
        if (error_code) {
            snprintf(error_msg, error_msg_len,
                    "%s: %s, %s", custom_msg,
                    pk_error_enum_to_string(pk_error_get_code(error_code)),
                    pk_error_get_details(error_code));
            g_object_unref(error_code);
            ret = 1;
            goto done;
        }
    }

    if (gerror) {
        snprintf(error_msg, error_msg_len, "%s: %s", custom_msg,
                gerror->message);
        ret = 1;
        goto done;
    }

    if (!results) {
        snprintf(error_msg, error_msg_len, "%s: Nothing returned", custom_msg);
        ret = 1;
        goto done;
    }

done:
    return ret;
}

gint pk_pkg_cmp(gpointer a, gpointer b)
{
    PkPackage *al = *(PkPackage **) a;
    PkPackage *bl = *(PkPackage **) b;

    return (gint) strcmp(pk_package_get_id(al), pk_package_get_id(bl));
}

gint pk_pkg_id_cmp(gpointer a, gpointer b)
{
    char *al = (char *) a;
    char *bl = (char *) b;

    guint n = 0;
    char *last_sep;

    /* omit data part from comparison (everything after the last ';') */
    last_sep = rindex(al, ';');
    if (last_sep) {
        n = last_sep - al + 1;
    }

    last_sep = rindex(bl, ';');
    if (last_sep) {
        n = MAX(n, last_sep - bl + 1);
    }

    if (n) {
        return (gint) strncmp(al, bl, n);
    } else {
        return (gint) strcmp(al, bl);
    }
}

gint pk_det_cmp(gpointer a, gpointer b)
{
    PkDetails *al = *(PkDetails **) a;
    PkDetails *bl = *(PkDetails **) b;
    gchar *pkg_id_a = NULL;
    gchar *pkg_id_b = NULL;
    gint ret;

    g_object_get(al, "package-id", &pkg_id_a, NULL);
    g_object_get(bl, "package-id", &pkg_id_b, NULL);

    ret = (gint) strcmp(pkg_id_a, pkg_id_b);

    g_free(pkg_id_a);
    g_free(pkg_id_b);

    return ret;
}

gint pk_repo_cmp(gpointer a, gpointer b)
{
    gint res;
    PkRepoDetail *al = *(PkRepoDetail **) a;
    PkRepoDetail *bl = *(PkRepoDetail **) b;
    gchar *repo_id_a = NULL, *repo_id_b = NULL;

    g_object_get(al, "repo-id", &repo_id_a, NULL);
    g_object_get(bl, "repo-id", &repo_id_b, NULL);

    res = (gint) strcmp(repo_id_a, repo_id_b);

    g_free(repo_id_a);
    g_free(repo_id_b);

    return res;
}

const gchar *get_distro_id()
{
    if (_distro_id == NULL)
        _distro_id = pk_get_distro_id();
    return _distro_id;
}

const gchar *get_distro_arch()
{
    const gchar *_distro_id = get_distro_id();
    return rindex(_distro_id, ';') + 1;
}


/*******************************************************************************
 * Functions related to Glib
 ******************************************************************************/

void gc_gobject_ptr_array_append(GPtrArray **a, const GPtrArray *b)
{
    guint i;

    if (!a || !b) {
        lmi_warn("Received empty parameter.");
        return;
    }

    if (!*a) {
        *a = g_ptr_array_new_full(b->len, g_object_unref);
    }

    for (i = 0; i < b->len; i++) {
        g_ptr_array_add(*a, g_object_ref(g_ptr_array_index(b, i)));
    }
}

/*******************************************************************************
 * Functions related to CMPI
 ******************************************************************************/

const char *get_elem_name_from_instance_id(const char *instance_id)
{
    if (!instance_id || !*instance_id) {
        lmi_warn("Empty InstanceID.");
        return "";
    }

    if (strlen(instance_id) <= SW_IDENTITY_INSTANCE_ID_PREFIX_LEN) {
        lmi_warn("Invalid InstanceID: %s", instance_id);
        return "";
    }

    if (strncasecmp(instance_id, SW_IDENTITY_INSTANCE_ID_PREFIX,
            SW_IDENTITY_INSTANCE_ID_PREFIX_LEN) != 0) {
        lmi_warn("Invalid InstanceID: %s", instance_id);
        return "";
    }

    return instance_id + SW_IDENTITY_INSTANCE_ID_PREFIX_LEN;
}

void create_instance_id(const char *class_name, const char *id,
        char *instance_id, const unsigned instance_id_len)
{
    if (id) {
        snprintf(instance_id, instance_id_len, LMI_ORGID ":%s:%s", class_name, id);
    } else {
        snprintf(instance_id, instance_id_len, LMI_ORGID ":%s", class_name);
    }
}

/*******************************************************************************
 * Object path checks
 ******************************************************************************/

bool check_system_software_collection(const CMPIBroker *cb,
                                      const CMPIObjectPath *path)
{
    CMPIString *str_namespace;
    CMPIData data;
    CMPIStatus status;
    char *our_namespace;
    char instance_id[BUFLEN] = "";

    if ((our_namespace = lmi_read_config("CIM", "Namespace")) == NULL)
        goto err_l;
    if ((str_namespace = CMGetNameSpace(path, NULL)) == NULL)
        goto namespace_err_l;
    if (strcmp(KChars(str_namespace), our_namespace))
        goto namespace_err_l;
    g_free(our_namespace);

    if (!CMClassPathIsA(cb, path, LMI_SystemSoftwareCollection_ClassName, NULL))
        goto err_l;

    create_instance_id(LMI_SystemSoftwareCollection_ClassName, NULL, instance_id,
                    BUFLEN);
    data = CMGetKey(path, "InstanceID", &status);
    if (status.rc || data.type != CMPI_string ||
            data.state != (CMPI_goodValue | CMPI_keyValue))
        goto err_l;
    if (strcmp(KChars(data.value.string), instance_id))
        goto err_l;

    return true;

namespace_err_l:
    g_free(our_namespace);
err_l:
    return false;
}

CMPIStatus check_software_identity_resource(const CMPIBroker *cb,
                                            const CMPIContext *ctx,
                                            const CMPIObjectPath *path,
                                            PkRepoDetail **pk_repo)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIString *str_namespace;
    const gchar *repo_id;
    PkRepoDetail *repo = NULL;
    gchar *our_namespace = NULL;
    gchar errbuf[BUFLEN];

    if ((our_namespace = lmi_read_config("CIM", "Namespace")) == NULL)
        goto err;
    if ((str_namespace = CMGetNameSpace(path, NULL)) == NULL)
        goto err;
    if (strcmp(KChars(str_namespace), our_namespace))
        goto err;
    g_clear_pointer(&our_namespace, g_free);

    if (!CMClassPathIsA(cb, path, LMI_SoftwareIdentityResource_ClassName, NULL))
        goto err;

    if (strcmp(lmi_get_string_property_from_objectpath(
                    path, "CreationClassName"),
                LMI_SoftwareIdentityResource_ClassName) != 0)
    {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "Invalid CreationClassName key property.");
        goto err;
    }
    if (strcmp(lmi_get_string_property_from_objectpath(
                    path, "SystemCreationClassName"),
                lmi_get_system_creation_class_name()) != 0)
    {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "Invalid SystemCreationClassName key property.");
        goto err;
    }
    if (strcmp(lmi_get_string_property_from_objectpath(path, "SystemName"),
                lmi_get_system_name_safe(ctx)) != 0)
    {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "Invalid SystemName key property.");
        goto err;
    }

    repo_id = lmi_get_string_property_from_objectpath(path, "Name");
    get_pk_repo(repo_id, &repo, errbuf, BUFLEN);
    if (!repo) {
        lmi_error(errbuf);
        snprintf(errbuf, BUFLEN, "Failed to find repository \"%s\"!", repo_id);
        lmi_error(errbuf);
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_NOT_FOUND, errbuf);
        goto err;
    }

    if (pk_repo) {
        *pk_repo = repo;
    } else {
        g_object_unref(repo);
    }

    return status;

err:
    g_clear_object(&repo);
    g_free(our_namespace);
    return status;
}

/*******************************************************************************
 * General functions
 ******************************************************************************/

short popcount(unsigned x)
{
    short count;

    if (!x) {
        return 0;
    }

    for (count = 0; x; count++) {
        x &= x - 1;
    }

    return count;
}
