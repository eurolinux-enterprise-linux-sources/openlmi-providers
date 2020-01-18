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
 * Authors: Michal Minar <miminar@redhat.com>
 */

#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "openlmi.h"
#include "openlmi-utils.h"
#include "ind_sender.h"
#include "job_manager.h"

#define JOB_MANAGER_THREAD_NAME "JobManager"
#define MINIMUM_TIME_BEFORE_REMOVAL (5*60)
#define DEFAULT_TIME_BEFORE_REMOVAL MINIMUM_TIME_BEFORE_REMOVAL
#define DEFAULT_CIM_CLASS_NAME "LMI_ConcreteJob"
#define JOB_FILE_EXTENSION ".json"
#define JOB_FILE_NAME_REGEX ("^(.*)-(\\d+)\\" JOB_FILE_EXTENSION "$")
#define CIM_INST_METHOD_CALL_CLASS_NAME "CIM_InstMethodCall"
#define CIM_ERROR_CLASS_NAME "CIM_Error"

#define CIM_CONCRETE_JOB_COMMUNICATION_STATUS_NOT_AVAILABLE 0
#define CIM_CONCRETE_JOB_LOCAL_OR_UTC_TIME_UTC_TIME         2
#define CIM_ERROR_SOURCE_FORMAT_OBJECT_PATH                 2
#define INST_METHOD_CALL_RETURN_VALUE_TYPE_0                2

typedef struct _JobTypeInfo {
    const gchar *cim_class_name;
    JobToCimInstanceCallback convert_func;
    MakeJobParametersCallback make_job_params_func;
    MethodResultValueTypeEnum return_value_type;
    JobProcessCallback process_func;
    gboolean running_job_cancellable;
    gboolean use_persistent_storage;
}JobTypeInfo;

typedef struct _PendingJobKey {
    guint priority;
    guint number;
}PendingJobKey;

typedef enum {
    JOB_ACTION_REMOVE,
    JOB_ACTION_LAST
}JobActionEnum;

typedef struct _CalendarEvent {
    /* time of event's execution in milliseconds since epoch */
    guint64 time;
    LmiJob *job;
    JobActionEnum action;
}CalendarEvent;

typedef struct _JobModifiedParams {
    LmiJob *job;
    LmiJobPropEnum property;
    GVariant *old_value;
}JobModifiedParams;

#define JM_CRITICAL_BEGIN \
    { \
        pthread_t _thread_id = pthread_self(); \
        _clog(CLOG_COLOR_MAGENTA, "[tid=%lu] locking", _thread_id); \
        pthread_mutex_lock(&_lock); \
        _clog(CLOG_COLOR_MAGENTA, "[tid=%lu] locked", _thread_id); \
    }

#define JM_CRITICAL_END \
    { \
        pthread_t _thread_id = pthread_self(); \
        _clog(CLOG_COLOR_MAGENTA, "[tid=%lu] unlocking", _thread_id); \
        pthread_mutex_unlock(&_lock); \
        _clog(CLOG_COLOR_MAGENTA, "[tid=%lu] unlocked", _thread_id); \
    }

/**
 * Guards `_initialized_counter` in jobmgr_init() and jobmgr_cleanup() functions.
 */
static pthread_mutex_t _init_lock = PTHREAD_MUTEX_INITIALIZER;
/**
 * Guards private data structures of Job Manager.
 * It is a recursive lock.
 */
static pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;
/* The first broker object passed to `jobmgr_init()`. It's used in most
 * operations manipulating with CMPI data abstractions. There is one exception
 * though. Indication sender needs to be given a broker object passed to
 * particular CIM indication provider instrumenting class whose instance is
 * about to be sent. See `_indication_brokers` for details.
 */
static const CMPIBroker *_cb = NULL;
static const CMPIContext *_cmpi_ctx = NULL;
static gchar *_profile_name = NULL;
static guint _initialized_counter = 0;
static gboolean _concurrent_processing = FALSE;
/* Maps GType of job to JobTypeInfo. This does not change after manager's init.
 */
static GTree *_job_type_map = NULL;
/* Maps provider names instrumenting CIM indication classes to corresponding
 * broker objects. Indication sender needs to be passed correct broker object
 * that was passed to particular indication class provider.
 *
 * No indication instance can be sent until particular indication providers are
 * initialized (they called `jobmgr_init()` with is_indication_provider being
 * `TRUE`).
 */
static GTree *_indication_brokers = NULL;
/* Maps job number to job instance. It contains all known jobs accessible with
 * GetInstance() call. No other job container of this job manager manipulates
 * reference count of jobs. Once the job is created, it is placed here and its
 * reference count is set to 1. Once it's deleted it's removed from here and
 * its reference count decreased.
 */
static GTree *_job_map = NULL;
/* Priority queue of pending jobs. Key is a pair (job's priority, job's number)
 * represented by `PendingJobKey`. Value is corresponding job. Job can not be
 * key because its priority may change anytime, therefore its lookup wouldn't
 * work.
 */
static GTree *_job_queue = NULL;
/* Set of running jobs. The upper limit of jobs is determined by
 * *concurrent_processing* flag. If `true`, there is no limit. Otherwise at
 * most 1 job can be present. Ponter to corresponding GTask is an associated
 * value.
 */
static GTree *_running_jobs = NULL;
/* Calendar for inner events. Event is some action scheduled for execution on
 * particular job. Tree contains just keys. Key is of type CalendarEvent.
 * If some action on particular job, which is already present, is being scheduled,
 * only the one scheduled for earlier time will be kept.
 */
static GTree *_event_calendar = NULL;
/**
 * Thread object executing main loop. Its behaviour is defined in *run()*
 * function.
 */
static GThread *_manager_thread = NULL;
/**
 * Context belonging to the main loop.
 */
static GMainContext *_main_ctx = NULL;
/**
 * Main loop object executed by *_manager_thread*. It handles calendar events
 * and all job modified and finished signals.
 */
static GMainLoop *_main_loop = NULL;

static const gchar *_action_names[] = {
    "remove"
};

static const gchar *_filter_queries[] = {
    /* STATIC_FILTER_ID_CHANGED */
    "SELECT * FROM LMI_%sInstModification WHERE "
        "SourceInstance ISA %s AND "
        "SourceInstance.CIM_ConcreteJob::JobState <> "
        "PreviousInstance.CIM_ConcreteJob::JobState",
    /* STATIC_FILTER_ID_CREATED */
    "SELECT * FROM LMI_%sInstCreation WHERE "
            "SourceInstance ISA %s",
    /* STATIC_FILTER_ID_DELETED */
    "SELECT * FROM LMI_%sInstDeletion WHERE "
            "SourceInstance ISA %s",
    /* STATIC_FILTER_ID_FAILED */
    "SELECT * FROM LMI_%sInstModification WHERE "
        "SourceInstance ISA %s AND "
        "SourceInstance.CIM_ConcreteJob::JobState = 10",
    /* STATIC_FILTER_ID_PERCENT_UPDATED */
    "SELECT * FROM LMI_%sInstModification WHERE "
        "SourceInstance ISA %s AND "
        "SourceInstance.CIM_ConcreteJob::PercentComplete <> "
        "PreviousInstance.CIM_ConcreteJob::PercentComplete",
    /* STATIC_FILTER_ID_SUCCEEDED */
    "SELECT * FROM LMI_%sInstModification WHERE "
        "SourceInstance ISA %s AND "
        "SourceInstance.CIM_ConcreteJob::JobState = 7",
};

/* Forward declarations */
static gint cmp_calendar_events(gconstpointer a,
                                gconstpointer b,
                                gpointer unused);
static gint cmp_job_file_names(gconstpointer a,
                               gconstpointer b,
                               gpointer counter);
static gint cmp_lmi_job_queue_keys(gconstpointer a,
                                   gconstpointer b,
                                   gpointer unused);
static gint cmp_uints(gconstpointer a,
                      gconstpointer b,
                      gpointer unused);
static CMPIStatus delete_job(LmiJob *job);
static void job_deletion_request_changed_callback(LmiJob *job,
                                                  gboolean delete_on_completion,
                                                  gint64 time_before_removal,
                                                  gpointer unused);
static gboolean dup_job_id(gpointer key,
                           gpointer value,
                           gpointer data);
static gboolean dup_running_job_id(gpointer key,
                                   gpointer value,
                                   gpointer data);
static void find_and_delete_calendar_event(gpointer item,
                                           gpointer unused);
static gboolean find_event_by_job(gpointer key,
                                  gpointer value,
                                  gpointer data);
static void free_calendar_event(gpointer ptr);
static const gchar *get_persistent_storage_path(void);
static void job_finished_callback(LmiJob *job,
                                  LmiJobStateEnum old_state,
                                  LmiJobStateEnum new_state,
                                  GVariant *result,
                                  const gchar *error,
                                  gpointer unused);
static void job_modified_callback(LmiJob *job,
                                  LmiJobPropEnum property,
                                  GVariant *old_value,
                                  GVariant *new_value,
                                  gpointer unused);
static void job_modified_params_free(JobModifiedParams *params);
static void job_process_start(GTask *task,
                              gpointer source_object,
                              gpointer task_data,
                              GCancellable *cancellable);
static void job_processed_callback(GObject *object,
                                   GAsyncResult *result,
                                   gpointer user_data);
static CMPIUint16 job_state_to_cim(LmiJobStateEnum state);
static CMPIArray *job_state_to_cim_operational_status(LmiJobStateEnum state);
static const gchar *job_state_to_status(LmiJobStateEnum state);
static CMPIStatus job_to_cim_instance(const LmiJob *job,
                                      CMPIInstance **instance);
static gboolean launch_jobs(gpointer unused);
static LmiJob *load_job_from_file(const gchar *job_file_path);
static gboolean load_job_from_file_cb(gpointer key,
                                      gpointer value,
                                      gpointer data);
static void load_jobs_from_persistent_storage(void);
static const JobTypeInfo *lookup_job_type_info(GType job_type);
static const JobTypeInfo *lookup_job_type_info_for_job(const LmiJob *job);
static gchar *make_job_file_path(const LmiJob *job);
static CMPIStatus make_method_parameters_cim_instance_for_job(
    const LmiJob *job,
    const gchar *cim_class_name,
    CMPIInstance **instance);
static gsize make_method_parameters_class_name(gchar *buf,
                                               gsize buflen,
                                               const gchar *method_name,
                                               gboolean output);
static gboolean process_calendar_event(gpointer unused);
static gboolean process_job_finished_callback(gpointer user_data);
static gboolean process_job_modified_callback(gpointer user_data);
static gboolean register_job_classes_with_ind_sender(gpointer key,
                                                     gpointer value,
                                                     gpointer data);
static gboolean register_job(LmiJob *job, gboolean is_new);
static CMPIStatus register_job_type(GType job_type,
                                    const gchar *cim_class_name,
                                    JobToCimInstanceCallback convert_func,
                                    MakeJobParametersCallback make_job_params_func,
                                    MethodResultValueTypeEnum return_value_type,
                                    JobProcessCallback process_func,
                                    gboolean running_job_cancellable,
                                    gboolean use_persistent_storage);
static CMPIStatus register_static_filters(const gchar *cim_class_name);
static gboolean reschedule_present_event(gpointer key,
                                         gpointer value,
                                         gpointer data);
static gpointer run(gpointer data);
static void save_job(const LmiJob *job, const JobTypeInfo *info);
static void schedule_event(guint64 timeout,
                           LmiJob *job,
                           JobActionEnum action);
static CMPIStatus send_indications(const gchar *jobid,
                                   CMPIInstance *source_instance,
                                   CMPIInstance *previous_instance,
                                   IndSenderStaticFilterEnum *filter_ids,
                                   guint filter_ids_count);
static gboolean stop_search_for_job_by_id(gpointer key,
                                          gpointer value,
                                          gpointer data);
static gboolean stop_search_for_job_by_name(gpointer key,
                                            gpointer value,
                                            gpointer data);
static gboolean stop_search_for_job_by_number(gpointer key,
                                              gpointer value,
                                              gpointer data);

static gint cmp_uints(gconstpointer a,
                      gconstpointer b,
                      gpointer unused)
{
    guint va = (guint) GPOINTER_TO_UINT(a);
    guint vb = (guint) GPOINTER_TO_UINT(b);
    if (va < vb)
        return -1;
    if (va > vb)
        return 1;
    return 0;
}

static gint cmp_job_file_names(gconstpointer a,
                               gconstpointer b,
                               gpointer counter)
{
    gchar const *na = a;
    gchar const *nb = b;
    gchar const *fst_num_start;
    gchar const *snd_num_start;
    guint64 fst_id;
    guint64 snd_id;

    fst_num_start = rindex(na, '-');
    g_assert(fst_num_start != NULL);
    snd_num_start = rindex(nb, '-');
    g_assert(snd_num_start != NULL);
    ++fst_num_start;
    ++snd_num_start;
    fst_id = g_ascii_strtoull(fst_num_start, NULL, 10);
    snd_id = g_ascii_strtoull(snd_num_start, NULL, 10);
    if (fst_id < snd_id)
        return -1;
    if (fst_id > snd_id)
        return 1;
    return 0;
}

static gint cmp_lmi_job_queue_keys(gconstpointer a,
                                   gconstpointer b,
                                   gpointer unused)
{
    const PendingJobKey *fst = a;
    const PendingJobKey *snd = b;
    if (fst->priority < snd->priority)
        return -1;
    if (fst->priority > snd->priority)
        return 1;
    if (fst->number < snd->number)
        return -1;
    if (fst->number > snd->number)
        return 1;
    return 0;
}

static gint cmp_calendar_events(gconstpointer a,
                                gconstpointer b,
                                gpointer unused)
{
    const CalendarEvent *fst = a;
    const CalendarEvent *snd = b;
    if (fst->time < snd->time)
        return -1;
    if (fst->time > snd->time)
        return 1;
    guint na = lmi_job_get_number(fst->job);
    guint nb = lmi_job_get_number(snd->job);
    if (na < nb)
        return -1;
    if (na > nb)
        return 1;
    return 0;
}

static void free_calendar_event(gpointer ptr)
{
    CalendarEvent *event = (CalendarEvent *) ptr;
    g_assert(event);
    g_free(event);
}

static const JobTypeInfo *lookup_job_type_info(GType job_type)
{
    const JobTypeInfo *info = NULL;
    g_assert(_job_type_map);
    g_assert(g_type_is_a(job_type, LMI_TYPE_JOB));

    if (!g_tree_lookup_extended(_job_type_map,
               (void *) ((uintptr_t) job_type), NULL, (gpointer *) &info))
    {
        lmi_error("Job type \"%s\" is not registered with job manager!",
                g_type_name(job_type));
    }
    return info;
}

static const JobTypeInfo *lookup_job_type_info_for_job(const LmiJob *job)
{
    return lookup_job_type_info(G_OBJECT_TYPE(job));
}

static CMPIUint16 job_state_to_cim(LmiJobStateEnum state)
{
    CMPIUint16 res;
    switch (state) {
        case LMI_JOB_STATE_ENUM_NEW:
            res = CIM_CONCRETE_JOB_JOB_STATE_NEW;
            break;
        case LMI_JOB_STATE_ENUM_RUNNING:
            res = CIM_CONCRETE_JOB_JOB_STATE_RUNNING;
            break;
        case LMI_JOB_STATE_ENUM_COMPLETED:
            res = CIM_CONCRETE_JOB_JOB_STATE_COMPLETED;
            break;
        case LMI_JOB_STATE_ENUM_TERMINATED:
            res = CIM_CONCRETE_JOB_JOB_STATE_TERMINATED;
            break;
        case LMI_JOB_STATE_ENUM_EXCEPTION:
            res = CIM_CONCRETE_JOB_JOB_STATE_EXCEPTION;
            break;
        default:
            g_assert_not_reached(); /* impossible */
    }
    return res;
}

static const gchar *job_state_to_status(LmiJobStateEnum state)
{
    const gchar *res;
    switch (state) {
        case LMI_JOB_STATE_ENUM_NEW:
            res = "Enqueued";
            break;
        case LMI_JOB_STATE_ENUM_RUNNING:
            res = "Running";
            break;
        case LMI_JOB_STATE_ENUM_COMPLETED:
            res = "Completed successfully";
            break;
        case LMI_JOB_STATE_ENUM_TERMINATED:
            res = "Terminated";
            break;
        case LMI_JOB_STATE_ENUM_EXCEPTION:
            res = "Failed";
            break;
        default:
            g_assert_not_reached(); /* impossible */
    }
    return res;
}

static CMPIArray *job_state_to_cim_operational_status(LmiJobStateEnum state)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIUint16 fst, snd = 0;
    CMPIValue value;
    CMPIArray *res = CMNewArray(_cb,
            state == LMI_JOB_STATE_ENUM_COMPLETED ? 2 : 1,
            CMPI_uint16, &status);

    if (!res || status.rc) {
        if (status.msg) {
            lmi_error(CMGetCharPtr(status.msg));
        } else {
            lmi_error("Memory allocation failed");
        }
    } else {
        switch (state) {
            case LMI_JOB_STATE_ENUM_NEW:
                fst = CIM_CONCRETE_JOB_OPERATIONAL_STATUS_DORMANT;
                break;
            case LMI_JOB_STATE_ENUM_RUNNING:
                fst = CIM_CONCRETE_JOB_OPERATIONAL_STATUS_OK;
                break;
            case LMI_JOB_STATE_ENUM_COMPLETED:
                fst = CIM_CONCRETE_JOB_OPERATIONAL_STATUS_OK;
                snd = CIM_CONCRETE_JOB_OPERATIONAL_STATUS_COMPLETED;
                break;
            case LMI_JOB_STATE_ENUM_TERMINATED:
                fst = CIM_CONCRETE_JOB_OPERATIONAL_STATUS_STOPPED;
                break;
            case LMI_JOB_STATE_ENUM_EXCEPTION:
                fst = CIM_CONCRETE_JOB_OPERATIONAL_STATUS_ERROR;
                break;
            default:
                g_assert_not_reached(); /* impossible */
        }

        value.uint16 = fst;
        status = CMSetArrayElementAt(res, 0, &value, CMPI_uint16);
        if (!status.rc && snd) {
            value.uint16 = snd;
            status = CMSetArrayElementAt(res, 1, &value, CMPI_uint16);
        }
        if (status.rc) {
            lmi_error("Memory allocation failed");
            CMRelease(res);
            res = NULL;
        }
    }
    return res;
}

static CMPIStatus return_value_to_string(
        CMPIInstance *method_parameters,
        CMPIString **string)
{
    gchar *tmpcstr = NULL;
    CMPIString *tmp;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIData data = CMGetProperty(method_parameters, "__ReturnValue", &status);
    g_assert(string != NULL);

    if (!status.rc)
        status = lmi_data_to_string(&data, &tmpcstr);
    if (!status.rc)
        tmp = CMNewString(_cb, tmpcstr, &status);
    g_free(tmpcstr);
    if (!status.rc)
        *string = tmp;

    return status;
}

static CMPIStatus make_inst_method_call_instance_for_job(const LmiJob *job,
                                                         gboolean pre,
                                                         CMPIInstance **instance)
{
    gchar *namespace;
    CMPIObjectPath *op;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIValue value;
    const JobTypeInfo *info;
    gchar buf[BUFLEN];
    CMPIArray *arr;
    LmiJobStateEnum state;
    g_assert(instance != NULL);

    if ((namespace = lmi_read_config("CIM", "Namespace")) == NULL) {
        lmi_error("Memory allocation failed");
        goto err;
    }

    op = CMNewObjectPath(_cb, namespace,
                    CIM_INST_METHOD_CALL_CLASS_NAME, &status);
    if (op == NULL || status.rc)
        goto namespace_err;
    g_free(namespace);
    namespace = NULL;

    if ((*instance = CMNewInstance(_cb, op, &status)) == NULL || status.rc)
        goto op_err;
    op = NULL;

    info = lookup_job_type_info_for_job(job);

    JOB_CRITICAL_BEGIN(job);

    if ((status = jobmgr_job_to_cim_instance(job, &value.inst)).rc)
        goto critical_end_err;
    if ((status = CMSetProperty(*instance, "SourceInstance",
            &value, CMPI_instance)).rc)
    {
        CMRelease(value.inst);
        goto critical_end_err;
    }

    if ((status = jobmgr_job_to_cim_op(job, &op)).rc)
        goto critical_end_err;
    if ((value.string = CMObjectPathToString(op, &status)) == NULL) {
        lmi_error("Memory allocation failed");
        goto critical_end_err;
    }
    CMRelease(op);
    op = NULL;
    if ((status = CMSetProperty(*instance, "SourceInstanceModelPath",
                    &value, CMPI_string)).rc)
        goto string_err;

    if ((value.string = CMNewString(_cb, lmi_job_get_method_name(job),
                    &status)) == NULL)
    {
        lmi_error("Memory allocation failed");
        goto critical_end_err;
    }
    if ((status = CMSetProperty(*instance, "MethodName",
                    &value, CMPI_string)).rc)
        goto string_err;

    value.boolean = pre != FALSE;
    if ((status = CMSetProperty(*instance, "PreCall", &value, CMPI_boolean)).rc)
        goto critical_end_err;

    value.uint16 = (  info->return_value_type
            + INST_METHOD_CALL_RETURN_VALUE_TYPE_0);
    if ((status = CMSetProperty(*instance, "ReturnValueType",
                    &value, CMPI_uint16)).rc)
        goto critical_end_err;

    if (!pre) {
        state = lmi_job_get_state(job);
        if ((arr = CMNewArray(_cb, state == LMI_JOB_STATE_ENUM_EXCEPTION ? 1:0,
                CMPI_instance, &status)) == NULL)
        {
            lmi_error("Memory allocation failed");
            goto critical_end_err;
        }
        if (state == LMI_JOB_STATE_ENUM_EXCEPTION) {
            if ((status = jobmgr_job_to_cim_error(job, &value.inst)).rc)
                goto arr_err;
            if ((status = CMSetArrayElementAt(
                            arr, 0, &value.inst, CMPI_instance)).rc)
            {
                CMRelease(value.inst);
                goto arr_err;
            }
        }
        value.array = arr;
        if ((status = CMSetProperty(*instance, "Error",
                    &value, CMPI_instanceA)).rc)
        {
            lmi_error("Memory allocation failed");
            goto arr_err;
        }
    }

    if (lmi_job_get_method_name(job) != NULL) {
        gboolean include_output = !pre &&
                  lmi_job_get_state(job) == LMI_JOB_STATE_ENUM_COMPLETED;
        make_method_parameters_class_name(buf, BUFLEN,
                lmi_job_get_method_name(job), include_output);
        if ((status = make_method_parameters_cim_instance_for_job(
                    job, buf, &value.inst)).rc)
            goto critical_end_err;
        if ((status = info->make_job_params_func(
                        _cb, _cmpi_ctx, job,
                        TRUE,       /* include input parameters */
                        include_output,
                        value.inst)).rc)
        {
            CMRelease(value.inst);
            goto critical_end_err;
        }
        if ((status = CMSetProperty(*instance, "MethodParameters",
                &value, CMPI_instance)).rc)
        {
            CMRelease(value.inst);
            goto critical_end_err;
        }

        if (include_output) {
            CMPIString *tmp = NULL;
            status = return_value_to_string(value.inst, &tmp);
            if (status.rc) {
                if (status.rc == CMPI_RC_ERR_NO_SUCH_PROPERTY) {
                    gchar *jobid = lmi_job_get_jobid(job);
                    lmi_warn("__ReturnValue property unset in method"
                           " parameters instance for job %s", jobid);
                    g_free(jobid);
                    CMSetStatus(&status, CMPI_RC_OK);
                } else {
                    goto critical_end_err;
                }
            }
            if (tmp != NULL) {
                value.string = tmp;
                if ((status = CMSetProperty(*instance, "ReturnValue",
                            &value, CMPI_string)).rc)
                    goto string_err;
            }
        }
    }

    JOB_CRITICAL_END(job);

    return status;

arr_err:
    CMRelease(value.array);
    goto critical_end_err;
string_err:
    CMRelease(value.string);
critical_end_err:
    JOB_CRITICAL_END(job);
    CMRelease(*instance);
    *instance = NULL;
    goto err;
op_err:
    if (op)
        CMRelease(op);
namespace_err:
    g_free(namespace);
err:
    if (!status.rc)
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    return status;
}

/**
 * Make a CIM class name for instance representing method parameters.
 * This is constructed dependeing on *output* parameter:
 *
 *      a) *output* is `FALSE`
 *          `__MethodParameters_<method_name>`
 *      b) *output* is `TRUE`
 *          `__MethodParameters_<method_name>_Result`
 *
 * Both classes need to be properly registered wih CIMOM. The latter is a
 * subclass of the former.
 *
 * @param output Whether the desired instance shall contain output parameters.
 */
static gsize make_method_parameters_class_name(gchar *buf,
                                               gsize buflen,
                                               const gchar *method_name,
                                               gboolean output)
{
    return g_snprintf(buf, buflen, "__MethodParameters_%s%s",
                    method_name, output ? "_Result" : "");
}

/**
 * Create an instance of class holding method parameters.
 *
 * @param cim_class_name Name of CIM class obtained with
 *      `make_method_parameters_class_name()`.
 */
static CMPIStatus make_method_parameters_cim_instance_for_job(
    const LmiJob *job,
    const gchar *cim_class_name,
    CMPIInstance **instance)
{
    gchar *namespace;
    CMPIObjectPath *op;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    g_assert(instance != NULL);

    if ((namespace = lmi_read_config("CIM", "Namespace")) == NULL) {
        lmi_error("Memory allocation failed");
        goto err;
    }

    op = CMNewObjectPath(_cb, namespace, cim_class_name, &status);
    g_free(namespace);
    if (op == NULL || status.rc)
        goto err;

    if ((*instance = CMNewInstance(_cb, op, &status)) == NULL || status.rc)
        goto op_err;

    return status;

op_err:
    CMRelease(op);
err:
    if (!status.rc) {
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    } else if (status.rc == CMPI_RC_ERR_NOT_FOUND) {
        lmi_error("Method parameters class %s isn't registered with CIMOM!",
                cim_class_name);
    }
    return status;
}

/**
 * Set `JobInParameters` and `JobOutParameters` of `LMI_ConcreteJob` instance.
 *
 * Note that `JobInParameters are not currently set due to shortcomings of
 * tog-pegasus broker.
 *
 * `JobOutParameters` shall be set only for completed job.
 */
static CMPIStatus fill_cim_job_parameters(const LmiJob *job,
                                          CMPIInstance *instance)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIValue value;
    gchar buf[BUFLEN] = {0};
    const JobTypeInfo *info;
    const gchar *method_name;
    g_assert(instance != NULL);

    if ((method_name = lmi_job_get_method_name(job)) != NULL) {
        info = lookup_job_type_info_for_job(job);
        make_method_parameters_class_name(
                buf, BUFLEN, method_name, FALSE);
        if ((status = make_method_parameters_cim_instance_for_job(
                    job, buf, &value.inst)).rc)
            goto err;
        if ((status = info->make_job_params_func(
                        _cb, _cmpi_ctx, job, TRUE, FALSE, value.inst)).rc)
            goto inst_err;
        if ((status = CMSetProperty(instance, "JobInParameters",
                &value, CMPI_instance)).rc)
            goto inst_err;

        if (lmi_job_get_state(job) == LMI_JOB_STATE_ENUM_COMPLETED) {
            make_method_parameters_class_name(buf, BUFLEN, method_name, TRUE);
            if ((status = make_method_parameters_cim_instance_for_job(
                        job, buf, &value.inst)).rc)
                goto err;
            if ((status = info->make_job_params_func(
                            _cb, _cmpi_ctx, job, FALSE, TRUE, value.inst)).rc)
                goto inst_err;
            if ((status = CMSetProperty(instance, "JobOutParameters",
                    &value, CMPI_instance)).rc)
                goto inst_err;
        }
    }

    return status;

inst_err:
    if (status.rc && status.msg != NULL) {
        lmi_error(CMGetCharsPtr(status.msg, NULL));
    } else {
        lmi_error("Memory allocation failed");
    }
    CMRelease(value.inst);
err:
    return status;
}

/**
 * Create an instance of `LMI_ConcreteJob` corresponding to given job.
 *
 * Note this does not call `convert_func()` callback to fill additional
 * properties.
 */
static CMPIStatus job_to_cim_instance(const LmiJob *job,
                                      CMPIInstance **instance)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIObjectPath *op;
    CMPIInstance *inst;
    CMPIValue value;
    CMPIData data;
    LmiJobStateEnum state;
    CMPIUint64 elapsed = 0;
    g_assert(instance);
    g_assert(LMI_IS_JOB(job));
    g_assert(_initialized_counter > 0);

    if ((status = jobmgr_job_to_cim_op(job, &op)).rc)
        goto err;

    if ((inst = CMNewInstance(_cb, op, &status)) == NULL || status.rc)
        goto op_err;

    data = CMGetKey(op, "InstanceID", NULL);
    value.string = CMClone(data.value.string, &status);
    if (value.string == NULL || status.rc)
        goto inst_err;
    if ((status = CMSetProperty(inst, "InstanceID", &value, CMPI_string)).rc)
        goto string_err;

    value.uint16 = CIM_CONCRETE_JOB_COMMUNICATION_STATUS_NOT_AVAILABLE;
    if ((status = CMSetProperty(inst, "CommunicationStatus",
                    &value, CMPI_uint16)).rc)
        goto inst_err;

    value.boolean = lmi_job_get_delete_on_completion(job);
    if ((status = CMSetProperty(inst, "DeleteOnCompletion",
                    &value, CMPI_boolean)).rc)
        goto inst_err;

    if (lmi_job_is_finished(job)) {
        elapsed = ( lmi_job_get_time_of_last_state_change(job)
                  - lmi_job_get_start_time(job));
    } else if (lmi_job_get_state(job) == LMI_JOB_STATE_ENUM_RUNNING) {
        elapsed = time(NULL) - lmi_job_get_start_time(job);
    }
    value.dateTime = CMNewDateTimeFromBinary(_cb,
                   elapsed * 1000000L, TRUE, &status);
    if (value.dateTime == NULL || status.rc)
        goto inst_err;
    if ((status = CMSetProperty(inst, "ElapsedTime",
                    &value, CMPI_dateTime)).rc)
        goto datetime_err;

    value.string = CMNewString(_cb,
                    lmi_job_get_jobid(job), &status);
    if (value.string == NULL || status.rc)
        goto inst_err;
    if ((status = CMSetProperty(inst, "ElementName", &value, CMPI_string)).rc)
        goto string_err;

    state = lmi_job_get_state(job);
    value.uint16 = state == LMI_JOB_STATE_ENUM_EXCEPTION ? 1 : 0;
    if ((status = CMSetProperty(inst, "ErrorCode", &value, CMPI_uint16)).rc)
        goto inst_err;

    if ((status = fill_cim_job_parameters(job, inst)).rc) {
        goto inst_err;
    }

    value.uint16 = job_state_to_cim(state);
    if ((status = CMSetProperty(inst, "JobState", &value, CMPI_uint16)).rc)
        goto inst_err;

    if (  (value.string = CMNewString(_cb,
                    job_state_to_status(state), &status)) == NULL
       || status.rc)
        goto inst_err;
    if ((status = CMSetProperty(inst, "JobStatus", &value, CMPI_string)).rc)
        goto string_err;

    value.uint16 = CIM_CONCRETE_JOB_LOCAL_OR_UTC_TIME_UTC_TIME;
    if ((status = CMSetProperty(inst, "LocalOrUtcTime",
                    &value, CMPI_uint16)).rc)
        goto inst_err;

    if (lmi_job_get_method_name(job)) {
        if (  (value.string = CMNewString(_cb,
                        lmi_job_get_method_name(job), &status)) == NULL
           || status.rc)
            goto inst_err;
    }else {
        value.string = NULL;
    }
    if ((status = CMSetProperty(inst, "MethodName", &value, CMPI_string)).rc)
        goto string_err;

    if (lmi_job_get_name(job)) {
        if (  (value.string = CMNewString(_cb,
                        lmi_job_get_name(job), &status)) == NULL
           || status.rc)
            goto inst_err;
    }else {
        value.string = NULL;
    }
    if ((status = CMSetProperty(inst, "Name", &value, CMPI_string)).rc)
        goto string_err;

    value.uint16 = lmi_job_get_percent_complete(job);
    if ((status = CMSetProperty(inst, "PercentComplete",
                    &value, CMPI_uint16)).rc)
        goto inst_err;

    if ((value.array = job_state_to_cim_operational_status(state)) == NULL)
        goto inst_err;
    if ((status = CMSetProperty(inst, "OperationalStatus",
                    &value, CMPI_uint16A)).rc)
        goto array_err;

    value.uint32 = lmi_job_get_priority(job);
    if ((status = CMSetProperty(inst, "Priority", &value, CMPI_uint32)).rc)
        goto inst_err;

    if (state != LMI_JOB_STATE_ENUM_NEW) {
        value.dateTime = CMNewDateTimeFromBinary(_cb,
                       lmi_job_get_start_time(job) * 1000000L,
                       FALSE, &status);
        if (value.dateTime == NULL || status.rc)
            goto inst_err;
        if ((status = CMSetProperty(inst, "StartTime",
                        &value, CMPI_dateTime)).rc)
            goto datetime_err;
    }

    value.dateTime = CMNewDateTimeFromBinary(_cb,
                   lmi_job_get_time_before_removal(job) * 1000000L,
                   TRUE, &status);
    if (value.dateTime == NULL || status.rc)
        goto inst_err;
    if ((status = CMSetProperty(inst, "TimeBeforeRemoval",
                    &value, CMPI_dateTime)).rc)
        goto datetime_err;

    value.dateTime = CMNewDateTimeFromBinary(_cb,
                   lmi_job_get_time_of_last_state_change(job) * 1000000L,
                   FALSE, &status);
    if (value.dateTime == NULL || status.rc)
        goto inst_err;
    if ((status = CMSetProperty(inst, "TimeOfLastStateChange",
                    &value, CMPI_dateTime)).rc)
        goto datetime_err;

    value.dateTime = CMNewDateTimeFromBinary(_cb,
                   lmi_job_get_time_submitted(job) * 1000000L,
                   FALSE, &status);
    if (value.dateTime == NULL || status.rc)
        goto inst_err;
    if ((status = CMSetProperty(inst, "TimeSubmitted",
                    &value, CMPI_dateTime)).rc)
        goto datetime_err;

    *instance = inst;
    return status;

array_err:
    CMRelease(value.array);
    goto inst_err;
datetime_err:
    CMRelease(value.dateTime);
    goto inst_err;
string_err:
    CMRelease(value.string);
inst_err:
    CMRelease(inst);
op_err:
    CMRelease(op);
err:
    if (!status.rc) {
        lmi_error("Memory allocation failed");
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    }
    return status;
}

struct _CalendarEventContainer {
    LmiJob *job;
    GList *matches;
};

static CMPIStatus delete_job(LmiJob *job)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    const JobTypeInfo *info;
    gchar *file_path = NULL;
    GFile *file = NULL;
    gchar *jobid = NULL;
    struct _CalendarEventContainer cec = {job, NULL};
    char err_buf[BUFLEN] = {0};
    CMPIInstance *inst = NULL;
    GError *gerror = NULL;
    IndSenderStaticFilterEnum filter_id = IND_SENDER_STATIC_FILTER_ENUM_DELETED;

    JOB_CRITICAL_BEGIN(job);

    if ((jobid = lmi_job_get_jobid(job)) == NULL)
        goto mem_err;
    if (!lmi_job_is_finished(job)) {
        snprintf(err_buf, BUFLEN,
                "Can't delete unfinished job \"%s\"!", jobid);
        CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_FAILED, err_buf);
        lmi_error(err_buf);
        goto done;
    }
    if ((info = lookup_job_type_info_for_job(job)) == NULL) {
        CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_NOT_FOUND,
                "Can't delete unregistered job!");
        goto done;
    }

    g_signal_handlers_disconnect_by_func(job, job_modified_callback, NULL);
    g_signal_handlers_disconnect_by_func(job, job_finished_callback, NULL);
    g_signal_handlers_disconnect_by_func(job,
            job_deletion_request_changed_callback, NULL);

    g_tree_foreach(_event_calendar, find_event_by_job, &cec);
    if (cec.matches) {
        g_list_foreach(cec.matches, find_and_delete_calendar_event, NULL);
        g_list_free(cec.matches);
    }

    if (info->use_persistent_storage) {
        if ((file_path = make_job_file_path(job)) == NULL)
            goto mem_err;
        if ((file = g_file_new_for_path(file_path)) == NULL)
            goto mem_err;
        if (!g_file_delete(file, NULL, &gerror)) {
            lmi_error("Failed to delete job's file \"%s\": %s",
                    file_path, gerror->message);
            goto err;
        }
        g_clear_object(&file);
        g_free(file_path);
        file_path = NULL;
    }

    /* create source instance for instance deletion indication */
    if (  (status = job_to_cim_instance(job, &inst)).rc
       || (  info->convert_func
          && (status = (*info->convert_func) (_cb, _cmpi_ctx, job, inst)).rc))
    {
        if (status.msg) {
            lmi_error("Failed to create instance deletion indication: %s",
                    CMGetCharsPtr(status.msg, NULL));
        } else {
            lmi_error("Failed to create instance deletion indication!");
        }
    }

    JOB_CRITICAL_END(job);
    /* this effectively destroys the job object */
    g_tree_remove(_job_map, GUINT_TO_POINTER(lmi_job_get_number(job)));
    lmi_info("Deleted job instance \"%s\".", jobid);

    if (inst) {
        /* send instance deletion indication */
        send_indications(jobid, inst, NULL, &filter_id, 1);
        CMRelease(inst);
    }
    g_free(jobid);
    return status;

mem_err:
    lmi_error("Memory allocation failed");
err:
    g_clear_error(&gerror);
    g_clear_object(&file);
    g_free(file_path);
    CMSetStatus(&status, CMPI_RC_ERR_FAILED);
done:
    JOB_CRITICAL_END(job);
    g_free(jobid);
    return status;
}

struct _DuplicateEventContainer {
    CalendarEvent *subject;
    CalendarEvent *duplicate;
};

/**
 * Traversing function for `_event_calendar` that just founds duplicate event
 * ignoring scheduled time.
 */
static gboolean reschedule_present_event(gpointer key,
                                         gpointer value,
                                         gpointer data)
{
    CalendarEvent *event = key;
    struct _DuplicateEventContainer *cnt = data;
    if (event->action == cnt->subject->action &&
            event->job == cnt->subject->job)
    {   /* same event is already scheduled */
        cnt->duplicate = event;
        return TRUE;
    }
    return FALSE;
}

static gboolean process_calendar_event(gpointer unused)
{
    CalendarEvent *event;
    JobActionEnum action;
    LmiJob *job;
    gchar *jobid = NULL;
    guint64 timeout;
    guint64 current_millis;
    g_assert(g_main_context_is_owner(_main_ctx));

    JM_CRITICAL_BEGIN;
    while (  (event = lmi_tree_peek_first(_event_calendar, NULL))
          && event->time <= ((guint64) time(NULL))*1000L)
    {
        action = event->action;
        job = event->job;
        jobid = lmi_job_get_jobid(job);
        lmi_debug("Processing scheduled event \"%s\" on job \"%s\".",
                _action_names[event->action], jobid);
        g_tree_remove(_event_calendar, event);

        switch (action) {
            case JOB_ACTION_REMOVE:
                /* last check before removal */
                if (lmi_job_get_delete_on_completion(job))
                    delete_job(job);
                break;
            default:
                g_assert_not_reached();  /* no other actions yet */
                break;
        }
    }
    if (event) {
        current_millis = ((guint64) time(NULL))*1000L;
        if (current_millis > event->time) {
            timeout = 0;
        } else {
            timeout = event->time - current_millis;
        }
        if (jobid == NULL)
            jobid = lmi_job_get_jobid(event->job);
        /* Can't use:
         *      g_timeout_add((guint) timeout, process_calendar_event, NULL);
         * since it does not ensure the callback will be called in desired
         * context (_main_ctx). */
        GSource *source = g_timeout_source_new((guint) timeout);
        g_source_set_callback(source, process_calendar_event, NULL, NULL);
        g_source_attach(source, _main_ctx);
        lmi_debug("Next event \"%s\" will be executed"
                " after \"%llu\" milliseconds on job \"%s\".",
                _action_names[event->action], timeout, jobid);
    }
    JM_CRITICAL_END;
    g_free(jobid);
    return FALSE;
}

/**
 * @param timeout Number of milliseconds to wait before running the action.
 */
static void schedule_event(guint64 timeout,
                           LmiJob *job,
                           JobActionEnum action)
{
    CalendarEvent *event;
    struct _DuplicateEventContainer cnt;
    gchar *jobid = NULL;
    guint64 current_time;
    g_assert(LMI_IS_JOB(job));

    current_time = (guint64) time(NULL);

    if ((event = g_new(CalendarEvent, 1)) == NULL) {
        lmi_error("Memory allocation failed");
    } else {
        event->time = current_time*1000L + timeout;
        event->job = job;
        event->action = action;
        cnt.subject = event;
        cnt.duplicate = NULL;
        g_tree_foreach(_event_calendar, reschedule_present_event, &cnt);
        if (cnt.duplicate && cnt.duplicate->time > event->time)
        {   /* remove duplicate event only if it's scheduled later */
            lmi_debug("Removing duplicate event [%s] on job %s "
                   " timing out in %lu seconds.", _action_names[event->action],
                   jobid, cnt.duplicate->time/1000L);
            g_tree_remove(_event_calendar, cnt.duplicate);
        }
        if (!cnt.duplicate || cnt.duplicate->time > event->time) {
            g_tree_insert(_event_calendar, event, NULL);
            jobid = lmi_job_get_jobid(job);
            lmi_debug("Scheduled %s action on job %s to occur after %lu"
                   " seconds.", _action_names[event->action],
                   jobid, timeout/1000L);
            if (event == lmi_tree_peek_first(_event_calendar, NULL)) {
                GSource *source = g_timeout_source_new(timeout);
                g_source_set_callback(source, process_calendar_event, NULL, NULL);
                g_source_attach(source, _main_ctx);
            }
        } else {
            g_free(event);
        }
    }
    g_free(jobid);
}

/**
 * Send *filter_ids_count* indications related to given job.
 *
 * @note This call needs to be guarded with `_lock`.
 *
 * @param source_instance Instance representing current state of a job.
 * @param previous_instance This is optional parameter used only for
 *      following filter identificators:
 *
 *          * IND_SENDER_STATIC_FILTER_ENUM_CHANGED
 *          * IND_SENDER_STATIC_FILTER_ENUM_PERCENT_UPDATED
 *          * IND_SENDER_STATIC_FILTER_ENUM_SUCCEEDED
 *          * IND_SENDER_STATIC_FILTER_ENUM_FAILED
 *
 *      It's an instance representing previous state of a job.
 * @param filter_ids An array of filter identificators that apply to the most
 *      current job's change. For example if job's state changes to
 *      `COMPLETED`, you'd pass an array `{ CHANGED, SUCCEEDED }`.
 * @param filter_ids_count Number of filter identificators in *filter_ids* array.
 */
static CMPIStatus send_indications(const gchar *jobid,
                                   CMPIInstance *source_instance,
                                   CMPIInstance *previous_instance,
                                   IndSenderStaticFilterEnum *filter_ids,
                                   guint filter_ids_count)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    const CMPIBroker *cb;
    gchar ind_cls_name[BUFLEN];
    gchar *ind_cls_name_suffix;
    const gchar *filter_name;
    g_assert(source_instance);
    g_assert(filter_ids != NULL);

    for (guint index = 0; index < filter_ids_count; ++index) {
        g_assert(filter_ids[index] < IND_SENDER_STATIC_FILTER_ENUM_LAST);
        filter_name = ind_sender_static_filter_names[filter_ids[index]];

        if (filter_ids[index] == IND_SENDER_STATIC_FILTER_ENUM_CREATED) {
            ind_cls_name_suffix = "Creation";
        } else if (filter_ids[index] == IND_SENDER_STATIC_FILTER_ENUM_DELETED) {
            ind_cls_name_suffix = "Deletion";
        } else {
            ind_cls_name_suffix = "Modification";
        }

        g_snprintf(ind_cls_name, BUFLEN, LMI_ORGID "_%sInst%s",
                _profile_name, ind_cls_name_suffix);

        /* Get a broker object which is associated with particular indication
         * class */
        if ((cb = g_tree_lookup(_indication_brokers, ind_cls_name)) == NULL) {
            lmi_debug("Not sending instance of indication %s[%s] which is not"
                   " initialized (jobid=%s).", ind_cls_name, filter_name, jobid);
            continue;
        }

        if (filter_ids[index] == IND_SENDER_STATIC_FILTER_ENUM_CREATED) {
            status = ind_sender_send_instcreation(cb, _cmpi_ctx,
                source_instance, filter_name);
        } else if (filter_ids[index] == IND_SENDER_STATIC_FILTER_ENUM_DELETED)
        {
            status = ind_sender_send_instdeletion(cb, _cmpi_ctx,
                source_instance, filter_name);
        } else {
            if (previous_instance == NULL) {
                lmi_error("Previous instance must be given in order to send"
                          " %s indication", ind_cls_name);
                continue;
            }
            status = ind_sender_send_instmodification(cb, _cmpi_ctx,
                previous_instance, source_instance, filter_name);
        }

        if (status.rc) {
            if (status.msg) {
                lmi_error("Failed to send instance of indication %s[%s]"
                        " \"%s\": %s!", ind_cls_name, filter_name, jobid,
                        CMGetCharPtr(status.msg));
            } else {
                lmi_error("Failed to send instance of indication %s[%s] for job"
                        " \"%s\"!", ind_cls_name, filter_name, jobid);
            }
            break;
        }
        lmi_info("Sent instance of indication %s[%s] for job \"%s\".",
                ind_cls_name, filter_name, jobid);
    }

    return status;
}

/**
 * Callback run when job moves to finished process.
 */
static void job_processed_callback(GObject *object,
                                   GAsyncResult *result,
                                   gpointer user_data)
{
    LmiJob *job = LMI_JOB(object);
    gchar *jobid;
    g_assert(g_main_context_is_owner(_main_ctx));

    JM_CRITICAL_BEGIN;
    JOB_CRITICAL_BEGIN(job);

    if ((jobid = lmi_job_get_jobid(job)) == NULL) {
        lmi_error("Memory allocation failed");
        goto err;
    }
    lmi_debug("Job #%u (jobid=%s, state=%s) processed.",
            lmi_job_get_number(job), jobid,
            lmi_job_state_to_string(lmi_job_get_state(job)));
    if (!lmi_job_is_finished(job)) {
        lmi_error("Job processing callback did not finish a job \"%s\"!"
                " Setting state to EXCEPTION!", jobid);
        lmi_job_finish_exception(job,
                LMI_JOB_STATUS_CODE_ENUM_FAILED,
                "job processing callback terminated unexpectedly");
    }

    g_tree_remove(_running_jobs, job);
    if (lmi_job_get_delete_on_completion(job)) {
        time_t tbr = lmi_job_get_time_before_removal(job);
        schedule_event(((guint64) tbr)*1000L, job, JOB_ACTION_REMOVE);
    }
    if (!_concurrent_processing && lmi_tree_peek_first(_job_queue, NULL))
    {   /* check for another pending job and launch it */
        /* Can't use:
         *      g_idle_add(launch_job, NULL);
         * since it does not ensure the callback will be called in desired
         * context (_main_ctx). */
        GSource *source = g_idle_source_new();
        g_source_set_callback(source, launch_jobs, NULL, NULL);
        g_source_attach(source, _main_ctx);
    }

err:
    JOB_CRITICAL_END(job);
    JM_CRITICAL_END;
}

/**
 * This shall be run in its own thread. It runs synchronous callback provided
 * at job type's registration which finishes the job.
 */
static void job_process_start(GTask *task,
                              gpointer source_object,
                              gpointer task_data,
                              GCancellable *cancellable)
{
    LmiJob *job = LMI_JOB(source_object);
    const JobTypeInfo *info;
    g_assert(G_IS_TASK(task));
    g_assert(LMI_IS_JOB(job));
    g_assert(!lmi_job_is_finished(job));

    JOB_CRITICAL_BEGIN(job);
    if (g_task_return_error_if_cancelled(task)) {
        lmi_job_finish_terminate(job);
        JOB_CRITICAL_END(job);
    } else {
        /* re-launched serialized job may be already in RUNNING state */
        if (lmi_job_get_state(job) == LMI_JOB_STATE_ENUM_NEW)
            lmi_job_start(job);
        JOB_CRITICAL_END(job);
        info = lookup_job_type_info_for_job(job);
        (*info->process_func) (job, cancellable);
        g_task_return_boolean(task, TRUE);
    }
    g_object_unref(task);
}

/**
 * Launch asynchonous thread to process one or more jobs prepaired to be run.
 * Such a job is removed from `_job_queue` and appended to `_running_jobs`.
 */
static gboolean launch_jobs(gpointer unused)
{
    const JobTypeInfo *info;
    LmiJob *job;
    LmiJob *running;
    gchar *jobid = NULL, *running_jobid;
    GTask *task = NULL;
    GCancellable *cancellable = NULL;
    PendingJobKey job_key;
    g_assert(g_main_context_is_owner(_main_ctx));

    JM_CRITICAL_BEGIN;

    while (lmi_tree_peek_first(_job_queue, (gpointer *) &job)) {
        if ((jobid = lmi_job_get_jobid(job)) == NULL)
            goto err;

        info = lookup_job_type_info_for_job(job);

        if (  _concurrent_processing
           || (running = lmi_tree_peek_first(_running_jobs, NULL)) == NULL)
        {
            if (info->running_job_cancellable) {
                if ((cancellable = g_cancellable_new()) == NULL)
                    goto err;
            }
            if ((task = g_task_new(job, cancellable,
                            job_processed_callback, task)) == NULL)
                goto err;
            g_tree_insert(_running_jobs, job, task);
            job_key.priority = lmi_job_get_priority(job);
            job_key.number = lmi_job_get_number(job);
            g_tree_remove(_job_queue, &job_key);
            g_task_run_in_thread(task, job_process_start);
            lmi_debug("Launched task to process job \"%s\".", jobid);
        } else {
            running_jobid = lmi_job_get_jobid(running);
            lmi_debug("Waiting for another job (%s) to finish before starting \"%s\".",
                    running_jobid, jobid);
            g_free(running_jobid);
            break;
        }
        g_free(jobid);
        jobid = NULL;
    }

    JM_CRITICAL_END;
    return FALSE;

err:
    JM_CRITICAL_END;
    g_clear_object(&task);
    g_free(jobid);
    lmi_error("Memory allocation failed");
    return FALSE;
}

/**
 * Compute and cache persistent storage path.
 *
 * @note Since static variable is not guearded, caller must use global
 *      *_lock* as a guard for this function.
 * @return Base directory path, where job files for particular provider
 *      shall be stored. It must not be freed.
 */
static const gchar *get_persistent_storage_path(void)
{
    static gchar *path = NULL;
    g_assert(_initialized_counter > 0);

    if (!path) {
        path = g_strdup_printf("%s/%s",
                    JOB_MANAGER_PERSISTENT_STORAGE_PREFIX,
                    _profile_name);
        if (path == NULL) {
            lmi_error("Memory allocation failed");
        } else {
            for ( size_t i=strlen(JOB_MANAGER_PERSISTENT_STORAGE_PREFIX) + 1
                ; i < strlen(JOB_MANAGER_PERSISTENT_STORAGE_PREFIX)
                                 + strlen(_profile_name); ++i)
            {
                path[i] = tolower(path[i]);
            }
        }
    }
    return path;
}

/**
 * Make an absolute path of job file.
 */
static gchar *make_job_file_path(const LmiJob *job)
{
    const gchar *storage_path;
    gchar *file_path = NULL;
    g_assert(LMI_IS_JOB(job));

    storage_path = get_persistent_storage_path();
    if (storage_path == NULL)
       goto err;
    file_path = g_strdup_printf("%s/%s-%u.json", storage_path,
            g_type_name(G_OBJECT_TYPE(job)), lmi_job_get_number(job));
    if (file_path == NULL)
        goto err;

    return file_path;

err:
    lmi_error("Memory allocation failed");
    return file_path;
}

static void save_job(const LmiJob *job, const JobTypeInfo *info)
{
    gchar *file_path = NULL;
    GFile *file = NULL, *parent = NULL;
    GError *gerror = NULL;
    gchar *tmp = NULL;
    g_assert(LMI_IS_JOB(job));

    if (!info)
        info = lookup_job_type_info_for_job(job);

    if (info->use_persistent_storage) {
        if ((file_path = make_job_file_path(job)) == NULL)
            goto mem_err;
        if ((file = g_file_new_for_commandline_arg(file_path)) == NULL)
            goto mem_err;
        if ((parent = g_file_get_parent(file)) == NULL)
            goto mem_err;
        if ((tmp = g_file_get_parse_name(parent)) == NULL)
            goto mem_err;

        // make sure that job storage dir exists
        if (!g_file_make_directory_with_parents(parent, NULL, &gerror)) {
            if (gerror->code != G_IO_ERROR_EXISTS) {
                lmi_error("Could not make directory for job persistent"
                       " storage at \"%s\": %s", tmp, gerror->message);
                goto done;
            }
        } else {
            lmi_debug("Created persistent storage for jobs at \"%s\".", tmp);
        }
        lmi_job_dump_to_file(job, file_path);
    }
    goto done;

mem_err:
    lmi_error("Memory allocation failed");
done:
    g_clear_object(&parent);
    g_clear_object(&file);
    g_free(file_path);
    g_free(tmp);
    g_clear_error(&gerror);
}

/**
 * Every job must be registered with this function. If the job is not new, it
 * will be put into `_job_queue` as a pending job.
 *
 * @param is_new Indicates whether the job is completely new (it was created by
 *      a call to `jobmgr_new_job()`) or it was loased from a file.
 * @return Whether the job was successfully registered.
 */
static gboolean register_job(LmiJob *job, gboolean is_new)
{
    gchar *jobid;
    const JobTypeInfo *info;
    CMPIStatus status;
    CMPIInstance *inst = NULL;
    g_assert(LMI_IS_JOB(job));

    if ((info = lookup_job_type_info_for_job(job)) == NULL)
        return FALSE;
    if ((jobid = lmi_job_get_jobid(job)) == NULL) {
        lmi_error("Memory allocation failed");
        return FALSE;
    }

    g_signal_connect(job, "modified", G_CALLBACK(job_modified_callback), NULL);
    g_signal_connect(job, "finished", G_CALLBACK(job_finished_callback), NULL);
    g_signal_connect(job, "deletion-request-changed",
            G_CALLBACK(job_deletion_request_changed_callback), NULL);

    g_tree_insert(_job_map, GUINT_TO_POINTER(lmi_job_get_number(job)), job);
    g_object_ref(job);
    save_job(job, info);

    lmi_debug("New job \"%s\" of type %s registered with job manager.",
            jobid, g_type_name(G_OBJECT_TYPE(job)));

    if (is_new) {
        /* send instance creation indication */
        IndSenderStaticFilterEnum filter_id = IND_SENDER_STATIC_FILTER_ENUM_CREATED;
        if ((status = job_to_cim_instance(job, &inst)).rc ||
                (info->convert_func && (status = (*info->convert_func) (
                      _cb, _cmpi_ctx, job, inst)).rc))
        {
            if (status.msg) {
                lmi_error("Failed to create instance creation indication: %s",
                        CMGetCharsPtr(status.msg, NULL));
            } else {
                lmi_error("Failed to create instance creation indication!");
            }
        } else {
            send_indications(jobid, inst, NULL, &filter_id, 1);
        }
        if (inst)
            CMRelease(inst);
    } else {
        if (lmi_job_is_finished(job) && lmi_job_get_delete_on_completion(job))
        {
            time_t seconds_finished = time(NULL) \
                      - lmi_job_get_time_of_last_state_change(job);
            time_t tbr = lmi_job_get_time_before_removal(job);
            tbr = seconds_finished >= tbr ? 0 : (tbr - seconds_finished);
            schedule_event(((guint64) tbr)*1000L, job, JOB_ACTION_REMOVE);
        } else if (!lmi_job_is_finished(job)) {
            jobmgr_run_job(job);
        }
    }

    g_free(jobid);
    return TRUE;
}

static LmiJob *load_job_from_file(gchar const *job_file_path)
{
    LmiJob *res = NULL;
    gchar *tmp = NULL;
    GType job_type;
    gchar job_type_name[BUFLEN];
    const gchar *basename;

    tmp = rindex(job_file_path, '-');
    g_assert(tmp != NULL);
    if ((basename = rindex(job_file_path, '/')) != NULL) {
        ++basename;
    } else {
        basename = job_file_path;
    }
    lmi_debug("Loading job file \"%s\".", basename);
    strncpy(job_type_name, basename, MIN(tmp - basename, BUFLEN));
    job_type_name[MIN(tmp - basename, BUFLEN - 1)] = 0;
    if ((job_type = g_type_from_name(job_type_name)) == 0) {
        lmi_error("Job type \"%s\" is not known to glib type system.",
                job_type_name);
        goto err;
    }
    if ((res = lmi_job_load_from_file(job_file_path, job_type)) != NULL) {
        if (!register_job(res, FALSE)) {
            g_clear_object(&res);
        }
    }
err:
    return res;
}

/**
 * Traversing function of a dictionary with job file paths.
 * Each job is loaded and registered with job manager.
 */
static gboolean load_job_from_file_cb(gpointer key,
                                      gpointer value,
                                      gpointer data)
{
    const gchar *file_path = key;
    guint *count = data;

    LmiJob *res = load_job_from_file(file_path);
    if (res && count) {
        *count = *count + 1;
    }
    return FALSE;
}

static void load_jobs_from_persistent_storage(void)
{
    GFileEnumerator *fenum = NULL;
    GFile *storage, *file = NULL;
    GFileInfo *info = NULL;
    GTree *sorted_job_paths = NULL;
    GError *gerror = NULL;
    const gchar *storage_path;
    const gchar *name;
    size_t length;
    int count = 0;
    GRegex *regex;
    gchar *path;
    g_assert(g_main_context_is_owner(_main_ctx));
    g_assert(_profile_name);

    storage_path = get_persistent_storage_path();
    if (storage_path == NULL)
    {
        lmi_error("Memory allocation failed");
        goto err;
    }
    storage = g_file_new_for_commandline_arg(storage_path);
    if (storage == NULL) {
        lmi_error("Memory allocation failed");
        goto err;
    }

    if ((fenum = g_file_enumerate_children(storage,
                    G_FILE_ATTRIBUTE_STANDARD_NAME ","
                    G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                    G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
                    0, NULL, &gerror)) == NULL)
    {
        lmi_error("Job persistent storage \"%s\" can not be enumerated: %s",
            storage_path, gerror->message);
        goto err;
    }

    if ((sorted_job_paths = g_tree_new_full(
                    cmp_job_file_names, NULL, g_free, NULL)) == NULL)
    {
        lmi_error("Memory allocation failed");
        goto err;
    }
    if ((regex = g_regex_new(JOB_FILE_NAME_REGEX, 0, 0, &gerror)) == NULL) {
        lmi_error("Failed to compile regular expression: %s", gerror->message);
        goto sorted_job_paths_err;
    }

    while ((info = g_file_enumerator_next_file(fenum, NULL, &gerror)) != NULL) {
        name = g_file_info_get_name(info);
        length = strlen(name);

        /* filter files by extension */
        if (length <= strlen(JOB_FILE_EXTENSION) ||
                strncmp(name + length - strlen(JOB_FILE_EXTENSION),
                        JOB_FILE_EXTENSION, strlen(JOB_FILE_EXTENSION)))
            goto next_info;
        /* filter out non-regular files */
        if (g_file_info_get_file_type(info) != G_FILE_TYPE_REGULAR &&
                g_file_info_get_file_type(info) != G_FILE_TYPE_SYMBOLIC_LINK)
            goto next_info;
        /* filter out not accessible */
        if (!g_file_info_get_attribute_boolean(info,
                    G_FILE_ATTRIBUTE_ACCESS_CAN_READ)) {
            goto next_info;
        }

        /* filter by reqular expression */
        if (!g_regex_match(regex, name, 0, NULL)) {
            lmi_warn("File \"%s\" does not match regular expression"
                   " \"%s\".", name, JOB_FILE_NAME_REGEX);
            goto next_info;
        }

        if ((file = g_file_get_child(storage, name)) == NULL) {
            lmi_error("Memory allocation failed");
            goto regex_err;
        }
        if ((path = g_file_get_path(file)) == NULL) {
            g_clear_object(&file);
            lmi_error("Memory allocation failed");
            goto regex_err;
        }
        g_clear_object(&file);
        g_tree_insert(sorted_job_paths, path, NULL);

next_info:
        g_clear_object(&info);
    }
    if (gerror) {
        lmi_error("Can't read persistent storage: %s", gerror->message);
    } else {
        g_tree_foreach(sorted_job_paths, load_job_from_file_cb, &count);
        lmi_info("Successfully loaded %u jobs of %s provider.",
                count, _profile_name);
    }

regex_err:
    g_clear_object(&info);
    g_regex_unref(regex);
sorted_job_paths_err:
    g_tree_unref(sorted_job_paths);
err:
    g_clear_object(&fenum);
    g_clear_object(&storage);
    g_clear_error(&gerror);
    return;
}

static void job_modified_params_free(JobModifiedParams *params)
{
    g_assert(params);

    g_variant_unref(params->old_value);
    g_free(params);
}

static gboolean process_job_modified_callback(gpointer user_data)
{
    CMPIInstance *old, *new;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    LmiJobStateEnum state;
    const JobTypeInfo *info;
    CMPIValue value;
    gchar *jobid = NULL;
    int filter_index = 0;
    IndSenderStaticFilterEnum filter_ids[2];
    char err_buf[BUFLEN] = {0};
    PendingJobKey *job_key = NULL;
    JobModifiedParams *params = user_data;
    g_assert(_initialized_counter);
    g_assert(LMI_IS_JOB(params->job));
    g_assert(g_main_context_is_owner(_main_ctx));

    info = lookup_job_type_info_for_job(params->job);

    JM_CRITICAL_BEGIN;
    JOB_CRITICAL_BEGIN(params->job);

    if ((jobid = lmi_job_get_jobid(params->job)) == NULL)
        goto memory_err;

    lmi_debug("Running modified callback for job %s triggered by"
           " \"%s\" property.", lmi_job_get_jobid(params->job),
            lmi_job_prop_to_string(params->property));

    save_job(params->job, info);

    /* process just following properties */
    if (params->property != LMI_JOB_PROP_ENUM_STATE &&
            params->property != LMI_JOB_PROP_ENUM_PRIORITY &&
            params->property != LMI_JOB_PROP_ENUM_PERCENT_COMPLETE)
        goto done;

    if ((status = job_to_cim_instance(params->job, &new)).rc)
    {
        if (status.msg) {
            snprintf(err_buf, BUFLEN,
                    "Failed to create source instance for job \"%s\": %s!",
                    jobid, CMGetCharPtr(status.msg));
        } else {
            snprintf(err_buf, BUFLEN,
                    "Failed to create source instance for job \"%s\"!",
                    jobid);
        }
        goto jobid_err;
    }

    if (info->convert_func) {
        if ((status = (*info->convert_func) (
                        _cb, _cmpi_ctx, params->job, new)).rc)
        {
            if (status.msg)
                strncpy(err_buf, CMGetCharPtr(status.msg), BUFLEN);
            goto new_err;
        }
    }

    if ((old = CMClone(new, &status)) == NULL || status.rc)
        goto new_err;

    switch (params->property) {
    case LMI_JOB_PROP_ENUM_STATE:
        state = g_variant_get_uint32(params->old_value);
        value.uint16 = job_state_to_cim(state);
        CMSetProperty(old, "JobState", &value, CMPI_uint16);
        if ((value.array = job_state_to_cim_operational_status(state)) == NULL)
            goto old_err;
        CMSetProperty(old, "OperationalStatus", &value, CMPI_uint16A);
        if ((value.string = CMNewString(_cb,
                        job_state_to_status(state),
                        &status)) == NULL || status.rc)
            goto old_err;
        CMSetProperty(old, "JobStatus", &value, CMPI_string);
        switch (lmi_job_get_state(params->job)) {
        case LMI_JOB_STATE_ENUM_COMPLETED:
            filter_ids[filter_index++] = IND_SENDER_STATIC_FILTER_ENUM_SUCCEEDED;
            break;
        case LMI_JOB_STATE_ENUM_EXCEPTION:
            filter_ids[filter_index++] = IND_SENDER_STATIC_FILTER_ENUM_FAILED;
            break;
        default:
            break;
        }
        filter_ids[filter_index++] = IND_SENDER_STATIC_FILTER_ENUM_CHANGED;
        break;

    case LMI_JOB_PROP_ENUM_PERCENT_COMPLETE:
        value.uint16 = g_variant_get_uint32(params->old_value);
        CMSetProperty(old, "PercentComplete", &value, CMPI_uint16);
        filter_ids[filter_index++] = IND_SENDER_STATIC_FILTER_ENUM_PERCENT_UPDATED;
        break;

    case LMI_JOB_PROP_ENUM_PRIORITY:
        if ((job_key = g_new(PendingJobKey, 1)) == NULL)
            goto old_err;
        if (g_tree_lookup(_job_queue, job_key)) {
            /* if job is pending, reinsert it into queue */
            job_key->priority = g_variant_get_uint32(params->old_value);
            job_key->number = lmi_job_get_number(params->job);
            g_tree_remove(_job_queue, job_key);
            job_key->priority = lmi_job_get_priority(params->job);
            g_tree_insert(_job_queue, job_key, params->job);
        }
        value.uint32 = g_variant_get_uint32(params->old_value);
        CMSetProperty(old, "Priority", &value, CMPI_uint32);
        break;

    default:
        break;
    }

    send_indications(jobid, new, old, filter_ids, filter_index);

    CMRelease(old);
    CMRelease(new);
done:
    JOB_CRITICAL_END(params->job);
    JM_CRITICAL_END;
    g_free(jobid);
    return FALSE;

old_err:
    CMRelease(old);
new_err:
    CMRelease(new);
jobid_err:
    g_free(jobid);
memory_err:
    JOB_CRITICAL_END(params->job);
    JM_CRITICAL_END;
    if (!err_buf[0]) {
        lmi_error("Memory allocation failed");
    } else {
        lmi_error(err_buf);
    }
    return FALSE;
}

/**
 * Callback for calling `process_job_modified_callback`. It just passes
 * parameters and ensures the callback is called in correct context.
 */
static void job_modified_callback(LmiJob *job,
                                  LmiJobPropEnum property,
                                  GVariant *old_value,
                                  GVariant *new_value,
                                  gpointer unused)
{
    JobModifiedParams *params;

    if ((params = g_new(JobModifiedParams, 1)) == NULL) {
        lmi_error("Memory allocation failed");
        return;
    }
    params->job = job;
    params->property = property;
    params->old_value = g_variant_ref(old_value);

    g_main_context_invoke_full(
            _main_ctx,
            G_PRIORITY_DEFAULT,
            process_job_modified_callback,
            params,
            (GDestroyNotify) job_modified_params_free);
}

static gboolean process_job_finished_callback(gpointer user_data)
{
    LmiJob *job = LMI_JOB(user_data);
    g_assert(_initialized_counter);
    g_assert(g_main_context_is_owner(_main_ctx));

    JM_CRITICAL_BEGIN;
    JOB_CRITICAL_BEGIN(job);

    gchar *jobid = lmi_job_get_jobid(job);
    PendingJobKey job_key = {lmi_job_get_priority(job), lmi_job_get_number(job)};

    if (!jobid)
        goto memory_err;
    if (lmi_job_get_delete_on_completion(job)) {
        time_t tbr = lmi_job_get_time_before_removal(job);
        schedule_event(((guint64) tbr)*1000L, job, JOB_ACTION_REMOVE);
    }
    g_tree_remove(_job_queue, &job_key);

    JOB_CRITICAL_END(job);
    JM_CRITICAL_END;
    g_free(jobid);
    return FALSE;

memory_err:
    JOB_CRITICAL_END(job);
    JM_CRITICAL_END;
    lmi_error("Memory allocation failed");
    return FALSE;
}

/**
 * This callback just passes relevant arguments to
 * `process_job_finished_callback()` and ensures it is executed in the right
 * context.
 */
static void job_finished_callback(LmiJob *job,
                                  LmiJobStateEnum old_state,
                                  LmiJobStateEnum new_state,
                                  GVariant *result,
                                  const gchar *error,
                                  gpointer unused)
{
    g_main_context_invoke(_main_ctx, process_job_finished_callback, job);
}

static gboolean process_job_deletion_request_changed_callback(
    gpointer user_data)
{
    LmiJob *job = LMI_JOB(user_data);
    gboolean doc;
    gint64 tbr;
    struct _DuplicateEventContainer cnt;
    gchar *jobid;
    CalendarEvent event = {0, job, JOB_ACTION_REMOVE};
    guint64 current_millis = time(NULL) * 1000L;
    g_assert(_initialized_counter);
    g_assert(g_main_context_is_owner(_main_ctx));

    JM_CRITICAL_BEGIN;
    JOB_CRITICAL_BEGIN(job);
    doc = lmi_job_get_delete_on_completion(job);
    tbr = lmi_job_get_time_before_removal(job);

    if (doc) {
        /* schedule finished job for deletion */
        if (lmi_job_is_finished(job)) {
            tbr = ( tbr*1000L
                  /* milliseconds elapsed since finished */
                  - ( current_millis
                    - lmi_job_get_time_of_last_state_change(job)*1000L));
            tbr = MAX(tbr, 0);
            schedule_event(tbr, job, event.action);
        }
    } else {
        /* remove scheduled removal event from calendar if any */
        cnt.subject = &event;
        cnt.duplicate = NULL;
        g_tree_foreach(_event_calendar, reschedule_present_event, &cnt);
        if (cnt.duplicate) {
            jobid = lmi_job_get_jobid(job);
            tbr = cnt.duplicate->time - current_millis;
            tbr = MAX(tbr, 0);
            lmi_debug("Removing event [%s] on job %s "
                   " timing out in %lu seconds.", _action_names[event.action],
                   jobid, tbr/1000L);
            g_tree_remove(_event_calendar, cnt.duplicate);
            g_free(jobid);
        }
    }
    JOB_CRITICAL_END(job);
    JM_CRITICAL_END;

    return FALSE;
}

/**
 * This callback just passes relevant arguments to
 * `process_job_deletion_request_changed_callback()` and ensures it is executed
 * in the right context.
 */
static void job_deletion_request_changed_callback(LmiJob *job,
                                                  gboolean delete_on_completion,
                                                  gint64 time_before_removal,
                                                  gpointer unused)
{
    g_main_context_invoke(_main_ctx,
            process_job_deletion_request_changed_callback, job);
}

/**
 * Traversing function for `_job_type_map` tree. It registeres all known job
 * type classes with indication sender.
 */
static gboolean register_job_classes_with_ind_sender(gpointer key,
                                                     gpointer value,
                                                     gpointer data)
{
    JobTypeInfo *info = value;
    CMPIStatus *status = data;

    if (status->rc)
        return TRUE;
    lmi_debug("Registering static filters for %s.", info->cim_class_name);
    *status = register_static_filters(info->cim_class_name);
    return status->rc ? TRUE:FALSE;
}

static CMPIStatus register_static_filters(const gchar *cim_class_name)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    StaticFilter filters[IND_SENDER_STATIC_FILTER_ENUM_LAST];
    int i;

    for (i=0; i < IND_SENDER_STATIC_FILTER_ENUM_LAST; ++i) {
        filters[i].id = ind_sender_static_filter_names[i];
        if ((filters[i].query = g_strdup_printf(
                _filter_queries[i], _profile_name, cim_class_name)) == NULL)
        {
            lmi_error("Memory allocation failed");
            CMSetStatus(&status, CMPI_RC_ERR_FAILED);
            goto err;
        }
    }
    status = ind_sender_add_static_filters(
            cim_class_name,
            filters,
            IND_SENDER_STATIC_FILTER_ENUM_LAST);

err:
    while (i >= 0) {
        g_free((char *) filters[--i].query);
    }
    return status;
}

static CMPIStatus register_job_type(GType job_type,
                                    const gchar *cim_class_name,
                                    JobToCimInstanceCallback convert_func,
                                    MakeJobParametersCallback make_job_params_func,
                                    MethodResultValueTypeEnum return_value_type,
                                    JobProcessCallback process_func,
                                    gboolean running_job_cancellable,
                                    gboolean use_persistent_storage)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    JobTypeInfo *info;
    g_assert(!_initialized_counter);
    g_assert(g_type_is_a(job_type, LMI_TYPE_JOB));

    if (  _job_type_map == NULL
       && (_job_type_map = g_tree_new_full(
               lmi_tree_ptrcmp_func, NULL, NULL, g_free)) == NULL)
        goto err;

    if ((info = g_new(JobTypeInfo, 1)) == NULL)
        goto err;
    if ((info->cim_class_name = g_strdup(cim_class_name ?
            cim_class_name : DEFAULT_CIM_CLASS_NAME)) == NULL)
        goto err;
    info->running_job_cancellable = running_job_cancellable;
    info->convert_func = convert_func;
    info->make_job_params_func = make_job_params_func;
    info->return_value_type = return_value_type;
    info->process_func = process_func;
    info->use_persistent_storage = use_persistent_storage;
    g_tree_insert(_job_type_map, (void *) ((uintptr_t) job_type), info);

    lmi_debug("Registered job type %s for job class %s.",
           g_type_name(job_type), cim_class_name);

    return status;

err:
    lmi_error("Memory allocation failed");
    CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    return status;
}

/**
 * Traversing function for `_job_type_map`.
 *
 * @param data is a pointer to boolean which is set to TRUE if
 *      any registered job type needs persistent storage.
 */
static gboolean is_persistent_storage_needed(gpointer key,
                                             gpointer value,
                                             gpointer data)
{
    const JobTypeInfo *info = value;
    gboolean *result = data;
    if (info->use_persistent_storage) {
        *result = TRUE;
        return TRUE;
    }
    return FALSE;
}

/**
 * Executes main loop.
 *
 * It takes care of calendar events and executes pending jobs.
 */
static gpointer run(gpointer data)
{
    gboolean load_persistent_storage = FALSE;
    pthread_cond_t *initialized_cond = data;

    lmi_debug("Starting job processing thread.");
    CBAttachThread(_cb, _cmpi_ctx);
    g_main_context_push_thread_default(_main_ctx);

    g_tree_foreach(_job_type_map,
            is_persistent_storage_needed,
            &load_persistent_storage);
    if (load_persistent_storage) {
        lmi_debug("Loading persistent jobs.");
        load_jobs_from_persistent_storage();
    }

    JM_CRITICAL_BEGIN;
    pthread_cond_signal(initialized_cond);
    JM_CRITICAL_END;
    g_main_loop_run(_main_loop);
    lmi_debug("Terminating main loop.");
    g_main_context_pop_thread_default(_main_ctx);
    CBDetachThread(_cb, _cmpi_ctx);
    return NULL;
}

CMPIStatus jobmgr_init(const CMPIBroker *cb,
                       const CMPIContext *ctx,
                       const gchar *profile_name,
                       const gchar *provider_name,
                       gboolean is_indication_provider,
                       gboolean concurrent_processing,
                       const JobTypeEntry *job_types,
                       gsize n_job_types)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    pthread_cond_t thread_started = PTHREAD_COND_INITIALIZER;
    pthread_mutexattr_t lock_attr;
    gchar *tmpstr = NULL;
    g_assert(cb);
    g_assert(ctx);
    g_assert(profile_name);
    g_assert(provider_name);

    lmi_debug("Began initialization for provider %s.", provider_name);
    if ((!_job_type_map || lmi_tree_peek_first(_job_type_map, NULL) == NULL) &&
            n_job_types < 1)
    {
        lmi_error("Can't initialize job manager without"
               " any registered job type!");
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_FAILED,
                "No job type registered for job manager to work.");
        return status;
    }

    pthread_mutex_lock(&_init_lock);
    if (_initialized_counter > 0) {
        if (is_indication_provider) {
            JM_CRITICAL_BEGIN;
            if ((tmpstr = g_strdup(provider_name)) == NULL)
                goto err;
            g_tree_replace(_indication_brokers, tmpstr, (gpointer) cb);
            JM_CRITICAL_END;
        }
        ++_initialized_counter;
        goto init_critical_end;
    }

    pthread_mutexattr_init(&lock_attr);
    pthread_mutexattr_settype(&lock_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock, &lock_attr);
    pthread_mutexattr_destroy(&lock_attr);

    JM_CRITICAL_BEGIN;
    _cb = cb;
    if ((status = ind_sender_init(profile_name)).rc)
        goto done;

    _concurrent_processing = concurrent_processing;
    if ((_profile_name = g_strdup(profile_name)) == NULL)
        goto err;
    if ((_job_map = g_tree_new_full(
                    cmp_uints, NULL, NULL, g_object_unref)) == NULL)
        goto profile_name_err;
    if ((_indication_brokers = g_tree_new_full(
                    lmi_tree_strcmp_func, NULL, g_free, NULL)) == NULL)
        goto job_map_err;
    if ((_job_queue = g_tree_new_full(
                    cmp_lmi_job_queue_keys, NULL, g_free, NULL)) == NULL)
        goto indication_brokers_err;
    if ((_running_jobs = g_tree_new_full(
                    lmi_tree_ptrcmp_func, NULL,
                    g_object_unref, NULL)) == NULL)
            goto job_queue_err;
    if ((_event_calendar = g_tree_new_full(
                    cmp_calendar_events, NULL,
                    free_calendar_event, NULL)) == NULL)
        goto running_jobs_err;
    if ((_main_ctx = g_main_context_new()) == NULL)
        goto event_calendar_err;
    if ((_main_loop = g_main_loop_new(_main_ctx, FALSE)) == NULL)
        goto main_ctx_err;

    if (is_indication_provider) {
        if ((tmpstr = g_strdup(provider_name)) == NULL)
            goto err;
        g_tree_insert(_indication_brokers, tmpstr, (gpointer) cb);
    }

    if (n_job_types > 0) {
        for (gsize i=0; i < n_job_types; ++i) {
            status = register_job_type(
                    job_types[i].job_type,
                    job_types[i].cim_class_name,
                    job_types[i].convert_func,
                    job_types[i].make_job_params_func,
                    job_types[i].return_value_type,
                    job_types[i].process_func,
                    job_types[i].running_job_cancellable,
                    job_types[i].use_persistent_storage);
            if (status.rc)
                goto main_loop_err;
        }
    }
    lmi_debug("Registering static filters with indication sender.");
    g_tree_foreach(_job_type_map, register_job_classes_with_ind_sender, &status);
    if (status.rc)
        goto main_loop_err;

    if ((_cmpi_ctx = CBPrepareAttachThread(cb, ctx)) == NULL)
        goto main_loop_err;
    /* We need to be initialized even before thread is started because first
     * thing the thread do is loading serialized jobs using functions checking
     * for job manager's initialization. */
    ++_initialized_counter;
    if ((_manager_thread = g_thread_new(JOB_MANAGER_THREAD_NAME,
                    run, (gpointer) &thread_started)) == NULL)
        goto initialized_counter_err;

    /* wait for spawned thread to initialize itself and load serialized jobs */
    pthread_cond_wait(&thread_started, &_lock);

    JM_CRITICAL_END;
    pthread_mutex_unlock(&_init_lock);
    lmi_debug("Job Manager initialized for provider %s.", provider_name);
    return status;

initialized_counter_err:
    --_initialized_counter;
main_loop_err:
    g_main_loop_unref(_main_loop);
    _main_loop = NULL;
main_ctx_err:
    g_main_context_unref(_main_ctx);
    _main_ctx = NULL;
event_calendar_err:
    g_tree_unref(_event_calendar);
    _event_calendar = NULL;
running_jobs_err:
    g_tree_unref(_running_jobs);
    _running_jobs = NULL;
job_queue_err:
    g_tree_unref(_job_queue);
    _job_queue = NULL;
job_map_err:
    g_tree_unref(_job_map);
    _job_map = NULL;
indication_brokers_err:
    g_tree_unref(_indication_brokers);
    _indication_brokers = NULL;
profile_name_err:
    g_free(_profile_name);
    _profile_name = NULL;
err:
    ind_sender_cleanup();
    _cb = NULL;
    _cmpi_ctx = NULL;
    if (!status.rc) {
        lmi_error("Memory allocation failed");
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    }
done:
    JM_CRITICAL_END;
init_critical_end:
    pthread_mutex_unlock(&_init_lock);
    return status;
}

CMPIStatus jobmgr_cleanup(const gchar *provider_name)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};

    pthread_mutex_lock(&_init_lock);

    if (_initialized_counter == 0) {
        lmi_error("Job Manager cleanup() called more times than init()!");
    } else {
        g_tree_remove(_indication_brokers, provider_name);
        if (_initialized_counter == 1) {
            JM_CRITICAL_BEGIN;
            g_assert(_manager_thread);
            g_main_loop_quit(_main_loop);
            g_thread_join(_manager_thread);
            g_main_loop_unref(_main_loop);
            _main_loop = NULL;
            g_main_context_unref(_main_ctx);
            _main_ctx = NULL;
            _manager_thread = NULL;
            g_tree_unref(_event_calendar);
            _event_calendar = NULL;
            g_tree_unref(_running_jobs);
            _running_jobs = NULL;
            g_tree_unref(_job_queue);
            _job_queue = NULL;
            g_tree_unref(_job_map);
            _job_map = NULL;
            g_tree_unref(_indication_brokers);
            g_free(_profile_name);
            _profile_name = NULL;
            g_tree_unref(_job_type_map);
            _job_type_map = NULL;
            status = ind_sender_cleanup();
            JM_CRITICAL_END;
        }
        --_initialized_counter;
    }

    pthread_mutex_unlock(&_init_lock);

    lmi_debug("Job Manager cleanup for %s provider finished.", provider_name);

    return status;
}

CMPIStatus jobmgr_register_job_type(GType job_type,
                                    const gchar *cim_class_name,
                                    JobToCimInstanceCallback convert_func,
                                    MakeJobParametersCallback make_job_params_func,
                                    MethodResultValueTypeEnum return_value_type,
                                    JobProcessCallback process_func,
                                    gboolean running_job_cancellable,
                                    gboolean use_persistent_storage)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};

    pthread_mutex_lock(&_init_lock);
    /* Can not register any new job type after job manager is initialized. */
    if (_initialized_counter < 1) {
        status = register_job_type(job_type, cim_class_name, convert_func,
                make_job_params_func, return_value_type, process_func,
                running_job_cancellable, use_persistent_storage);
    }
    pthread_mutex_unlock(&_init_lock);

    return status;
}

LmiJob *jobmgr_new_job(GType job_type)
{
    LmiJob *res = NULL;
    g_assert(_initialized_counter);

    JM_CRITICAL_BEGIN;
    if ((res = g_object_new(job_type, NULL)) == NULL)
        goto err;
    if (!register_job(res, TRUE))
        goto err;

    JM_CRITICAL_END;
    return res;

err:
    g_clear_object(&res);
    lmi_error("Failed to create new job!");
    JM_CRITICAL_END;
    return res;
}

CMPIStatus jobmgr_run_job(LmiJob *job)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    PendingJobKey *job_key = NULL;
    gchar *jobid;
    GSource *source;
    g_assert(_initialized_counter);
    g_assert(LMI_IS_JOB(job));

    JM_CRITICAL_BEGIN;
    JOB_CRITICAL_BEGIN(job);

    jobid = lmi_job_get_jobid(job);

    if (lmi_job_get_method_name(job) == NULL)
        lmi_warn("method-name property of job \"%s\" is not set, indications"
                 "won't work properly and parameters won't be set.", jobid);

    if ((job_key = g_new(PendingJobKey, 1)) == NULL) {
        lmi_error("Memory allocation failed");
        goto err;
    }
    job_key->priority = lmi_job_get_priority(job);
    job_key->number = lmi_job_get_number(job);
    if (g_tree_lookup_extended(_job_queue, &job_key, NULL, NULL)) {
        lmi_warn("Can't run job \"%s\" second time.", jobid);
        CMSetStatusWithChars(_cb, &status,
                CMPI_RC_ERR_FAILED, "Job already started!");
        goto err;
    }
    JOB_CRITICAL_END(job);
    g_tree_insert(_job_queue, job_key, job);
    source = g_idle_source_new();
    g_source_set_callback(source, launch_jobs, NULL, NULL);
    g_source_attach(source, _main_ctx);

    JM_CRITICAL_END;
    g_free(jobid);
    return status;

err:
    g_free(job_key);
    JOB_CRITICAL_END(job);
    JM_CRITICAL_END;
    g_free(jobid);
    return status;
}

CMPIStatus jobmgr_run_job_id(const char *jobid)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    LmiJob *job;
    g_assert(_initialized_counter);

    JM_CRITICAL_BEGIN;
    job = g_tree_lookup(_job_map, jobid);
    JM_CRITICAL_END;
    if (!job) {
        char *msg;
        if (asprintf(&msg, "No such job with id \"%s\"!", jobid)) {
            CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_NOT_FOUND,
                    msg);
            free(msg);
        } else {
            lmi_error("No such job with id \"%s\"!", jobid);
            CMSetStatus(&status, CMPI_RC_ERR_NOT_FOUND);
        }
    }else {
        status = jobmgr_run_job(job);
    }

    return status;
}

static gboolean dup_job_id(gpointer key,
                           gpointer value,
                           gpointer data)
{
    gchar ***dest_ptr = data;
    if ((**dest_ptr = lmi_job_get_jobid(value)) == NULL)
        /* Stop iteration. */
        return TRUE;
    (*dest_ptr)++;
    return FALSE;
}

gchar **jobmgr_get_job_ids(guint *count)
{
    gchar **res = NULL, **ptr;
    guint size;
    g_assert(_initialized_counter);

    JM_CRITICAL_BEGIN;
    size = g_tree_nnodes(_job_map);
    if ((res = g_new(gchar *, size)) == NULL)
        goto err;
    ptr = res;
    g_tree_foreach(_job_map, dup_job_id, &ptr);
    if (ptr - res < size)
        goto res_err;
    *ptr = NULL;
    if (count)
        *count = size;
    JM_CRITICAL_END;

    return res;

res_err:
    while (ptr > res)
        g_free(*(--ptr));
    g_free(res);
    res = NULL;
err:
    JM_CRITICAL_END;
    lmi_error("Memory allocation failed");
    return res;
}

static gboolean dup_job_number(gpointer key,
                               gpointer value,
                               gpointer data)
{
    guint **dest_ptr = data;
    **dest_ptr = lmi_job_get_number(value);
    (*dest_ptr)++;
    return FALSE;
}


guint *jobmgr_get_job_numbers(guint *count)
{
    guint *res = NULL, *ptr;
    guint size;
    g_assert(_initialized_counter);

    JM_CRITICAL_BEGIN;
    size = g_tree_nnodes(_job_map);
    if ((res = g_new(guint, size + 1)) == NULL)
        goto err;
    ptr = res;
    g_tree_foreach(_job_map, dup_job_number, &ptr);
    if (ptr - res < size)
        goto res_err;
    *ptr = 0;
    if (count)
        *count = size;
    JM_CRITICAL_END;

    return res;

res_err:
    g_free(res);
    res = NULL;
err:
    JM_CRITICAL_END;
    lmi_error("Memory allocation failed");
    return res;
}

gchar **jobmgr_get_pending_job_ids(guint *count)
{
    gchar **res = NULL, **ptr;
    guint size;
    g_assert(_initialized_counter);

    JM_CRITICAL_BEGIN;
    size = g_tree_nnodes(_job_queue);
    if ((res = g_new(gchar *, size + 1)) == NULL)
        goto err;
    ptr = res;
    g_tree_foreach(_job_queue, dup_job_id, &ptr);
    if (ptr - res < size)
        goto res_err;
    *ptr = NULL;
    if (count)
        *count = size;
    JM_CRITICAL_END;

    return res;

res_err:
    while (ptr > res)
        g_free(*(--ptr));
    g_free(res);
    res = NULL;
err:
    JM_CRITICAL_END;
    lmi_error("Memory allocation failed");
    return res;
}

guint *jobmgr_get_pending_job_numbers(guint *count)
{
    guint *res = NULL, *ptr;
    guint size;
    g_assert(_initialized_counter);

    JM_CRITICAL_BEGIN;
    size = g_tree_nnodes(_job_queue);
    if ((res = g_new(guint, size + 1)) == NULL)
        goto err;
    ptr = res;
    g_tree_foreach(_job_queue, dup_job_number, &ptr);
    if (ptr - res < size)
        goto res_err;
    *ptr = 0;
    if (count)
        *count = size;
    JM_CRITICAL_END;

    return res;

res_err:
    g_free(res);
    res = NULL;
err:
    JM_CRITICAL_END;
    lmi_error("Memory allocation failed");
    return res;
}

static gboolean dup_running_job_id(gpointer key,
                                   gpointer value,
                                   gpointer data)
{
    gchar ***dest_ptr = data;
    if ((**dest_ptr = lmi_job_get_jobid(key)) == NULL)
        /* Stop iteration. */
        return TRUE;
    (*dest_ptr)++;
    return FALSE;
}

gchar **jobmgr_get_running_job_ids(guint *count)
{
    gchar **res = NULL, **ptr;
    guint size;
    g_assert(_initialized_counter);

    JM_CRITICAL_BEGIN;
    size = g_tree_nnodes(_running_jobs);
    if ((res = g_new(gchar *, size + 1)) == NULL)
        goto err;
    ptr = res;
    g_tree_foreach(_running_jobs, dup_running_job_id, &ptr);
    if (ptr - res < size)
        goto res_err;
    *ptr = NULL;
    if (count)
        *count = size;
    JM_CRITICAL_END;

    return res;

res_err:
    while (ptr > res)
        g_free(*(--ptr));
    g_free(res);
    res = NULL;
err:
    JM_CRITICAL_END;
    lmi_error("Memory allocation failed");
    return res;
}

static gboolean dup_running_job_number(gpointer key,
                                       gpointer value,
                                       gpointer data)
{
    guint **dest_ptr = data;
    **dest_ptr = lmi_job_get_number(key);
    (*dest_ptr)++;
    return FALSE;
}

guint *jobmgr_get_running_job_numbers(guint *count)
{
    guint *res = NULL, *ptr;
    guint size;
    g_assert(_initialized_counter);

    JM_CRITICAL_BEGIN;
    size = g_tree_nnodes(_running_jobs);
    if ((res = g_new(guint, size + 1)) == NULL)
        goto err;
    ptr = res;
    g_tree_foreach(_running_jobs, dup_running_job_number, &ptr);
    if (ptr - res < size)
        goto res_err;
    *ptr = 0;
    if (count)
        *count = size;
    JM_CRITICAL_END;

    return res;

res_err:
    g_free(res);
    res = NULL;
err:
    JM_CRITICAL_END;
    lmi_error("Memory allocation failed");
    return res;
}

struct _JobNameContainer {
    const gchar * name;
    LmiJob *job;
};

static gboolean stop_search_for_job_by_id(gpointer key,
                                          gpointer value,
                                          gpointer data)
{
    LmiJob *job = LMI_JOB(value);
    struct _JobNameContainer *jnc = data;
    if (!g_strcmp0(lmi_job_get_jobid(job), jnc->name)) {
        jnc->job = job;
        return TRUE;
    }
    return FALSE;
}

LmiJob *jobmgr_get_job_by_id(const gchar *jobid)
{
    struct _JobNameContainer jnc = {jobid, NULL};
    g_assert(_initialized_counter);
    g_assert(jobid);

    JM_CRITICAL_BEGIN;
    g_tree_foreach(_job_map, stop_search_for_job_by_id, &jnc);
    JM_CRITICAL_END;
    if (jnc.job)
        g_object_ref(jnc.job);

    return jnc.job;
}

struct _JobNumberContainer {
    guint number;
    LmiJob *job;
};

static gboolean stop_search_for_job_by_number(gpointer key,
                                              gpointer value,
                                              gpointer data)
{
    LmiJob *job = LMI_JOB(value);
    struct _JobNumberContainer *jnc = data;
    if (lmi_job_get_number(job) == jnc->number) {
        jnc->job = job;
        return TRUE;
    }
    return FALSE;
}

LmiJob *jobmgr_get_job_by_number(guint number)
{
    struct _JobNumberContainer jnc = {number, NULL};
    g_assert(_initialized_counter);

    JM_CRITICAL_BEGIN;
    g_tree_foreach(_job_map, stop_search_for_job_by_number, &jnc);
    JM_CRITICAL_END;
    if (jnc.job)
        g_object_ref(jnc.job);

    return jnc.job;
}

/**
 * Traversing function for `_job_map` tree. It tries to find job by name and
 * assignes it to `_JobNameContainer`.
 */
static gboolean stop_search_for_job_by_name(gpointer key,
                                            gpointer value,
                                            gpointer data)
{
    LmiJob *job = LMI_JOB(value);
    struct _JobNameContainer *jnc = data;
    if (!g_strcmp0(lmi_job_get_name(job), jnc->name)) {
        jnc->job = job;
        return TRUE;
    }
    return FALSE;
}

LmiJob *jobmgr_get_job_by_name(const gchar *name)
{
    struct _JobNameContainer jnc = {name, NULL};
    g_assert(_initialized_counter);
    g_assert(name);

    JM_CRITICAL_BEGIN;
    g_tree_foreach(_job_map, stop_search_for_job_by_name, &jnc);
    JM_CRITICAL_END;
    if (jnc.job)
        g_object_ref(jnc.job);

    return jnc.job;
}

CMPIStatus jobmgr_job_to_cim_op(const LmiJob *job, CMPIObjectPath **op)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    gchar *namespace;
    char instance_id[BUFLEN];
    CMPIValue value;
    const JobTypeInfo *info;
    g_assert(op);
    g_assert(LMI_IS_JOB(job));
    g_assert(_initialized_counter > 0);

    if ((info = lookup_job_type_info_for_job(job)) == NULL) {
        CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_NOT_FOUND,
                "Job type is not registered with job manager.");
        goto done;
    }
    if ((namespace = lmi_read_config("CIM", "Namespace")) == NULL)
        goto err;
    *op = CMNewObjectPath(_cb, namespace, info->cim_class_name, &status);
    g_free(namespace);
    if (*op == NULL || status.rc)
        goto err;

    g_snprintf(instance_id, BUFLEN, "LMI:%s:%u", info->cim_class_name,
                    lmi_job_get_number(job));

    value.string = CMNewString(_cb, instance_id, &status);
    if (value.string == NULL || status.rc)
        goto op_err;

    if ((status = CMAddKey(*op, "InstanceID", &value, CMPI_string)).rc)
        goto string_err;

done:
    return status;

string_err:
    CMRelease(value.string);
op_err:
    CMRelease(*op);
err:
    lmi_error("Memory allocation failed");
    if (!status.rc)
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    return status;
}

CMPIStatus jobmgr_job_to_cim_instance(const LmiJob *job,
                                      CMPIInstance **instance)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    const JobTypeInfo *info;
    g_assert(instance);

    if ((info = lookup_job_type_info_for_job(job)) == NULL) {
        CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_NOT_FOUND,
                "Job type is not registered with job manager.");
        goto done;
    }

    JOB_CRITICAL_BEGIN(job);
    status = job_to_cim_instance(job, instance);
    if (!status.rc) {
        if (info->convert_func && (status = (*info->convert_func) (
                   _cb, _cmpi_ctx, job, *instance)).rc)
        {
            CMRelease(*instance);
            instance = NULL;
        }
    }
    JOB_CRITICAL_END(job);

done:
    return status;
}

CMPIStatus jobmgr_job_to_method_result_op(const LmiJob *job,
                                          const gchar *class_name,
                                          CMPIObjectPath **op)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    gchar *namespace = NULL;
    char instance_id[BUFLEN];
    CMPIValue value;
    const gchar *cim_class_name = class_name;
    gchar buf[BUFLEN];
    g_assert(op);
    g_assert(LMI_IS_JOB(job));
    g_assert(_initialized_counter > 0);

    if (class_name == NULL) {
        g_snprintf(buf, BUFLEN, "LMI_%sMethodResult", _profile_name);
        cim_class_name = buf;
    }

    if ((namespace = lmi_read_config("CIM", "Namespace")) == NULL)
        goto err;
    if ((*op = CMNewObjectPath(_cb, namespace, buf, &status)) == NULL)
        goto namespace_err;
    g_free(namespace);
    namespace = NULL;

    g_snprintf(instance_id, BUFLEN, "LMI:%s:%u", cim_class_name,
                    lmi_job_get_number(job));

    value.string = CMNewString(_cb, instance_id, &status);
    if (value.string == NULL || status.rc)
        goto op_err;

    if ((status = CMAddKey(*op, "InstanceID", &value, CMPI_string)).rc)
        goto string_err;

    return status;

string_err:
    CMRelease(value.string);
op_err:
    CMRelease(*op);
namespace_err:
    g_free(namespace);
err:
    lmi_error("Memory allocation failed");
    if (!status.rc)
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    return status;
}

CMPIStatus jobmgr_job_to_method_result_instance(const LmiJob *job,
                                                const gchar *class_name,
                                                CMPIInstance **instance)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIObjectPath *op;
    CMPIInstance *inst;
    CMPIValue value;
    CMPIData data;
    gchar buf[BUFLEN];
    g_assert(instance);
    g_assert(LMI_IS_JOB(job));
    g_assert(_initialized_counter > 0);

    JOB_CRITICAL_BEGIN(job);

    if (lmi_job_get_method_name(job) == NULL) {
        g_snprintf(buf, BUFLEN, "Can't create LMI_MethodResult instance"
                " out of job #%u with method-name unset.", lmi_job_get_number(job));
        goto err;
    }

    if ((status = jobmgr_job_to_method_result_op(job, class_name, &op)).rc)
        goto err;

    if ((inst = CMNewInstance(_cb, op, &status)) == NULL || status.rc)
        goto op_err;

    data = CMGetKey(op, "InstanceID", NULL);
    value.string = CMClone(data.value.string, &status);
    if (value.string == NULL || status.rc)
        goto inst_err;
    if ((status = CMSetProperty(inst, "InstanceID", &value, CMPI_string)).rc)
        goto string_err;

    g_snprintf(buf, BUFLEN, "Result of method %s",
            lmi_job_get_method_name(job));
    if ((value.string = CMNewString(_cb, buf, &status)) == NULL)
        goto inst_err;
    if ((status = CMSetProperty(inst, "Caption", &value, CMPI_string)).rc)
        goto string_err;

    g_snprintf(buf, BUFLEN, "Result of asynchronous job #%d created"
            " as a result of \"%s\" method's invocation.",
            lmi_job_get_number(job), lmi_job_get_method_name(job));
    if ((value.string = CMNewString(_cb, buf, &status)) == NULL)
        goto inst_err;
    if ((status = CMSetProperty(inst, "Description", &value, CMPI_string)).rc)
        goto string_err;

    g_snprintf(buf, BUFLEN, "MethodResult-%u", lmi_job_get_number(job));
    if ((value.string = CMNewString(_cb, buf, &status)) == NULL)
        goto inst_err;
    if ((status = CMSetProperty(inst, "ElementName", &value, CMPI_string)).rc)
        goto string_err;

    if ((status = make_inst_method_call_instance_for_job(
                job, TRUE, &value.inst)).rc)
        goto inst_err;
    if ((status = CMSetProperty(inst, "PreCallIndication",
            &value, CMPI_instance)).rc)
    {
        CMRelease(value.inst);
        goto inst_err;
    }

    if (lmi_job_is_finished(job)) {
        if ((status = make_inst_method_call_instance_for_job(
                    job, FALSE, &value.inst)).rc)
            goto inst_err;
        if ((status = CMSetProperty(inst, "PostCallIndication",
                &value, CMPI_instance)).rc)
        {
            CMRelease(value.inst);
            goto inst_err;
        }
    }

    JOB_CRITICAL_END(job);

    *instance = inst;
    return status;

string_err:
    CMRelease(value.string);
inst_err:
    CMRelease(inst);
    *instance = NULL;
    goto err;
op_err:
    CMRelease(op);
err:
    if (!status.rc) {
        lmi_error("Memory allocation failed");
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    }
    JOB_CRITICAL_END(job);
    return status;
}

CMPIStatus jobmgr_job_to_cim_error(const LmiJob *job,
                                   CMPIInstance **instance)
{
    gchar *namespace;
    CMPIObjectPath *op = NULL;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIValue value;
    g_assert(instance != NULL);

    JOB_CRITICAL_BEGIN(job);
    if (lmi_job_get_state(job) != LMI_JOB_STATE_ENUM_EXCEPTION) {
        /* make the CIMError instance only for failed job */
        JOB_CRITICAL_END(job);
        return status;
    }

    if ((namespace = lmi_read_config("CIM", "Namespace")) == NULL) {
        lmi_error("Memory allocation failed");
        goto err;
    }

    op = CMNewObjectPath(_cb, namespace, CIM_ERROR_CLASS_NAME, &status);
    if (op == NULL || status.rc)
        goto namespace_err;
    g_free(namespace);
    namespace = NULL;

    if ((*instance = CMNewInstance(_cb, op, &status)) == NULL || status.rc)
        goto op_err;
    op = NULL;


    value.uint32 = lmi_job_get_status_code(job);
    if ((status = CMSetProperty(*instance, "CIMStatusCode",
                    &value, CMPI_uint32)).rc)
        goto instance_err;

    if ((status = jobmgr_job_to_cim_op(job, &op)).rc)
        goto instance_err;
    if ((value.string = CMObjectPathToString(op, &status)) == NULL)
        goto instance_err;
    CMRelease(op);
    op = NULL;
    if ((status = CMSetProperty(*instance, "ErrorSource",
                    &value, CMPI_string)).rc)
        goto string_err;

    value.uint16 = CIM_ERROR_SOURCE_FORMAT_OBJECT_PATH;
    if ((status = CMSetProperty(*instance, "ErrorSourceFormat",
                    &value, CMPI_uint16)).rc)
        goto instance_err;

    value.uint16 = lmi_job_get_error_type(job);
    if ((status = CMSetProperty(*instance, "ErrorType",
                    &value, CMPI_uint16)).rc)
        goto instance_err;

    if (lmi_job_get_error(job) != NULL) {
        if ((value.string = CMNewString(
                        _cb, lmi_job_get_error(job), &status)) == NULL)
            goto string_err;
        if ((status = CMSetProperty(*instance, "Message",
                        &value, CMPI_string)).rc)
            goto string_err;
    }

    JOB_CRITICAL_END(job);

    return status;

string_err:
    CMRelease(value.string);
instance_err:
    CMRelease(*instance);
    *instance = NULL;
    JOB_CRITICAL_END(job);
op_err:
    if (op)
        CMRelease(op);
namespace_err:
    g_free(namespace);
err:
    if (!status.rc) {
        lmi_error("Memory allocation failed");
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    }
    return status;
}

/**
 * Traversing function for `_event_calendar` tree.
 */
static gboolean find_event_by_job(gpointer key,
                                  gpointer value,
                                  gpointer data)
{
    struct _CalendarEventContainer *cec = data;
    CalendarEvent *ev = key;
    if (ev->job == cec->job) {
        cec->matches = g_list_append(cec->matches, ev);
    }
    return FALSE;
}

static void find_and_delete_calendar_event(gpointer item,
                                           gpointer unused)
{
    CalendarEvent *ev = item;
    g_tree_remove(_event_calendar, ev);
}

CMPIStatus jobmgr_delete_job(LmiJob *job)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    g_assert(_initialized_counter);

    JM_CRITICAL_BEGIN;
    status = delete_job(job);
    JM_CRITICAL_END;

    return status;
}

CMPIStatus jobmgr_terminate_job(LmiJob *job)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    const JobTypeInfo *info;
    GTask *task;
    GCancellable *cancellable;
    gchar *jobid = NULL;
    char err_buf[BUFLEN] = {0};
    g_assert(_initialized_counter);

    if ((info = lookup_job_type_info_for_job(job)) == NULL) {
        CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_NOT_FOUND,
                "Can't terminate unregistered job!");
        goto err;
    }

    if ((jobid = lmi_job_get_jobid(job)) == NULL)
        goto memory_err;

    JM_CRITICAL_BEGIN;
    JOB_CRITICAL_BEGIN(job);
    if (lmi_job_is_finished(job)) {
        snprintf(err_buf, BUFLEN,
                "Can't terminate finished job \"%s\"!", jobid);
        CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_FAILED, err_buf);
        goto release_lock_err;
    }
    if (!info->running_job_cancellable &&
            lmi_job_get_state(job) == LMI_JOB_STATE_ENUM_RUNNING)
    {
        snprintf(err_buf, BUFLEN,
                "Can't terminate running job \"%s\"!", jobid);
        CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_NOT_SUPPORTED, err_buf);
        goto release_lock_err;
    }

    if ((task = g_tree_lookup(_running_jobs, job)) == NULL) {
        /* job has not been started yet */
        if (!lmi_job_finish_terminate(job)) {
            snprintf(err_buf, BUFLEN,
                "Failed to terminate job \"%s\"!", jobid);
            CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_FAILED, err_buf);
            goto release_lock_err;
        }
        lmi_info("Job #%u (id=%s) terminated successfully.",
                lmi_job_get_number(job), jobid);
    } else {
        lmi_debug("Cancelling job \"%s\".", jobid);
        cancellable = g_task_get_cancellable(task);
        g_cancellable_cancel(cancellable);
    }

    JOB_CRITICAL_END(job);
    JM_CRITICAL_END;

    lmi_job_wait_until_finished(job);
    if (lmi_job_get_state(job) != LMI_JOB_STATE_ENUM_TERMINATED) {
        snprintf(err_buf, BUFLEN,
            "Failed to terminate job \"%s\" which is now %s!", jobid,
            lmi_job_get_state(job) == LMI_JOB_STATE_ENUM_COMPLETED ?
            "completed" : "completed with exception");
        CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_FAILED, err_buf);
        goto release_lock_err;
    }

    g_free(jobid);
    return status;

release_lock_err:
    JOB_CRITICAL_END(job);
    JM_CRITICAL_END;
memory_err:
    if (err_buf[0]) {
        lmi_error(err_buf);
    } else {
        lmi_error("Memory allocation failed");
    }
    if (!status.rc)
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    g_free(jobid);
err:
    return status;
}

struct _JobTypeSearchContainer {
    const gchar *cim_class_name;
    GType job_type;
};

static gboolean find_job_type_for_cim_class_name(gpointer key,
                                                 gpointer value,
                                                 gpointer data)
{
    GType job_type = (GType) ((uintptr_t) key);
    const JobTypeInfo *info = value;
    struct _JobTypeSearchContainer *jtsc = data;

    if (!g_strcmp0(info->cim_class_name, jtsc->cim_class_name)) {
        jtsc->job_type = job_type;
        return TRUE;
    }
    return FALSE;
}

CMPIStatus jobmgr_get_job_matching_op(const CMPIObjectPath *path,
                                      LmiJob **job)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIData data;
    const char *instance_id;
    const char *class_name_start;
    const char *class_name_end;
    guint64 number;
    gchar *endptr;
    LmiJob *jobptr = NULL;
    struct _JobTypeSearchContainer jtsc = {NULL, G_TYPE_INVALID};
    const JobTypeInfo *info;
    gchar error[BUFLEN] = "";
    g_assert(_initialized_counter);

    data = CMGetKey(path, "InstanceID", &status);
    if (status.rc)
        return status;
    if (data.state != (CMPI_keyValue | CMPI_goodValue) ||
            data.type != CMPI_string)
    {
        g_snprintf(error, BUFLEN, "InstanceID must be non-empty string.");
        goto err;
    }

    instance_id = CMGetCharsPtr(data.value.string, NULL);
    if (g_ascii_strncasecmp(LMI_ORGID ":", instance_id, strlen(LMI_ORGID) + 1)) {
        g_snprintf(error, BUFLEN,
                "Invalid InstanceID \"%s\" of job object path.", instance_id);
        goto err;
    }
    class_name_start = instance_id + strlen(LMI_ORGID) + 1;
    class_name_end = index(class_name_start, ':');
    if (class_name_end == NULL) {
        g_snprintf(error, BUFLEN,
                "Invalid InstanceID \"%s\" of job object path"
                " (expected ':' after class name).", instance_id);
        goto err;
    }
    if ((jtsc.cim_class_name = g_strndup(class_name_start,
                    class_name_end - class_name_start)) == NULL)
    {
        lmi_error("Memory allocation failed.");
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
        goto err;
    }

    g_tree_foreach(_job_type_map, find_job_type_for_cim_class_name, &jtsc);

    if (jtsc.job_type == G_TYPE_INVALID) {
        lmi_error("Class name \'%s\" not found in %u items!",
                jtsc.cim_class_name, g_tree_nnodes(_job_type_map));
        g_snprintf(error, BUFLEN,
                "Class name \"%s\" in job's InstanceID does not belong"
                " to any known job type.", jtsc.cim_class_name);
        goto err;
    }

    number = g_ascii_strtoull(class_name_end + 1, &endptr, 10);
    if (number == G_MAXUINT64 || number == 0 || number > (guint64) G_MAXINT32) {
        g_snprintf(error, BUFLEN,
                "Missing or invalid job number in job's InstanceID"
                " \"%s\".", instance_id);
        goto err;
    }

    if (*endptr != '\0') {
        g_snprintf(error, BUFLEN,
                "Junk after job number in job's InstanceID \"%s\".",
                instance_id);
        goto err;
    }

    if ((jobptr = jobmgr_get_job_by_number(number)) == NULL) {
        g_snprintf(error, BUFLEN, "Job #%u does not exist.", (guint32) number);
        goto err;
    }

    info = lookup_job_type_info_for_job(jobptr);
    if (g_ascii_strcasecmp(info->cim_class_name, jtsc.cim_class_name)) {
        g_snprintf(error, BUFLEN,
                "No job matching given InstanceID \"%s\".", instance_id);
        goto err;
    }
    g_free((gchar *) jtsc.cim_class_name);

    if (job) {
        *job = jobptr;
    } else {
        g_object_unref(jobptr);
    }

    return status;

err:
    g_clear_object(&jobptr);
    g_free((gchar *) jtsc.cim_class_name);
    if (!status.rc) {
        if (error[0]) {
            CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_NOT_FOUND, error);
        } else {
            CMSetStatus(&status, CMPI_RC_ERR_NOT_FOUND);
        }
    }
    if (error[0])
        lmi_warn(error);
    return status;
}
