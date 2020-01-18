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

#include <string.h>
#include <time.h>
#include <pthread.h>
#include <json-glib/json-glib.h>
#include "openlmi.h"
#include "openlmi-utils.h"
#include "lmi_job.h"

#define DEFAULT_PRIORITY 128
#define DEFAULT_TIME_BEFORE_REMOVAL 5 * 60
#define MINIMUM_TIME_BEFORE_REMOVAL 10
#define DEFAULT_DELETE_ON_COMPLETION TRUE
#define IS_FINAL_STATE(state) \
           (  state == LMI_JOB_STATE_ENUM_COMPLETED \
           || state == LMI_JOB_STATE_ENUM_TERMINATED \
           || state == LMI_JOB_STATE_ENUM_EXCEPTION)

/* Forward declarations */
static void lmi_job_finalize(GObject *object);
static void lmi_job_serializable_iface_init(JsonSerializableIface *iface);

#define LMI_JOB_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), LMI_TYPE_JOB, LmiJobPrivate))

#define PRIV_CRITICAL_BEGIN(priv) \
    { \
        pthread_t _thread_id = pthread_self(); \
        _clog(CLOG_COLOR_GREEN, "[tid=%lu] locking job %u", \
                _thread_id, priv->number); \
        pthread_mutex_lock(&priv->lock); \
        _clog(CLOG_COLOR_GREEN, "[tid=%lu] locked job %u", \
                _thread_id, priv->number); \
    }

#define PRIV_CRITICAL_END(priv) \
    { \
        pthread_t _thread_id = pthread_self(); \
        _clog(CLOG_COLOR_GREEN, "[tid=%lu] unlocking job %u", \
                _thread_id, priv->number); \
        pthread_mutex_unlock(&priv->lock); \
        _clog(CLOG_COLOR_GREEN, "[tid=%lu] unlocked job %u", \
                _thread_id, priv->number); \
    }

/**
 * Publicly accessible properties.
 */
enum _PublicProperties {
    PROP_0,
    PROP_NUMBER,
                /* !< Constant, unique, typed as uint32 accessible as "number".
                 */
    PROP_NAME,  /*!< String property initially set to `NULL`. It's modifiable
                 * by cim client. Accessible as "name". */
    PROP_JOBID, /*!< String property initially set to `NULL`. Job processing
                 * function should set it to value identifying job in backend.
                 * Note that `lmi_job_get_jobid()` never returns `NULL` unless
                 * memory error occurs. Job number is returned as a fallback
                 * value. Accessible as "jobid". */
    PROP_STATE, /*!< State of job typed as `LmiJobStateEnum`. Accessible as
                 * "state". */
    PROP_PRIORITY,
                /*!< Priority of job typed as uint32. Modifiable by client.
                 * Defaults to `DEFAULT_PRIORITY`. Accessible as "priority". */
    PROP_METHOD_NAME,
                /*!< String property holding a name of asynchronous method that
                 * created the job. Accessible as "method-name". */
    PROP_TIME_SUBMITTED,
                /*!< Time of job's creation as a number of seconds since the
                 * Epoch. Accessible as "time-submitted". */
    PROP_TIME_BEFORE_REMOVAL,
                /*!< Number of seconds the job is kept alive after its
                 * completion. Typed as int64. Accessible as
                 * "time-before-removal". */
    PROP_TIME_OF_LAST_STATE_CHANGE,
                /*!< Time of job's last state change as a number of seconds
                 * since the Epoch. Accessible as "time-of-last-state-change".
                 */
    PROP_START_TIME,
                /*!< Time the job was started as a number of seconds since the
                 * Epoch. Accessible as "start-time". */
    PROP_DELETE_ON_COMPLETION,
                /*!< Boolean property saying whether the job shall be
                 * automatically deleted after its completion. Job is not
                 * deleted immediatelly. "time-before-removal" can be used to
                 * modify a time interval. Accessible as
                 * "delete-on-completion". */
    PROP_PERCENT_COMPLETE,
                /*!< Uint32 property ranging from 0 to 100. Accessible as
                 * "percent-complete". */
    PROP_STATUS_CODE,
                /*!< CIM status code property set upon error. It's set together
                 * with state *EXCEPTION*. Accessible as "status-code".
                 * Defaults to `CIM_ERR_FAILED` for job in `EXCEPTION` state.
                 * Please refer to `CIM_Error.CIMStatusCode` description in
                 * CIM schema.  */
    PROP_RESULT,
                /*!< Result of operation. Typed as `GVariant` without any
                 * limitation to type it can hold. Accessible as "result".
                 * Defaults to `NULL` which is represented by GVariant of type
                 * `G_VARIANT_TYPE_HANDLE` with value 0 when using getter and
                 * setter of gobject framework. `lmi_job_get_result()` returns
                 * `NULL` instead. This property can only be set together with
                 * state *COMPLETED*. */
    PROP_ERROR_TYPE,
                /*!< Uint32 property holding error type. Accessible as
                 * "error-type". It defaults to `Unknown` (0). Please refer to
                 * `CIM_Error.ErrorType` description in CIM schema for more
                 * information. */
    PROP_ERROR, /*!< String property holding an error message. Accessible as
                 * "error". It can only be set together with state *EXCEPTION*.
                 */
    PROP_LAST,
};

#define PROP_IN_PARAMETERS  (PROP_LAST + 0)
#define PROP_OUT_PARAMETERS (PROP_LAST + 1)

/**
 * Privately accessible properties.
 * These are meant to be accessed only by json serializer.
 */
enum _PrivateProperties {
    PROP_PRIVATE_IN_PARAMETERS,
                /*!< Private input properties of asynchronous method that
                 * created particular job. They are stored in hash table with
                 * name of property as a key and `GVariant` as a value.
                 * Accessible as "in-parameters" only through
                 * `JsonSerializableIface` interface. */
    PROP_PRIVATE_OUT_PARAMETERS,
                /*! Private output properties. Accessible as "out-parameters". */
    PROP_PRIVATE_LAST,
};

#define PROP_IN_PARAMETERS_NAME \
    s_prop_private_names[PROP_PRIVATE_IN_PARAMETERS]
#define PROP_OUT_PARAMETERS_NAME \
    s_prop_private_names[PROP_PRIVATE_OUT_PARAMETERS]

enum {
    SIGNAL_MODIFIED,
    SIGNAL_STATE_CHANGED,
    SIGNAL_FINISHED,
    SIGNAL_PRIORITY_CHANGED,
    SIGNAL_DELETION_REQUEST_CHANGED,
    SIGNAL_LAST
};

/* Used to deserialize result property. First string is a variant type, second
 * is its value serialized with `g_variant_print`. */
#define PARAM_VARIANT_TYPE_STR "{ss}"
/* This is an dictionary entry where key is a name of input/output parameter
 * and value is another dictionary entry matching `PARAM_VARIANT_TYPE_STR` above. */
#define PARAMS_ENTRY_VARIANT_TYPE_STR "{sv}"
/* This is variant type used for serialization of input/output parameters. */
#define PARAMS_VARIANT_TYPE_STR ("a" PARAMS_ENTRY_VARIANT_TYPE_STR)
#define PARAMS_VARIANT_TYPE \
    ((const GVariantType *) PARAMS_VARIANT_TYPE_STR)

/**
 * Array of job signals.
 */
static guint _signals[SIGNAL_LAST] = { 0 };
/**
 * Guard for `_last_job_number`.
 */
static pthread_mutex_t _job_number_lock = PTHREAD_MUTEX_INITIALIZER;
/**
 * Number of newest job created. It is automatically incremented after each
 * newly created job. 0 is not a valid job number. This variable shall be
 * accessed only in critical section guarded with `_last_job_number`.
 */
static guint _last_job_number = 0;
/**
 * Storage for private properties which are not stored in LmiJobClass.
 */
static GParamSpec *_private_prop_specs[PROP_PRIVATE_LAST] = { NULL };

struct _LmiJobPrivate {
    pthread_mutex_t lock;
    guint number;
    gchar *name;
    gchar *jobid;
    LmiJobStateEnum state;
    guint priority;
    gchar *method_name;
    time_t time_submitted;
    time_t time_before_removal;
    time_t time_of_last_state_change;
    time_t start_time;
    gboolean delete_on_completion;
    GHashTable *in_parameters;
    GHashTable *out_parameters;
    guint percent_complete;
    GVariant *result;
    LmiJobStatusCodeEnum status_code;
    LmiJobErrorTypeEnum error_type;
    gchar *error;
    gpointer data;
    GDestroyNotify data_destroy;
};

/**
 * Define LMI_TYPE_JOB implementing JsonSerializableIface interface.
 */
G_DEFINE_TYPE_WITH_CODE(LmiJob, lmi_job, G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE(JSON_TYPE_SERIALIZABLE,
            lmi_job_serializable_iface_init));

static const gchar *s_job_state_enum_names[] = {
    "new", "running", "completed", "terminated", "exception" };
static const gchar *s_job_prop_enum_names[] = {
    "name", "jobid", "state", "priority", "method-name",
    "time-before-removal", "time-of-last-state-change", "start-time",
    "delete-on-completion", "percent-complete", "status-code",
    "result", "error-type", "error" };
static const gchar *s_prop_private_names[] = {
    "in-parameters", "out-parameters" };

static void lmi_job_get_property(GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
    LmiJob *job = LMI_JOB(object);
    LmiJobPrivate *priv = job->priv;
    GVariant *tmp;

    switch (prop_id) {
        case PROP_NUMBER:
            g_value_set_uint(value, priv->number);
            break;
        case PROP_NAME:
            g_value_set_string(value, priv->name);
            break;
        case PROP_JOBID:
            g_value_set_string(value, priv->jobid);
            break;
        case PROP_STATE:
            g_value_set_uint(value, priv->state);
            break;
        case PROP_PRIORITY:
            g_value_set_uint(value, priv->priority);
            break;
        case PROP_METHOD_NAME:
            g_value_set_string(value, priv->method_name);
            break;
        case PROP_TIME_SUBMITTED:
            g_value_set_int64(value, priv->time_submitted);
            break;
        case PROP_TIME_BEFORE_REMOVAL:
            g_value_set_int64(value, priv->time_before_removal);
            break;
        case PROP_TIME_OF_LAST_STATE_CHANGE:
            g_value_set_int64(value, priv->time_of_last_state_change);
            break;
        case PROP_START_TIME:
            g_value_set_int64(value, priv->start_time);
            break;
        case PROP_DELETE_ON_COMPLETION:
            g_value_set_boolean(value, priv->delete_on_completion);
            break;
        case PROP_PERCENT_COMPLETE:
            g_value_set_uint(value, priv->percent_complete);
            break;
        case PROP_STATUS_CODE:
            g_value_set_uint(value, priv->status_code);
            break;
        case PROP_RESULT:
            if (priv->result) {
                tmp = priv->result;
            } else {
                tmp = g_variant_new_handle(0);
            }
            g_value_set_variant(value, tmp);
            break;
        case PROP_ERROR_TYPE:
            g_value_set_uint(value, priv->error_type);
            break;
        case PROP_ERROR:
            g_value_set_string(value, priv->error);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void lmi_job_set_property(GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
    LmiJob *job = LMI_JOB(object);
    LmiJobPrivate *priv = job->priv;
    GVariant *variant;
    GValue defval = G_VALUE_INIT;

    PRIV_CRITICAL_BEGIN(priv);
    switch (prop_id) {
        case PROP_NUMBER:
            priv->number = MAX(priv->number, g_value_get_uint(value));
            pthread_mutex_lock(&_job_number_lock);
            if (priv->number > _last_job_number)
            {
                _last_job_number = priv->number;
            }
            pthread_mutex_unlock(&_job_number_lock);
            break;
        case PROP_NAME:
            lmi_job_set_name(job, g_value_get_string(value));
            break;
        case PROP_JOBID:
            lmi_job_set_jobid(job, g_value_get_string(value));
            break;
        case PROP_STATE:
            priv->state = g_value_get_uint(value);
            break;
        case PROP_PRIORITY:
            lmi_job_set_priority(job, g_value_get_uint(value));
            break;
        case PROP_METHOD_NAME:
            lmi_job_set_method_name(job, g_value_get_string(value));
            break;
        case PROP_TIME_SUBMITTED:
            g_value_init(&defval, G_TYPE_INT64);
            g_value_copy(value, &defval);
            if (!g_param_value_defaults(pspec, &defval))
                priv->time_submitted = g_value_get_int64(value);
            break;
        case PROP_TIME_BEFORE_REMOVAL:
            lmi_job_set_time_before_removal(job, g_value_get_int64(value));
            break;
        case PROP_TIME_OF_LAST_STATE_CHANGE:
            g_value_init(&defval, G_TYPE_INT64);
            g_value_copy(value, &defval);
            if (!g_param_value_defaults(pspec, &defval))
                priv->time_of_last_state_change = g_value_get_int64(value);
            break;
        case PROP_START_TIME:
            g_value_init(&defval, G_TYPE_INT64);
            g_value_copy(value, &defval);
            if (!g_param_value_defaults(pspec, &defval))
                priv->start_time = g_value_get_int64(value);
            break;
        case PROP_DELETE_ON_COMPLETION:
            lmi_job_set_delete_on_completion(job, g_value_get_boolean(value));
            break;
        case PROP_PERCENT_COMPLETE:
            lmi_job_set_percent_complete(job, g_value_get_uint(value));
            break;
        case PROP_STATUS_CODE:
            priv->status_code = g_value_get_uint(value);
            break;
        case PROP_RESULT:
            variant = g_value_get_variant(value);
            if (priv->result != variant) {
                if (priv->result) {
                    g_variant_unref(priv->result);
                }
                if ((g_variant_is_of_type(variant, G_VARIANT_TYPE_HANDLE) &&
                     g_variant_get_handle(variant) == 0) || variant == NULL)
                {
                    priv->result = NULL;
                } else {
                    priv->result = g_variant_ref(variant);
                }
            }
            break;
        case PROP_ERROR_TYPE:
            lmi_job_set_error_type(job, g_value_get_uint(value));
            break;
        case PROP_ERROR:
            priv->error = g_strdup(g_value_get_string(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    PRIV_CRITICAL_END(priv);
}

/**
 * Marshaller used for `modified()` signal.
 */
static void g_cclosure_marshal_VOID__UINT_VARIANT_VARIANT(
    GClosure     *closure,
    GValue       *return_value G_GNUC_UNUSED,
    guint         n_param_values,
    const GValue *param_values,
    gpointer      invocation_hint G_GNUC_UNUSED,
    gpointer      marshal_data)
{
    typedef void (*GMarshalFunc_VOID__UINT_VARIANT_VARIANT) (
            gpointer     data1,
            guint        arg_1,
            GVariant    *arg_2,
            GVariant    *arg_3,
            gpointer     data2);
    register GMarshalFunc_VOID__UINT_VARIANT_VARIANT callback;
    register GCClosure *cc = (GCClosure*) closure;
    register gpointer data1, data2;

    g_return_if_fail (n_param_values == 4);

    if (G_CCLOSURE_SWAP_DATA (closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    } else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__UINT_VARIANT_VARIANT) (
            marshal_data ? marshal_data : cc->callback);

    callback(
            data1,
            g_value_get_uint(param_values + 1),
            g_value_get_variant(param_values + 2),
            g_value_get_variant(param_values + 3),
            data2);
}

/**
 * Marshaller used for `priority-changed()` and `state-changed()` signals.
 */
static void g_cclosure_marshal_VOID__UINT_UINT(
    GClosure     *closure,
    GValue       *return_value G_GNUC_UNUSED,
    guint         n_param_values,
    const GValue *param_values,
    gpointer      invocation_hint G_GNUC_UNUSED,
    gpointer      marshal_data)
{
    typedef void (*GMarshalFunc_VOID__UINT_UINT) (
            gpointer     data1,
            guint        arg_1,
            guint        arg_2,
            gpointer     data2);
    register GMarshalFunc_VOID__UINT_UINT callback;
    register GCClosure *cc = (GCClosure*) closure;
    register gpointer data1, data2;

    g_return_if_fail (n_param_values == 3);

    if (G_CCLOSURE_SWAP_DATA (closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    } else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__UINT_UINT) (
            marshal_data ? marshal_data : cc->callback);

    callback(
            data1,
            g_value_get_uint(param_values + 1),
            g_value_get_uint(param_values + 2),
            data2);
}

/**
 * Marshaller used for `finished()` signal.
 */
static void g_cclosure_marshal_VOID__UINT_UINT_VARIANT_STRING(
    GClosure     *closure,
    GValue       *return_value G_GNUC_UNUSED,
    guint         n_param_values,
    const GValue *param_values,
    gpointer      invocation_hint G_GNUC_UNUSED,
    gpointer      marshal_data)
{
    typedef void (*GMarshalFunc_VOID__UINT_UINT_VARIANT_STRING) (
            gpointer     data1,
            guint        arg_1,
            guint        arg_2,
            GVariant    *arg_4,
            const gchar *arg_5,
            gpointer     data2);
    register GMarshalFunc_VOID__UINT_UINT_VARIANT_STRING callback;
    register GCClosure *cc = (GCClosure*) closure;
    register gpointer data1, data2;

    g_return_if_fail (n_param_values == 5);

    if (G_CCLOSURE_SWAP_DATA (closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    } else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__UINT_UINT_VARIANT_STRING) (
            marshal_data ? marshal_data : cc->callback);

    callback(
            data1,
            g_value_get_uint(param_values + 1),
            g_value_get_uint(param_values + 2),
            g_value_get_variant(param_values + 3),
            g_value_get_string(param_values + 4),
            data2);
}

/**
 * Marshaller used for `deletion-request-changed()` signal.
 */
static void g_cclosure_marshal_VOID__BOOLEAN_INT64(
    GClosure     *closure,
    GValue       *return_value G_GNUC_UNUSED,
    guint         n_param_values,
    const GValue *param_values,
    gpointer      invocation_hint G_GNUC_UNUSED,
    gpointer      marshal_data)
{
    typedef void (*GMarshalFunc_VOID__BOOLEAN_INT64) (
            gpointer     data1,
            gboolean     arg_1,
            gint64       arg_2,
            gpointer     data2);
    register GMarshalFunc_VOID__BOOLEAN_INT64 callback;
    register GCClosure *cc = (GCClosure*) closure;
    register gpointer data1, data2;

    g_return_if_fail (n_param_values == 3);

    if (G_CCLOSURE_SWAP_DATA (closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    } else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__BOOLEAN_INT64) (
            marshal_data ? marshal_data : cc->callback);

    callback(
            data1,
            g_value_get_boolean(param_values + 1),
            g_value_get_int64(param_values + 2),
            data2);
}

static void lmi_job_class_init(LmiJobClass *klass)
{
    GParamSpec *pspec;
    g_type_class_add_private(klass, sizeof(LmiJobPrivate));
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->get_property = lmi_job_get_property;
    gobject_class->set_property = lmi_job_set_property;
    gobject_class->finalize = lmi_job_finalize;

    pspec = g_param_spec_uint("number",
            NULL, "Sequential number of job. Guaranteed to be unique.",
            0, G_MAXUINT, 1,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_NUMBER, pspec);

    pspec = g_param_spec_string(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_NAME],
            NULL, "Modifiable job name.", NULL,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_NAME, pspec);

    pspec = g_param_spec_string(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_JOBID],
            NULL, "Job identification string. Must be unique.",
            NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_JOBID, pspec);

    pspec = g_param_spec_uint(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_STATE],
            NULL, "Job state.",
            LMI_JOB_STATE_ENUM_NEW, LMI_JOB_STATE_ENUM_LAST - 1,
            LMI_JOB_STATE_ENUM_NEW,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_STATE, pspec);

    pspec = g_param_spec_uint(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_PRIORITY],
            NULL, "The urgency of execution of the Job."
           " The lower the number, the higher the priority.",
           0, G_MAXUINT, DEFAULT_PRIORITY,
           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_PRIORITY, pspec);

    pspec = g_param_spec_string(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_METHOD_NAME],
            NULL, "If not NULL, the name of the intrinsic operation or "
           "extrinsic method for which this Job represents an invocation.",
            NULL,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_METHOD_NAME, pspec);

    pspec = g_param_spec_int64("time-submitted",
            NULL, "Time of job's creation.",
            G_MININT64, G_MAXINT64, 0,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_TIME_SUBMITTED, pspec);

    pspec = g_param_spec_int64(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_TIME_BEFORE_REMOVAL],
            NULL, "The amount of time in seconds that the Job is retained after"
           " it has finished executing.",
            0, G_MAXINT64, DEFAULT_TIME_BEFORE_REMOVAL,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class,
            PROP_TIME_BEFORE_REMOVAL, pspec);

    pspec = g_param_spec_int64(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_TIME_OF_LAST_STATE_CHANGE],
            NULL, "The time when the state of the Job last changed.",
            G_MININT64, G_MAXINT64, 0,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class,
            PROP_TIME_OF_LAST_STATE_CHANGE, pspec);

    pspec = g_param_spec_int64(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_START_TIME],
            NULL, "The time that the Job was actually started.",
            G_MININT64, G_MAXINT64, 0,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class,
            PROP_START_TIME, pspec);

    pspec = g_param_spec_boolean(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_DELETE_ON_COMPLETION],
            NULL, "Indicates whether or not the job should be automatically"
            " deleted upon completion.",
            DEFAULT_DELETE_ON_COMPLETION,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class,
            PROP_DELETE_ON_COMPLETION, pspec);

    pspec = g_param_spec_uint(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_PERCENT_COMPLETE],
            NULL, "The percentage of the job that has completed at the time"
            " that this value is requested.",
            0, G_MAXUINT, 0,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class,
            PROP_PERCENT_COMPLETE, pspec);

    pspec = g_param_spec_uint(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_STATUS_CODE],
            NULL, "CIM status code of completed job.",
            LMI_JOB_STATUS_CODE_ENUM_OK, LMI_JOB_STATUS_CODE_ENUM_LAST - 1,
            LMI_JOB_STATUS_CODE_ENUM_OK,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_STATUS_CODE, pspec);

    pspec = g_param_spec_variant(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_RESULT],
            NULL, "Result of invoked asynchronous method upon successful"
            " execution.",
            G_VARIANT_TYPE_ANY, NULL,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_RESULT, pspec);

    pspec = g_param_spec_uint(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_ERROR_TYPE],
            NULL, "Error type corresponding to CIM_Error.ErrorType.",
            LMI_JOB_ERROR_TYPE_ENUM_UNKNOWN, LMI_JOB_ERROR_TYPE_ENUM_LAST - 1,
            LMI_JOB_ERROR_TYPE_ENUM_UNKNOWN,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_ERROR_TYPE, pspec);

    pspec = g_param_spec_string(
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_ERROR],
            NULL, "Error description filled upon failure.", NULL,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_ERROR, pspec);

    _signals[SIGNAL_MODIFIED] =
            g_signal_new("modified",
                    G_TYPE_FROM_CLASS(gobject_class), G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET(LmiJobClass, modified),
                    NULL, NULL, g_cclosure_marshal_VOID__UINT_VARIANT_VARIANT,
                    G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_VARIANT, G_TYPE_VARIANT);

    _signals[SIGNAL_STATE_CHANGED] =
            g_signal_new("state-changed",
                    G_TYPE_FROM_CLASS(gobject_class), G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET(LmiJobClass, state_changed),
                    NULL, NULL, g_cclosure_marshal_VOID__UINT_UINT,
                    G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

    _signals[SIGNAL_FINISHED] =
            g_signal_new("finished",
                    G_TYPE_FROM_CLASS(gobject_class), G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET(LmiJobClass, finished),
                    NULL, NULL, g_cclosure_marshal_VOID__UINT_UINT_VARIANT_STRING,
                    G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT,
                    G_TYPE_VARIANT, G_TYPE_STRING);

    _signals[SIGNAL_PRIORITY_CHANGED] =
            g_signal_new("priority-changed",
                    G_TYPE_FROM_CLASS(gobject_class), G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET(LmiJobClass, priority_changed),
                    NULL, NULL, g_cclosure_marshal_VOID__UINT_UINT,
                    G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

    _signals[SIGNAL_DELETION_REQUEST_CHANGED] =
            g_signal_new("deletion-request-changed",
                    G_TYPE_FROM_CLASS(gobject_class), G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET(LmiJobClass, deletion_request_changed),
                    NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN_INT64,
                    G_TYPE_NONE, 2, G_TYPE_BOOLEAN, G_TYPE_INT64);

}

/**
 * Generator of unique job numbers. Each call increments `_last_job_number`.
 */
static guint lmi_job_generate_number()
{
    guint res;
    pthread_mutex_lock(&_job_number_lock);
    res = ++_last_job_number;
    pthread_mutex_unlock(&_job_number_lock);
    return res;
}

static void lmi_job_init(LmiJob *self)
{
    LmiJobPrivate *priv = LMI_JOB_GET_PRIVATE(self);
    pthread_mutexattr_t lock_attr;
    self->priv = priv;

    pthread_mutexattr_init(&lock_attr);
    pthread_mutexattr_settype(&lock_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&priv->lock, &lock_attr);
    pthread_mutexattr_destroy(&lock_attr);
    priv->number = lmi_job_generate_number();
    priv->name = NULL;
    priv->jobid = NULL;
    priv->state = LMI_JOB_STATE_ENUM_NEW;
    priv->priority = DEFAULT_PRIORITY;
    priv->method_name = NULL;
    priv->time_submitted = time(NULL);
    priv->time_before_removal = DEFAULT_TIME_BEFORE_REMOVAL;
    priv->time_of_last_state_change = priv->time_submitted;
    priv->start_time = -1;
    priv->delete_on_completion = DEFAULT_DELETE_ON_COMPLETION;
    priv->in_parameters = NULL;
    priv->out_parameters = NULL;
    priv->percent_complete = 0;
    priv->status_code = LMI_JOB_STATUS_CODE_ENUM_OK;
    priv->result = NULL;
    priv->error_type = LMI_JOB_ERROR_TYPE_ENUM_UNKNOWN;
    priv->error = NULL;
    priv->data = NULL;
    priv->data_destroy = NULL;
}

static void lmi_job_finalize(GObject *object)
{
    LmiJob *self = LMI_JOB(object);
    LmiJobPrivate *priv = self->priv;

    if (priv->data != NULL && priv->data_destroy != NULL) {
        priv->data_destroy(priv->data);
    }
    g_free(priv->name);
    g_free(priv->jobid);
    if (priv->in_parameters)
        g_hash_table_unref(priv->in_parameters);
    if (priv->out_parameters)
        g_hash_table_unref(priv->out_parameters);
    g_free(priv->method_name);
    if (priv->result)
        g_variant_unref(priv->result);
    g_free(priv->error);
    pthread_mutex_destroy(&priv->lock);

    G_OBJECT_CLASS(lmi_job_parent_class)->finalize(object);
}

/**
 * Initialize private properties. These are used only for (de)serialization.
 */
static gboolean init_private_prop_specs() {
    if (_private_prop_specs[PROP_PRIVATE_IN_PARAMETERS] == NULL) {
        _private_prop_specs[PROP_PRIVATE_IN_PARAMETERS] = g_param_spec_variant(
                PROP_IN_PARAMETERS_NAME,
                NULL, "Input parameters of asynchronous method.",
                PARAMS_VARIANT_TYPE, NULL,
                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
        if (_private_prop_specs[PROP_PRIVATE_IN_PARAMETERS] == NULL)
            goto err;
        _private_prop_specs[PROP_PRIVATE_OUT_PARAMETERS] = g_param_spec_variant(
                PROP_OUT_PARAMETERS_NAME,
                NULL, "Output parameters of asynchronous method.",
                PARAMS_VARIANT_TYPE, NULL,
                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
        if (_private_prop_specs[PROP_PRIVATE_OUT_PARAMETERS] == NULL) {
            g_free(_private_prop_specs[PROP_PRIVATE_IN_PARAMETERS]);
            _private_prop_specs[PROP_PRIVATE_IN_PARAMETERS] = NULL;
            goto err;
        }
    }
    return TRUE;

err:
    lmi_error("Memory allocation failed");
    return FALSE;
}

/**
 * Serializer needs to know about our private properties. We need to add them
 * to a list of property specifications.
 *
 * @return NULL-terminated array of pointers to param specifications. Release it
 *      with `g_free()`.
 */
static GParamSpec **lmi_job_serializable_iface_list_properties(
    JsonSerializable *serializable,
    guint *n_pspecs)
{
    guint index = 0;
    GParamSpec **pspecs = NULL;
    GParamSpec **pspecs_orig;
    g_assert(_private_prop_specs[PROP_PRIVATE_IN_PARAMETERS] != NULL);
    g_assert(_private_prop_specs[PROP_PRIVATE_OUT_PARAMETERS] != NULL);

    if ((pspecs_orig = g_object_class_list_properties(G_OBJECT_GET_CLASS(
                LMI_JOB(serializable)), NULL)) == NULL)
        goto err;
    if ((pspecs = g_malloc_n(sizeof(GParamSpec *), PROP_LAST + PROP_PRIVATE_LAST)) == NULL)
        goto pspecs_orig_err;

    while (index < PROP_LAST - 1) {
        pspecs[index] = pspecs_orig[index];
        ++index;
    }
    pspecs[index++] = _private_prop_specs[PROP_PRIVATE_IN_PARAMETERS];
    pspecs[index++] = _private_prop_specs[PROP_PRIVATE_OUT_PARAMETERS];
    pspecs[index] = NULL;
    if (n_pspecs)
        *n_pspecs = index;

    return pspecs;

pspecs_orig_err:
    g_free(pspecs_orig);
err:
    lmi_error("Memory allocation failed");
    return pspecs;
}

/**
 * Convert given variant to a dictionary entry (having "{ss}" variant type)
 * where key is a type string of variant and value is its serialized value
 * obtained with `g_variant_print()`.
 *
 * We need to know a type of variant we are parsing. And because *result* and
 * *in/out-parameters* can contain arbitrary value, we need to convert them
 * to string we know how to parse.
 */
static GVariant *make_param_entry(GVariant *source)
{
    gchar *value;
    GVariant *result = NULL;

    if ((value = g_variant_print(source, FALSE)) == NULL) {
        lmi_error("Failed to serialize variant!");
    } else if ((result = g_variant_new(PARAM_VARIANT_TYPE_STR,
            g_variant_get_type_string(source), value)) == NULL)
    {
        g_free(value);
        lmi_error("Memory allocation failed");
    }
    return result;
}

/**
 * Get private properties of job. Input and output parameters are turned
 * into a dictionary variant.
 */
static void lmi_job_serializable_iface_get_property(
    JsonSerializable *serializable,
    GParamSpec       *pspec,
    GValue           *value)
{
    GVariantBuilder variant_builder;
    GVariant *variant;
    GHashTable *params;
    GHashTableIter pi;
    gpointer param_key;
    gpointer param_value;
    LmiJob *job = LMI_JOB(serializable);
    int prop_id = -1;
    g_assert(_private_prop_specs[PROP_PRIVATE_IN_PARAMETERS] != NULL);
    g_assert(_private_prop_specs[PROP_PRIVATE_OUT_PARAMETERS] != NULL);

    if (!g_strcmp0(pspec->name, PROP_IN_PARAMETERS_NAME)) {
        prop_id = PROP_IN_PARAMETERS;
    } else if (!g_strcmp0(pspec->name, PROP_OUT_PARAMETERS_NAME)) {
        prop_id = PROP_OUT_PARAMETERS;
    }

    if (prop_id >= 0) {
        params = prop_id == PROP_IN_PARAMETERS ?
                job->priv->in_parameters : job->priv->out_parameters;
        g_variant_builder_init(&variant_builder, PARAMS_VARIANT_TYPE);
        if (params) {
            g_hash_table_iter_init(&pi, params);
            while (g_hash_table_iter_next(&pi, &param_key, &param_value)) {
                g_variant_builder_add(&variant_builder,
                        PARAMS_ENTRY_VARIANT_TYPE_STR, param_key, param_value);
            }
        }
        if ((variant = g_variant_builder_end(&variant_builder)) == NULL)
            goto variant_builder_err;
        g_value_set_variant(value, variant);
    } else {
        /* handle public properties */
        g_object_get_property(G_OBJECT(serializable), pspec->name, value);
    }
    return;

variant_builder_err:
    g_variant_builder_clear(&variant_builder);
    lmi_error("Memory allocation failed");
}

/**
 * Set deserialized values of private properties.
 */
static void lmi_job_serializable_iface_set_property(
    JsonSerializable *serializable,
    GParamSpec       *pspec,
    const GValue     *value)
{
    int prop_id = -1;
    GVariant *variant;
    GVariantIter iter;
    gchar *param_key;
    GVariant *param_value;
    LmiJob *job = LMI_JOB(serializable);
    g_assert(_private_prop_specs[PROP_PRIVATE_IN_PARAMETERS] != NULL);
    g_assert(_private_prop_specs[PROP_PRIVATE_OUT_PARAMETERS] != NULL);

    if (!g_strcmp0(pspec->name, PROP_IN_PARAMETERS_NAME)) {
        prop_id = PROP_IN_PARAMETERS;
    } else if (!g_strcmp0(pspec->name, PROP_OUT_PARAMETERS_NAME)) {
        prop_id = PROP_OUT_PARAMETERS;
    }

    if (prop_id >= 0) {
        if ((variant = g_value_get_variant(value)) == NULL)
            return;
        g_variant_iter_init(&iter, variant);
        while (g_variant_iter_loop(&iter, PARAMS_ENTRY_VARIANT_TYPE_STR, &param_key, &param_value))
        {
            if (prop_id == PROP_IN_PARAMETERS) {
                lmi_job_set_in_param(job, param_key, param_value);
            } else {
                lmi_job_set_out_param(job, param_key, param_value);
            }
        }
    } else {
        /* handle public properties */
        g_object_set_property(G_OBJECT(serializable), pspec->name, value);
    }
}

/**
 * Serialize any `GVariant` properties. Variants need to be preprocessed
 * before they can be passed to json serializer since it stores values
 * without type information. Therefore any variant is stored as a
 * dictionary entry holding strings. Where key is variant's type and
 * value is its serialized value.
 */
static JsonNode *lmi_job_serializable_iface_serialize_property(
    JsonSerializable *serializable,
    const gchar      *property_name,
    const GValue     *value,
    GParamSpec       *pspec)
{
    JsonNode *result = NULL;
    int prop_id = -1;
    GVariant *params_variant;
    GVariant *param_key;
    GVariant *param_value;
    GVariant *entry;
    GVariantBuilder builder;
    GVariantIter iter;
    g_assert(_private_prop_specs[PROP_PRIVATE_IN_PARAMETERS] != NULL);
    g_assert(_private_prop_specs[PROP_PRIVATE_OUT_PARAMETERS] != NULL);

    if (!g_strcmp0(pspec->name, PROP_IN_PARAMETERS_NAME)) {
        prop_id = PROP_IN_PARAMETERS;
    } else if (!g_strcmp0(pspec->name, PROP_OUT_PARAMETERS_NAME)) {
        prop_id = PROP_OUT_PARAMETERS;
    } else if (!g_strcmp0(pspec->name,
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_RESULT]))
    {
        prop_id = PROP_RESULT;
    }

    switch (prop_id) {
    case PROP_RESULT:
        params_variant = g_value_get_variant(value);
        entry = make_param_entry(params_variant);
        if (entry) {
            result = json_gvariant_serialize(entry);
            g_variant_unref(entry);
        }
        break;

    case PROP_IN_PARAMETERS:
    case PROP_OUT_PARAMETERS:
        params_variant = g_value_get_variant(value);
        g_variant_builder_init(&builder, (const GVariantType *) "a{s{ss}}");
        g_variant_iter_init(&iter, params_variant);
        while (g_variant_iter_next(&iter, "{&sv}", &param_key, &param_value)) {
            if ((entry = make_param_entry(param_value)) == NULL) {
                lmi_error("Failed to serialize \"%s\" parameter or %s!",
                        param_key, property_name);
                goto variant_builder_err;
            }
            g_variant_unref(param_value);
            g_variant_builder_add(&builder, "{s@{ss}}", param_key, entry);
        }
        if ((params_variant = g_variant_builder_end(&builder)) == NULL) {
            lmi_error("Failed to serialize \"%s\" property!", property_name);
            goto variant_builder_err;
        }
        result = json_gvariant_serialize(params_variant);
        g_variant_unref(params_variant);
        break;

    default:
        /* non-variant properties */
        result = json_serializable_default_serialize_property(
            serializable, property_name, value, pspec);
        break;
    }

    return result;

variant_builder_err:
    g_variant_builder_clear(&builder);
    return result;
}

/**
 * Reconstruct original variant converted by `make_param_entry()`.
 *
 * @param name Property name. Used just for error reporting.
 * @param entry Dictionary entry variant.
 * @return Original variant.
 */
static GVariant *parse_param_entry(const char *name, GVariant *entry)
{
    GError *gerror = NULL;
    GVariant *result = NULL;
    gchar *result_type;
    gchar *result_value;

    g_variant_get(entry, "{&s&s}", &result_type, &result_value);
    result = g_variant_parse((const GVariantType *) result_type,
                    result_value, NULL, NULL, &gerror);

    if (result == NULL) {
        lmi_error("Failed to parse %s entry: %s.", name, gerror->message);
        g_free(gerror);
    }
    return result;
}

/**
 * Reconstruct original variant converted by `make_param_entry()`.
 *
 * @param name Property name. Used just for error reporting.
 * @param node Json node that is expected to hold dictionary entry variant
 *      produced with `make_param_entry()`.
 * @param Original variant.
 */
static GVariant *parse_param_entry_node(const char *name, JsonNode *node)
{
    GError *gerror = NULL;
    GVariant *variant;
    GVariant *result = NULL;

    if ((variant = json_gvariant_deserialize(node, PARAM_VARIANT_TYPE_STR, &gerror)) != NULL)
    {
        result = parse_param_entry(name, variant);
        g_variant_unref(variant);
    } else {
        lmi_error("Failed to deserialize %s entry: %s.", name, gerror->message);
        g_error_free(gerror);
    }
    return result;
}

/**
 * Handle properties holding variants.
 */
static gboolean lmi_job_serializable_iface_deserialize_property(
    JsonSerializable *serializable,
    const gchar      *property_name,
    GValue           *value,
    GParamSpec       *pspec,
    JsonNode         *property_node)
{
    int prop_id = -1;
    GVariant *variant;
    GVariantIter vi;
    GVariantBuilder *builder;
    GVariant *parsed;
    GError *gerror = NULL;
    gchar *key;
    GVariant *param;
    g_assert(_private_prop_specs[PROP_PRIVATE_IN_PARAMETERS] != NULL);
    g_assert(_private_prop_specs[PROP_PRIVATE_OUT_PARAMETERS] != NULL);

    if (!g_strcmp0(pspec->name, PROP_IN_PARAMETERS_NAME)) {
        prop_id = PROP_IN_PARAMETERS;
    } else if (!g_strcmp0(pspec->name, PROP_OUT_PARAMETERS_NAME)) {
        prop_id = PROP_OUT_PARAMETERS;
    } else if (!g_strcmp0(pspec->name,
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_RESULT]))
    {
        prop_id = PROP_RESULT;
    }

    switch (prop_id) {
    case PROP_RESULT:
        if ((parsed = parse_param_entry_node("result", property_node)) == NULL)
            goto err;
        g_value_set_variant(value, parsed);
        break;

    case PROP_IN_PARAMETERS:
    case PROP_OUT_PARAMETERS:
        if ((variant = json_gvariant_deserialize(property_node,
                "a{s{ss}}", &gerror)) == NULL)
            goto parse_err;

        if ((builder = g_variant_builder_new(PARAMS_VARIANT_TYPE)) == NULL)
            goto variant_err;
        g_variant_iter_init(&vi, variant);
        while (g_variant_iter_next(&vi, "{&s@{ss}}", &key, &param))
        {
            if ((parsed = parse_param_entry(key, param)) == NULL) {
                g_variant_unref(param);
                goto builder_err;
            }
            g_variant_unref(param);
            g_variant_builder_add(builder, PARAMS_ENTRY_VARIANT_TYPE_STR,
                    key, parsed);
        }
        if ((parsed = g_variant_builder_end(builder)) == NULL)
            goto builder_err;
        g_variant_unref(variant);
        g_value_set_variant(value, parsed);
        break;

    default:
        /* non-variant properties */
        return json_serializable_default_deserialize_property(
                serializable, property_name, value, pspec, property_node);
    }
    return TRUE;

parse_err:
    lmi_error("Failed to parse \"%s\" property: %s.",
            pspec->name, gerror->message);
    g_free(gerror);
    goto err;
builder_err:
    g_variant_builder_unref(builder);
variant_err:
    g_variant_unref(variant);
err:
    return FALSE;
}

/**
 * Make sure private properties will be found by serializer.
 */
static GParamSpec *lmi_job_serializable_iface_find_property(
    JsonSerializable *serializable,
    const char *name)
{
    int prop_id = 0;
    g_assert(_private_prop_specs[PROP_PRIVATE_IN_PARAMETERS] != NULL);
    g_assert(_private_prop_specs[PROP_PRIVATE_OUT_PARAMETERS] != NULL);

    while (prop_id < PROP_PRIVATE_LAST) {
        if (!g_strcmp0(name, s_prop_private_names[prop_id])) {
            return _private_prop_specs[prop_id];
        }
        ++prop_id;
    }

    /* property must be public */
    return g_object_class_find_property(
            G_OBJECT_GET_CLASS(serializable), name);
}

static void lmi_job_serializable_iface_init(JsonSerializableIface *iface)
{
    iface->serialize_property = lmi_job_serializable_iface_serialize_property;
    iface->deserialize_property = lmi_job_serializable_iface_deserialize_property;
    iface->find_property = lmi_job_serializable_iface_find_property;
    iface->list_properties = lmi_job_serializable_iface_list_properties;
    iface->set_property = lmi_job_serializable_iface_set_property;
    iface->get_property = lmi_job_serializable_iface_get_property;
    init_private_prop_specs();
}

LmiJob *lmi_job_new()
{
    LmiJob *job;
    job = g_object_new(LMI_TYPE_JOB, NULL);
    return LMI_JOB(job);
}

guint lmi_job_get_number(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    guint res = job->priv->number;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

const gchar *lmi_job_get_name(const LmiJob *job)
{
    gchar *res = NULL;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    if (job->priv->name)
        res = job->priv->name;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

gchar *lmi_job_get_jobid(const LmiJob *job)
{
    gchar *res = NULL;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    if (job->priv->jobid) {
        res = g_strdup(job->priv->jobid);
    } else {
        res = g_strdup_printf("%u", job->priv->number);
    }
    PRIV_CRITICAL_END(job->priv);

    return res;
}

gboolean lmi_job_has_own_jobid(const LmiJob *job)
{
    gboolean res;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    res = job->priv->jobid != NULL;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

LmiJobStateEnum lmi_job_get_state(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    LmiJobStateEnum res = job->priv->state;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

guint lmi_job_get_priority(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    guint res = job->priv->priority;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

const gchar *lmi_job_get_method_name(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    const gchar *res = job->priv->method_name;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

time_t lmi_job_get_time_submitted(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    time_t res = job->priv->time_submitted;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

time_t lmi_job_get_time_before_removal(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    time_t res = job->priv->time_before_removal;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

time_t lmi_job_get_time_of_last_state_change(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    time_t res = job->priv->time_of_last_state_change;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

time_t lmi_job_get_start_time(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    time_t res = job->priv->start_time;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

gboolean lmi_job_get_delete_on_completion(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    gboolean res = job->priv->delete_on_completion;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

guint lmi_job_get_percent_complete(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    guint res = job->priv->percent_complete;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

LmiJobStatusCodeEnum lmi_job_get_status_code(const LmiJob *job)
{
    LmiJobStatusCodeEnum res;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    res = job->priv->status_code;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

GVariant *lmi_job_get_result(const LmiJob *job)
{
    GVariant *res = NULL;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    res = job->priv->result;
    if (res)
        g_variant_ref(res);
    PRIV_CRITICAL_END(job->priv);

    return res;
}

guint32 lmi_job_get_result_code(const LmiJob *job)
{
    GVariant *res = lmi_job_get_result(job);

    if (res == NULL)
        return G_MAXUINT32;
    if (!g_variant_is_of_type(res, G_VARIANT_TYPE_UINT32))
        return G_MAXUINT32 - 1;
    return g_variant_get_uint32(res);
}

LmiJobErrorTypeEnum lmi_job_get_error_type(const LmiJob *job)
{
    LmiJobErrorTypeEnum res;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    res = job->priv->error_type;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

const gchar *lmi_job_get_error(const LmiJob *job)
{
    gchar *res = NULL;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    res = job->priv->error;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

gpointer lmi_job_get_data(const LmiJob *job)
{
    gpointer res;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    res = job->priv->data;
    PRIV_CRITICAL_END(job->priv);

    return res;
}

gboolean lmi_job_is_finished(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    gboolean res = IS_FINAL_STATE(job->priv->state);
    PRIV_CRITICAL_END(job->priv);

    return res;
}

void lmi_job_set_name(LmiJob *job, const gchar *name)
{
    GVariant *vold, *vnew;
    const gchar *old;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    old = job->priv->name;
    if (  (old && name && strcmp(name, old))
       || (!old && name) || (old && !name))
    {
        job->priv->name = g_strdup(name);
        vold = g_variant_new_string(old ? old:"");
        vnew = g_variant_new_string(name ? name:"");
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_NAME]);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
                LMI_JOB_PROP_ENUM_NAME, vold, vnew);
    }
    PRIV_CRITICAL_END(job->priv);
}

void lmi_job_set_jobid(LmiJob *job, const gchar *jobid)
{
    GVariant *vold, *vnew;
    const gchar *old;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    old = job->priv->jobid;
    if (  (old && jobid && strcmp(jobid, old))
       || (!old && jobid) || (old && !jobid))
    {
        job->priv->jobid = g_strdup(jobid);
        vold = g_variant_new_string(old ? old:"");
        vnew = g_variant_new_string(jobid ? jobid:"");
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_JOBID]);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
                LMI_JOB_PROP_ENUM_NAME, vold, vnew);
        lmi_debug("Job #%u has just got an id=%s assigned.",
                lmi_job_get_number(job), jobid);
    }
    PRIV_CRITICAL_END(job->priv);
}

void lmi_job_set_priority(LmiJob *job, guint priority)
{
    GVariant *vold, *vnew;
    guint old;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    old = job->priv->priority;
    if (old != priority) {
        job->priv->priority = priority;
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_PRIORITY]);
        g_signal_emit(job, _signals[SIGNAL_PRIORITY_CHANGED], 0,
                old, priority);
        vold = g_variant_new_uint32(old);
        vnew = g_variant_new_uint32(priority);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
                LMI_JOB_PROP_ENUM_JOBID, vold, vnew);
    }
    PRIV_CRITICAL_END(job->priv);
}

void lmi_job_set_method_name(LmiJob *job, const gchar *method_name)
{
    GVariant *vold, *vnew;
    const gchar *old;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    old = job->priv->method_name;
    if (  (old && method_name && strcmp(method_name, old))
       || (!old && method_name) || (old && !method_name))
    {
        job->priv->method_name = g_strdup(method_name);
        vold = g_variant_new_string(old ? old:"");
        vnew = g_variant_new_string(method_name ? method_name:"");
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_METHOD_NAME]);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
                LMI_JOB_PROP_ENUM_METHOD_NAME, vold, vnew);
    }
    PRIV_CRITICAL_END(job->priv);
}

void lmi_job_set_time_before_removal(LmiJob *job,
                                     gint64 time_before_removal)
{
    GVariant *vold, *vnew;
    gint64 old;
    g_assert(LMI_IS_JOB(job));

    if (time_before_removal < MINIMUM_TIME_BEFORE_REMOVAL)
        time_before_removal = MINIMUM_TIME_BEFORE_REMOVAL;

    PRIV_CRITICAL_BEGIN(job->priv);
    old = job->priv->time_before_removal;
    if (old != time_before_removal) {
        job->priv->time_before_removal = time_before_removal;
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_TIME_BEFORE_REMOVAL]);
        g_signal_emit(job, _signals[SIGNAL_DELETION_REQUEST_CHANGED], 0,
                job->priv->delete_on_completion, time_before_removal);
        vold = g_variant_new_int64(old);
        vnew = g_variant_new_int64(time_before_removal);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
                LMI_JOB_PROP_ENUM_TIME_BEFORE_REMOVAL, vold, vnew);
    }
    PRIV_CRITICAL_END(job->priv);
}

void lmi_job_set_delete_on_completion(LmiJob *job,
                                      gboolean delete_on_completion)
{
    GVariant *vold, *vnew;
    gboolean old;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    old = job->priv->delete_on_completion;
    if (old != delete_on_completion) {
        job->priv->delete_on_completion = delete_on_completion;
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_DELETE_ON_COMPLETION]);
        g_signal_emit(job, _signals[SIGNAL_DELETION_REQUEST_CHANGED], 0,
                delete_on_completion, job->priv->time_before_removal);
        vold = g_variant_new_boolean(old);
        vnew = g_variant_new_boolean(delete_on_completion);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
                LMI_JOB_PROP_ENUM_DELETE_ON_COMPLETION, vold, vnew);
    }
    PRIV_CRITICAL_END(job->priv);
}

void lmi_job_set_percent_complete(LmiJob *job,
                                  guint percent_complete)
{
    GVariant *vold, *vnew;
    guint old;
    g_assert(LMI_IS_JOB(job));

    if (percent_complete > 100)
        percent_complete = 100;

    PRIV_CRITICAL_BEGIN(job->priv);
    old = job->priv->percent_complete;
    if (old != percent_complete) {
        job->priv->percent_complete = percent_complete;
        vold = g_variant_new_uint32(old);
        vnew = g_variant_new_uint32(percent_complete);
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_PERCENT_COMPLETE]);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
                LMI_JOB_PROP_ENUM_PERCENT_COMPLETE, vold, vnew);
    }
    PRIV_CRITICAL_END(job->priv);
}

void lmi_job_set_error_type(LmiJob *job, LmiJobErrorTypeEnum error_type)
{
    GVariant *vold, *vnew;
    guint old;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    old = job->priv->error_type;
    if (old != error_type) {
        job->priv->error_type = error_type;
        vold = g_variant_new_uint32(old);
        vnew = g_variant_new_uint32(error_type);
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_ERROR_TYPE]);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
                LMI_JOB_PROP_ENUM_ERROR_TYPE, vold, vnew);
    }
    PRIV_CRITICAL_END(job->priv);
}

void lmi_job_set_data(LmiJob *job,
                      gpointer job_data,
                      GDestroyNotify job_data_destroy)
{
    LmiJobPrivate *priv = job->priv;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(priv);
    if (priv->data != job_data) {
        if (priv->data != NULL && priv->data_destroy != NULL) {
           priv->data_destroy(priv->data);
        }
        priv->data = job_data;
        priv->data_destroy = job_data_destroy;
    }
    PRIV_CRITICAL_END(priv);
}

static gboolean lmi_job_set_state(LmiJob *job,
                                  LmiJobStateEnum state,
                                  LmiJobStatusCodeEnum status_code,
                                  GVariant *result,
                                  const gchar *error)
{
    GVariant *vold, *vnew;
    gchar *jobid = NULL;
    LmiJobStateEnum old_state;
    time_t old_start_time;
    time_t old_time_of_last_state_change;
    guint old_percent_complete;
    guint old_status_code;
    time_t current_time;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    old_state = job->priv->state;
    old_start_time = job->priv->start_time;
    old_percent_complete = job->priv->percent_complete;
    old_status_code = job->priv->status_code;
    old_time_of_last_state_change = job->priv->time_of_last_state_change;
    current_time  = time(NULL);

    if ((jobid = lmi_job_get_jobid(job)) == NULL)
        goto err;

    /* check inputs */
    if (  old_state != LMI_JOB_STATE_ENUM_NEW
       && state     == LMI_JOB_STATE_ENUM_RUNNING)
    {
        error = "Cannot start job \"%s\" second time!";
        goto bad_input;
    }

    if (  old_state != LMI_JOB_STATE_ENUM_RUNNING
       && state     == LMI_JOB_STATE_ENUM_COMPLETED)
    {
        if (old_state == LMI_JOB_STATE_ENUM_NEW) {
            error = "Cannot finish unstarted job \"%s\"!";
        } else {
            error = "Cannot finish already finished job \"%s\"!";
        }
        goto bad_input;
    }

    if (  IS_FINAL_STATE(old_state)
       && IS_FINAL_STATE(state))
    {
        error = "Cannot finish already finished job \"%s\"!";
        goto bad_input;
    }

    if (  IS_FINAL_STATE(old_state)
       && !IS_FINAL_STATE(state))
    {
        error = "Cannot change state of finished job \"%s\"!";
        goto bad_input;
    }

    /* modify job */
    job->priv->state = state;
    job->priv->time_of_last_state_change = current_time;
    job->priv->status_code = status_code;
    lmi_debug("State of job \"%s\" changed from \"%s\" to \"%s\".",
            jobid, s_job_state_enum_names[old_state], s_job_state_enum_names[state]);
    if (old_state == LMI_JOB_STATE_ENUM_NEW) {
        job->priv->start_time = current_time;
    }
    if (IS_FINAL_STATE(state)) {
        if (state == LMI_JOB_STATE_ENUM_COMPLETED && result) {
            job->priv->result = g_variant_ref_sink(result);
        } else if (state == LMI_JOB_STATE_ENUM_EXCEPTION && error) {
            job->priv->error = g_strdup(error);
        }
        if (state == LMI_JOB_STATE_ENUM_COMPLETED) {
            job->priv->percent_complete = 100;
        } else {
            job->priv->percent_complete = 0;
        }
    }

    g_object_notify(G_OBJECT(job),
            s_job_prop_enum_names[LMI_JOB_PROP_ENUM_STATE]);
    if (result != NULL)
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_RESULT]);
    if (error != NULL) {
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_ERROR]);
        lmi_debug("Job error set: \"%s\".", error);
    }
    if (IS_FINAL_STATE(state)) {
        g_signal_emit(job, _signals[SIGNAL_FINISHED], 0,
                old_state, state,
                job->priv->result == NULL
                    ? g_variant_new_handle(0)
                    : job->priv->result,
                job->priv->error);
    }

    /* emit signals */
    g_signal_emit(job, _signals[SIGNAL_STATE_CHANGED], 0,
            old_state, state);

    vold = g_variant_new_uint32(old_state);
    vnew = g_variant_new_uint32(state);
    g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
           LMI_JOB_PROP_ENUM_STATE, vold, vnew);

    if (old_state == LMI_JOB_STATE_ENUM_NEW) {
        vold = g_variant_new_int64(old_start_time);
        vnew = g_variant_new_int64(job->priv->start_time);
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_START_TIME]);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
               LMI_JOB_PROP_ENUM_START_TIME, vold, vnew);
    }

    if (old_percent_complete != job->priv->percent_complete) {
        vold = g_variant_new_uint32(old_percent_complete);
        vnew = g_variant_new_uint32(job->priv->percent_complete);
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_PERCENT_COMPLETE]);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
               LMI_JOB_PROP_ENUM_PERCENT_COMPLETE, vold, vnew);
    }

    if (old_status_code != job->priv->status_code) {
        vold = g_variant_new_uint32(old_status_code);
        vnew = g_variant_new_uint32(job->priv->status_code);
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_STATUS_CODE]);
        g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
               LMI_JOB_PROP_ENUM_STATUS_CODE, vold, vnew);
    }

    vold = g_variant_new_int64(old_time_of_last_state_change);
    vnew = g_variant_new_int64(job->priv->time_of_last_state_change);
        g_object_notify(G_OBJECT(job),
                s_job_prop_enum_names[LMI_JOB_PROP_ENUM_TIME_OF_LAST_STATE_CHANGE]);
    g_signal_emit(job, _signals[SIGNAL_MODIFIED], 0,
           LMI_JOB_PROP_ENUM_TIME_OF_LAST_STATE_CHANGE, vold, vnew);

    PRIV_CRITICAL_END(job->priv);

    return TRUE;

bad_input:
    lmi_error(error, jobid);
    g_free(jobid);
err:
    PRIV_CRITICAL_END(job->priv);
    return FALSE;
}

gboolean lmi_job_start(LmiJob *job)
{
    return lmi_job_set_state(job, LMI_JOB_STATE_ENUM_RUNNING,
            LMI_JOB_STATUS_CODE_ENUM_OK, NULL, NULL);
}

gboolean lmi_job_finish_ok(LmiJob *job, GVariant *result)
{
    return lmi_job_set_state(job, LMI_JOB_STATE_ENUM_COMPLETED,
           LMI_JOB_STATUS_CODE_ENUM_OK, result, NULL);
}

gboolean lmi_job_finish_ok_with_code(LmiJob *job, guint32 exit_code)
{
    gboolean result = false;
    GVariant *variant = g_variant_new_uint32(exit_code);
    if (variant != NULL) {
        result = lmi_job_finish_ok(job, variant);
    } else {
        lmi_error("Memory allocation failed");
    }
    return result;
}

gboolean lmi_job_finish_exception(LmiJob *job,
                                  LmiJobStatusCodeEnum status_code,
                                  const gchar *error, ...)
{
    gchar verror[BUFSIZ + 1];
    va_list args;

    va_start(args, error);
    vsnprintf(verror, BUFSIZ, error, args);
    va_end(args);

    return lmi_job_set_state(job, LMI_JOB_STATE_ENUM_EXCEPTION,
           status_code, NULL, verror);
}

gboolean lmi_job_finish_terminate(LmiJob *job)
{
    return lmi_job_set_state(job, LMI_JOB_STATE_ENUM_TERMINATED,
           LMI_JOB_STATUS_CODE_ENUM_OK, NULL, NULL);
}

gchar **lmi_job_get_in_param_keys(const LmiJob *job)
{
    gchar **res ;
    guint size = 0;
    GHashTableIter hi;
    gpointer key;
    int index = 0;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    if (job->priv->in_parameters)
        size = g_hash_table_size(job->priv->in_parameters);
    res = g_new(gchar *, size + 1);
    if (res != NULL) {
        if (size > 1) {
            g_hash_table_iter_init(&hi, job->priv->in_parameters);
            while (g_hash_table_iter_next(&hi, &key, NULL)) {
                res[index++] = g_strdup((const char *) key);
            }
        }
        res[index] = NULL;
    }
    PRIV_CRITICAL_END(job->priv);

    return res;
}

gboolean lmi_job_has_in_param(const LmiJob *job, const gchar *param)
{
    gboolean res = FALSE;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    if (job->priv->in_parameters)
        res = g_hash_table_lookup_extended(job->priv->in_parameters, param,
            NULL, NULL);
    PRIV_CRITICAL_END(job->priv);

    return res;
}

GVariant *lmi_job_get_in_param(const LmiJob *job, const gchar *param)
{
    GVariant *res = NULL;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    if (job->priv->in_parameters) {
        res = g_hash_table_lookup(job->priv->in_parameters, param);
        if (res)
            g_variant_ref(res);
    }
    PRIV_CRITICAL_END(job->priv);

    return res;
}

gboolean lmi_job_set_in_param(LmiJob *job, const gchar *param, GVariant *value)
{
    gchar *key;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    if (!job->priv->in_parameters)
        job->priv->in_parameters = g_hash_table_new_full(
                lmi_str_lcase_hash_func, lmi_str_icase_equal, g_free,
                (GDestroyNotify) g_variant_unref);
    if (!job->priv->in_parameters)
        goto err;
    if ((key = g_strdup(param)) == NULL)
        goto err;
    g_hash_table_insert(job->priv->in_parameters, key, value);
    g_variant_ref_sink(value);
    PRIV_CRITICAL_END(job->priv);

    return TRUE;
err:
    PRIV_CRITICAL_END(job->priv);
    return FALSE;
}

gchar **lmi_job_get_out_param_keys(const LmiJob *job)
{
    gchar **res = NULL;
    guint size = 0;
    GHashTableIter hi;
    gpointer key;
    int index = 0;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    if (job->priv->out_parameters)
        size = g_hash_table_size(job->priv->out_parameters);
    res = g_new(gchar *, size + 1);
    if (res != NULL) {
        if (size > 1) {
            g_hash_table_iter_init(&hi, job->priv->out_parameters);
            while (g_hash_table_iter_next(&hi, &key, NULL)) {
                res[index++] = g_strdup((const char *) key);
            }
        }
        res[index] = NULL;
    }
    PRIV_CRITICAL_END(job->priv);

    return res;
}

gboolean lmi_job_has_out_param(const LmiJob *job, const gchar *param)
{
    gboolean res = FALSE;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    if (job->priv->out_parameters)
        res = g_hash_table_lookup_extended(job->priv->out_parameters, param,
            NULL, NULL);
    PRIV_CRITICAL_END(job->priv);

    return res;
}

GVariant *lmi_job_get_out_param(const LmiJob *job, const gchar *param)
{
    GVariant *res = NULL;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    if (job->priv->out_parameters) {
        res = g_hash_table_lookup(job->priv->out_parameters, param);
        if (res)
            g_variant_ref(res);
    }
    PRIV_CRITICAL_END(job->priv);

    return res;
}

gboolean lmi_job_set_out_param(LmiJob *job, const gchar *param, GVariant *value)
{
    gchar *key;
    g_assert(LMI_IS_JOB(job));

    PRIV_CRITICAL_BEGIN(job->priv);
    if (!job->priv->out_parameters)
        job->priv->out_parameters = g_hash_table_new_full(
                lmi_str_lcase_hash_func, lmi_str_icase_equal, g_free,
                (GDestroyNotify) g_variant_unref);
    if (!job->priv->out_parameters)
        goto err;
    if ((key = g_strdup(param)) == NULL)
        goto err;
    g_hash_table_insert(job->priv->out_parameters, key, value);
    g_variant_ref_sink(value);
    PRIV_CRITICAL_END(job->priv);

    return TRUE;
err:
    PRIV_CRITICAL_END(job->priv);
    return FALSE;
}

const gchar *lmi_job_state_to_string(LmiJobStateEnum state)
{
    g_assert(  state >= LMI_JOB_STATE_ENUM_NEW
            && state < LMI_JOB_STATE_ENUM_LAST);
    return s_job_state_enum_names[state];
}

const gchar *lmi_job_prop_to_string(LmiJobPropEnum prop)
{
    g_assert(prop >= 0 && prop < LMI_JOB_PROP_ENUM_LAST);
    return s_job_prop_enum_names[prop];
}

void lmi_job_critical_section_begin(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));
    PRIV_CRITICAL_BEGIN(job->priv);
}

void lmi_job_critical_section_end(const LmiJob *job)
{
    g_assert(LMI_IS_JOB(job));
    PRIV_CRITICAL_END(job->priv);
}

static void process_finished_signal(LmiJob *job,
                                    LmiJobStateEnum old_state,
                                    LmiJobStateEnum new_state,
                                    GVariant *result,
                                    const gchar *error,
                                    gpointer data)
{
    GMainLoop *main_loop = data;
    g_main_loop_quit(main_loop);
}

static gboolean unlock_job(gpointer data)
{
    LmiJob *job = LMI_JOB(data);
    JOB_CRITICAL_END(job);
    return FALSE;
}

void lmi_job_wait_until_finished(LmiJob *job)
{
    /* Let's run private main loop which waits for *finished* signal of job. */
    GMainContext *ctx = NULL;
    GMainLoop *main_loop = NULL;
    gchar *jobid;
    GSource *source = NULL;

    JOB_CRITICAL_BEGIN(job);
    if (!lmi_job_is_finished(job)) {
        ctx = g_main_context_new();
        if (ctx == NULL)
            goto critical_end_err;
        main_loop = g_main_loop_new(ctx, FALSE);
        if (main_loop == NULL)
            goto ctx_err;
        g_main_context_push_thread_default(ctx);

        /* Callback needs reference to our private main loop because it
         * needs to stop it */
        g_signal_connect(job, "finished", G_CALLBACK(process_finished_signal),
                main_loop);

        /* First thing that needs to be done when the loop is running is
         * unlocking the job so it can be finished in another thread. */
        if ((source = g_idle_source_new()) == NULL)
            goto pop_ctx_err;
        g_source_set_callback(source, unlock_job, job, NULL);
        g_source_attach(source, ctx);

        jobid = lmi_job_get_jobid(job);
        lmi_debug("Waiting for transition to finished state of job \"%s\".",
                jobid);
        g_free(jobid);

        /* Block until the main loop is stopped from *process_finished_signal*
         * callback. */
        g_main_loop_run(main_loop);

        g_signal_handlers_disconnect_by_func(
                job, process_finished_signal, NULL);
        g_main_context_pop_thread_default(ctx);
        g_main_loop_unref(main_loop);
        g_main_context_unref(ctx);
    } else {
        JOB_CRITICAL_END(job);
    }
    return;

pop_ctx_err:
    g_signal_handlers_disconnect_by_func(job, process_finished_signal, NULL);
    g_main_context_pop_thread_default(ctx);
    g_main_loop_unref(main_loop);
ctx_err:
    g_main_context_unref(ctx);
critical_end_err:
    JOB_CRITICAL_END(job);
    lmi_error("Memory allocation failed");
}

