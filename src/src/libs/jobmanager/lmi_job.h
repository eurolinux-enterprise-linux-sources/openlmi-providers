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

#include <glib-object.h>
#include <gio/gio.h>

#ifndef LMI_JOB_H
#define LMI_JOB_H

#define LMI_TYPE_JOB                  (lmi_job_get_type ())
#define LMI_JOB(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LMI_TYPE_JOB, LmiJob))
#define LMI_IS_JOB(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LMI_TYPE_JOB))
#define LMI_JOB_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), LMI_TYPE_JOB, LmiJobClass))
#define LMI_IS_JOB_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), LMI_TYPE_JOB))
#define LMI_JOB_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), LMI_TYPE_JOB, LmiJobClass))

typedef struct _LmiJobPrivate LmiJobPrivate;
typedef struct _LmiJob        LmiJob;
typedef struct _LmiJobClass   LmiJobClass;

#define CLOG_COLOR_BLACK        30
#define CLOG_COLOR_RED          31
#define CLOG_COLOR_GREEN        32
#define CLOG_COLOR_YELLOW       33
#define CLOG_COLOR_BLUE         34
#define CLOG_COLOR_MAGENTA      35
#define CLOG_COLOR_CYAN         36
#define CLOG_COLOR_WHITE        37

#ifdef DEBUG_LOCKING
    #include <stdio.h>

    #define _clog(color_code, msg, ...) \
        fprintf(stderr, "\x1b[%um" __FILE__ ":%u " msg "\x1b[0m\n", \
                color_code, __LINE__, ##__VA_ARGS__)
#else
    static inline void _clog(unsigned int color_code, const char *msg, ...)
    {
        (void) msg;
    }
#endif

#define JOB_CRITICAL_BEGIN(job) \
    { \
        pthread_t _thread_id = pthread_self(); \
        _clog(CLOG_COLOR_CYAN, "[tid=%lu] locking job",  _thread_id); \
        lmi_job_critical_section_begin(job); \
        _clog(CLOG_COLOR_CYAN, "[tid=%lu] locked job", _thread_id); \
    }

#define JOB_CRITICAL_END(job) \
    { \
        pthread_t _thread_id = pthread_self(); \
        _clog(CLOG_COLOR_CYAN, "[tid=%lu] unlocking job", _thread_id); \
        lmi_job_critical_section_end(job); \
        _clog(CLOG_COLOR_CYAN, "[tid=%lu] unlocked job", _thread_id); \
    }

/**
 * Possible job states. Job begins with *NEW* state and transition to *RUNNING*
 * when it's started. Next state is one of *COMPLETED*, *TERMINATED* and
 * *EXCEPTION*. These states are called *final* states. No other transition
 * can be made from final state.
 */
typedef enum {
    LMI_JOB_STATE_ENUM_NEW,
    LMI_JOB_STATE_ENUM_RUNNING,
    LMI_JOB_STATE_ENUM_COMPLETED,
    LMI_JOB_STATE_ENUM_TERMINATED,
    LMI_JOB_STATE_ENUM_EXCEPTION,
    LMI_JOB_STATE_ENUM_LAST
}LmiJobStateEnum;

/**
 * Used as a value of `status-code` property of job.
 *
 * Can only be set with `lmi_job_finish_exception()`.
 */
typedef enum {
    LMI_JOB_STATUS_CODE_ENUM_OK                                  =  0,
    LMI_JOB_STATUS_CODE_ENUM_FAILED                              =  1,
    LMI_JOB_STATUS_CODE_ENUM_ACCESS_DENIED                       =  2,
    LMI_JOB_STATUS_CODE_ENUM_INVALID_NAMESPACE                   =  3,
    LMI_JOB_STATUS_CODE_ENUM_INVALID_PARAMETER                   =  4,
    LMI_JOB_STATUS_CODE_ENUM_INVALID_CLASS                       =  5,
    LMI_JOB_STATUS_CODE_ENUM_NOT_FOUND                           =  6,
    LMI_JOB_STATUS_CODE_ENUM_NOT_SUPPORTED                       =  7,
    LMI_JOB_STATUS_CODE_ENUM_CLASS_HAS_CHILDREN                  =  8,
    LMI_JOB_STATUS_CODE_ENUM_CLASS_HAS_INSTANCES                 =  9,
    LMI_JOB_STATUS_CODE_ENUM_INVALID_SUPERCLASS                  = 10,
    LMI_JOB_STATUS_CODE_ENUM_ALREADY_EXISTS                      = 11,
    LMI_JOB_STATUS_CODE_ENUM_NO_SUCH_PROPERTY                    = 12,
    LMI_JOB_STATUS_CODE_ENUM_TYPE_MISMATCH                       = 13,
    LMI_JOB_STATUS_CODE_ENUM_QUERY_LANGUAGE_NOT_SUPPORTED        = 14,
    LMI_JOB_STATUS_CODE_ENUM_INVALID_QUERY                       = 15,
    LMI_JOB_STATUS_CODE_ENUM_METHOD_NOT_AVAILABLE                = 16,
    LMI_JOB_STATUS_CODE_ENUM_METHOD_NOT_FOUND                    = 17,
    LMI_JOB_STATUS_CODE_ENUM_UNEXPECTED_RESPONSE                 = 18,
    LMI_JOB_STATUS_CODE_ENUM_INVALID_RESPONSE_DESTINATION        = 19,
    LMI_JOB_STATUS_CODE_ENUM_NAMESPACE_NOT_EMPTY                 = 20,
    LMI_JOB_STATUS_CODE_ENUM_INVALID_ENUMERATION_CONTEXT         = 21,
    LMI_JOB_STATUS_CODE_ENUM_INVALID_OPERATION_TIMEOUT           = 22,
    LMI_JOB_STATUS_CODE_ENUM_PULL_HAS_BEEN_ABANDONED             = 23,
    LMI_JOB_STATUS_CODE_ENUM_PULL_CANNOT_BE_ABANDONED            = 24,
    LMI_JOB_STATUS_CODE_ENUM_FILTERED_ENUMERATION_NOT_SUPPORTED  = 25,
    LMI_JOB_STATUS_CODE_ENUM_CONTINUATION_ON_ERROR_NOT_SUPPORTED = 26,
    LMI_JOB_STATUS_CODE_ENUM_SERVER_LIMITS_EXCEEDED              = 27,
    LMI_JOB_STATUS_CODE_ENUM_SERVER_IS_SHUTTING_DOWN             = 28,
    LMI_JOB_STATUS_CODE_ENUM_QUERY_FEATURE_NOT_SUPPORTED         = 29,
    LMI_JOB_STATUS_CODE_ENUM_LAST
}LmiJobStatusCodeEnum;

/**
 * Used as a value of `error-type` property of job.
 *
 * Shall only be set when is in `EXCEPTION` state.
 */
typedef enum {
    LMI_JOB_ERROR_TYPE_ENUM_UNKNOWN                     =  0,
    LMI_JOB_ERROR_TYPE_ENUM_OTHER                       =  1,
    LMI_JOB_ERROR_TYPE_ENUM_COMMUNICATIONS_ERROR        =  2,
    LMI_JOB_ERROR_TYPE_ENUM_QUALITY_OF_SERVICE_ERROR    =  3,
    LMI_JOB_ERROR_TYPE_ENUM_SOFTWARE_ERROR              =  4,
    LMI_JOB_ERROR_TYPE_ENUM_HARDWARE_ERROR              =  5,
    LMI_JOB_ERROR_TYPE_ENUM_ENVIRONMENTAL_ERROR         =  6,
    LMI_JOB_ERROR_TYPE_ENUM_SECURITY_ERROR              =  7,
    LMI_JOB_ERROR_TYPE_ENUM_OVERSUBSCRIPTION_ERROR      =  8,
    LMI_JOB_ERROR_TYPE_ENUM_UNAVAILABLE_RESOURCE_ERROR  =  9,
    LMI_JOB_ERROR_TYPE_ENUM_UNSUPPORTED_OPERATION_ERROR = 10,
    LMI_JOB_ERROR_TYPE_ENUM_LAST
}LmiJobErrorTypeEnum;

/**
 * This identifies modifiable properties of job. It's particularly useful for
 * *modified* signal of job instance to identify property that got changed.
 */
typedef enum {
    LMI_JOB_PROP_ENUM_NAME,
    LMI_JOB_PROP_ENUM_JOBID,
    LMI_JOB_PROP_ENUM_STATE,
    LMI_JOB_PROP_ENUM_PRIORITY,
    LMI_JOB_PROP_ENUM_METHOD_NAME,
    LMI_JOB_PROP_ENUM_TIME_BEFORE_REMOVAL,
    LMI_JOB_PROP_ENUM_TIME_OF_LAST_STATE_CHANGE,
    LMI_JOB_PROP_ENUM_START_TIME,
    LMI_JOB_PROP_ENUM_DELETE_ON_COMPLETION,
    LMI_JOB_PROP_ENUM_PERCENT_COMPLETE,
    LMI_JOB_PROP_ENUM_STATUS_CODE,
    LMI_JOB_PROP_ENUM_RESULT,
    LMI_JOB_PROP_ENUM_ERROR_TYPE,
    LMI_JOB_PROP_ENUM_ERROR,
    LMI_JOB_PROP_ENUM_LAST
}LmiJobPropEnum;

struct _LmiJob {
    GObject parent;
    LmiJobPrivate *priv;
};

struct _LmiJobClass {
    GObjectClass parent_class;

    /* Follows a list of instance signals. They are listed in ascending
     * priority - first listed will be emitted last. */

    /**
     * Signal emitted whenever there is a change to some property.
     */
    void (*modified) (LmiJob *job,
                      LmiJobPropEnum property,
                      GVariant *old_value,
                      GVariant *new_value);

    /**
     * Signal emitted when there is a change to `time-before-removal`
     * or `delete-on-completion` properties.
     */
    void (*deletion_request_changed) (LmiJob *job,
                                      gboolean delete_on_completion,
                                      gint64 time_before_removal);

    /**
     * Signal emitted when a change to job's priority occurs.
     */
    void (*priority_changed) (
            LmiJob *job,
            guint old_priority,
            guint new_priority);

    /**
     * Signal emitted whenever job changes its state. It is usually triggered
     * just twice per one job. Once upon a transition from *NEW* to *RUNNING*
     * and then from *RUNNING* to one of final states.
     */
    void (*state_changed) (LmiJob *job,
                           LmiJobStateEnum old_state,
                           LmiJobStateEnum new_state);

    /**
     * Signal emitted when job's state changes to final one.
     *
     * @param result Result of a job when job's state is *COMPLETED*.
     *     `G_VARIANT_TYPE_HANDLE` with value 0 is passed otherwise.
     * @param error Error message filled when job's state is *EXCEPTION*.
     */
    void (*finished) (LmiJob *job,
                      LmiJobStateEnum old_state,
                      LmiJobStateEnum new_state,
                      GVariant *result,
                      const gchar *error);

};

GType lmi_job_get_type();

/**
 * Create a new job. Number generator is invoked to get unique number. Other
 * properties are set to their default values.
 */
LmiJob *lmi_job_new();

/**
 * Get a unique number of job.
 */
guint lmi_job_get_number(const LmiJob *job);

/**
 * Get the name of job. Default value is `NULL`. This value is assigned to
 * `Name` property of corresponding `CIM_ConcreteJob` instance.
 *
 * Do not free returned value.
 */
const gchar *lmi_job_get_name(const LmiJob *job);

/**
 * Get the jobid. This is used to associate job's instance with corresponding
 * job of your provider's backend.
 *
 * If not set with `lmi_job_set_jobid`, job's number in decimal representation
 * will be returned.
 *
 * Result needs to be freed with `g_free()`.
 */
gchar *lmi_job_get_jobid(const LmiJob *job);

/**
 * Is the jobid set?
 */
gboolean lmi_job_has_own_jobid(const LmiJob *job);

/**
 * Get a current state of job.
 */
LmiJobStateEnum lmi_job_get_state(const LmiJob *job);

/**
 * Get job's priority. Default value is 128.
 */
guint lmi_job_get_priority(const LmiJob *job);

/**
 * Get a name of CIM method whose invocation led to creation of this job. This
 * property needs to be set with `lmi_job_set_method_name()` shortly after
 * job's creation.
 *
 * Do not free returned value.
 */
const gchar *lmi_job_get_method_name(const LmiJob *job);

/**
 * Get a time of job's creation. Readable property, initialized upon job's
 * creation.
 */
time_t lmi_job_get_time_submitted(const LmiJob *job);

/**
 * Get number of seconds the job is kept after its completion. If
 * `delete-on-completion` property is set and job comes to final state, it will
 * be deleted after certain number of seconds returned by this function.
 *
 * Default and minimum value is 5 minutes.
 */
time_t lmi_job_get_time_before_removal(const LmiJob *job);

/**
 * Get a time of job's last state change.
 */
time_t lmi_job_get_time_of_last_state_change(const LmiJob *job);

/**
 * Get a time of job's execution start. If job is not started, 0 will be
 * returned.
 */
time_t lmi_job_get_start_time(const LmiJob *job);

/**
 * Shall the job be deleted after its completion? It's not deleted
 * immediately but after `time-before-removal` seconds.
 */
gboolean lmi_job_get_delete_on_completion(const LmiJob *job);

/**
 * Get the percentage of completion, an integer ranging from 0 to 100.
 * This number shall be updated during job's *RUNNING* state. When
 * job finishes successfully, it is set to 100.
 */
guint lmi_job_get_percent_complete(const LmiJob *job);

/**
 * Get CIM status code of job. This shall be set together with `EXCEPTION`
 * state. It won't be modified if job completes successfully.
 */
LmiJobStatusCodeEnum lmi_job_get_status_code(const LmiJob *job);

/**
 * Get result of completed job. Unless job is in *COMPLETED* state, this
 * returns `NULL`.
 *
 * Call `g_variant_unref()` when the result is not needed anymore.
 */
GVariant *lmi_job_get_result(const LmiJob *job);

/**
 * Get result code of completed job. Unless job is in *COMPLETED* state, this
 * returns G_MAXUINT. If the result is not type of unsigned int, G_MAXUINT-1 is
 * returned.
 */
guint32 lmi_job_get_result_code(const LmiJob *job);

/**
 * Get error type. This is optionally set when job completes with error.
 */
LmiJobErrorTypeEnum lmi_job_get_error_type(const LmiJob *job);

/**
 * Get error message of failed job. Unless is in *EXCEPTION* state, this
 * returns `NULL`.
 *
 * Do not free returned value.
 */
const gchar *lmi_job_get_error(const LmiJob *job);

/**
 * Get job data pointer.
 */
gpointer lmi_job_get_data(const LmiJob *job);

/**
 * Check, whether job is in final state.
 */
gboolean lmi_job_is_finished(const LmiJob *job);

/**
 * Change the name of job. Affects `Name` property of corresponding
 * `CIM_ConreteJob`. Should be used immediately after job's creation.
 *
 * Emits *modified* signal.
 */
void lmi_job_set_name(LmiJob *job, const gchar *name);

/**
 * Set the job id. Should be used immediately after job's creation.
 *
 * Emits *modified* signal.
 */
void lmi_job_set_jobid(LmiJob *job, const gchar *jobid);

/**
 * Change job's priority.
 *
 * Emits *priority-changed* and *modified* signals.
 */
void lmi_job_set_priority(LmiJob *job, guint priority);

/**
 * Set the name of CIM method creating given job.
 *
 * Emits *modified* signal.
 */
void lmi_job_set_method_name(LmiJob *job, const gchar *method_name);

/**
 * Change time interval of job's removal after its completion.
 *
 * Emits *deletion-request-changed* and *modified* signals.
 */
void lmi_job_set_time_before_removal(LmiJob *job,
                                     glong time_before_removal);

/**
 * Specify whether job shall be deleted after its completion.
 *
 * Emits *deletion-request-changed* and *modified* signals.
 */
void lmi_job_set_delete_on_completion(LmiJob *job,
                                      gboolean delete_on_completion);

/**
 * Change percentage of job's completion.
 *
 * Emits *modified* signal.
 *
 * @param percent_complete Is an integer ranging from 0 to 100.
 */
void lmi_job_set_percent_complete(LmiJob *job, guint percent_complete);

/**
 * Change error type of job. This shall be used only for failed job.
 */
void lmi_job_set_error_type(LmiJob *job, LmiJobErrorTypeEnum error_type);

/**
 * Set user data. If already set, current data will be released.
 */
void lmi_job_set_data(LmiJob *job,
                      gpointer job_data,
                      GDestroyNotify job_data_destroy);

/**
 * Change job's state to *RUNNING*. Job must be in *NEW* state.
 *
 * Emits *state-changed* and *modified* signals.
 */
gboolean lmi_job_start(LmiJob *job);

/**
 * Change job's state to *COMPLETED* and set the result.
 *
 * Emits *finished*, *state-changed* and *modified* signals.
 *
 * @param result Result of asynchronous method. If it is a floating variant,
 *      it will be singed, otherise it will be referenced.
 * @returns Whether the state was successfully set.
 */
gboolean lmi_job_finish_ok(LmiJob *job, GVariant *result);

/**
 * Change job's state to *COMPLETED* and set the result code.
 *
 * Emits *finished*, *state-changed* and *modified* signals.
 *
 * @param result Result of asynchronous method. Unsigned integer
 *      GVariant will be created to hold this value.
 *      Use `lmi_job_get_result_code()` to get the result back.
 * @returns Whether the state was successfully set.
 */
gboolean lmi_job_finish_ok_with_code(LmiJob *job, guint32 exit_code);

/**
 * Change job's state to *EXCEPTION* and set the error message.
 *
 * Emits *finished*, *state-changed* and *modified* signals.
 *
 * @returns Whether the state was successfully set.
 */
gboolean lmi_job_finish_exception(LmiJob *job,
                                  LmiJobStatusCodeEnum status_code,
                                  const gchar *error, ...);

/**
 * Change job's state to *TERMINATED*.
 *
 * Emits *finished*, *state-changed* and *modified* signals.
 *
 * @returns Whether the state was successfully set.
 */
gboolean lmi_job_finish_terminate(LmiJob *job);

/**
 * Get names of input parameters of corresponding CIM method that caused
 * job's creation.
 *
 * Release the result with `g_strfreev().`
 *
 * @returns `NULL`-terminated array of input parameter names.
 */
gchar **lmi_job_get_in_param_keys(const LmiJob *job);

/**
 * Is input parameter set for given job? Check is done in case-insensitive way.
 */
gboolean lmi_job_has_in_param(const LmiJob *job, const gchar *param);

/**
 * Get the value of input parameter of CIM method.
 *
 * Unreference the result with `g_variant_unref()` when you don't need it.
 *
 * @param param Name of queried input parameter. Lookup is done in
 * case-insensitive way.
 * @returns variant that shall be unreferenced when not needed.
 *      `NULL` is returned for missing key.
 */
GVariant *lmi_job_get_in_param(const LmiJob *job, const gchar *param);

/**
 * Set the value of input parameter of CIM method whose invocation led to creation
 * of given job.
 *
 * @param param Name of input parameter which is compared in case-insensitive way
 *      to others.
 * @param value Any value of any type. If it is floating variant, it will be
 *      singed, otherwise it will be referenced.
 */
gboolean lmi_job_set_in_param(LmiJob *job, const gchar *param, GVariant *value);

/**
 * Get names of outputput parameters of corresponding CIM method that caused
 * job's creation.
 *
 * Release the result with `g_strfreev().`
 *
 * @returns `NULL`-terminated array of output parameter names.
 */
gchar **lmi_job_get_out_param_keys(const LmiJob *job);

/**
 * Is output parameter set for given job? Check is done in case-insensitive way.
 */
gboolean lmi_job_has_out_param(const LmiJob *job, const gchar *param);

/**
 * Get the value of output parameter of CIM method.
 *
 * Unreference the result with `g_variant_unref()` when you don't need it.
 *
 * @param param Name of queried output parameter. Lookup is done in
 *      case-insensitive way.
 * @returns variant that shall be unreferenced when not needed.
 *      `NULL` is returned for missing key.
 */
GVariant *lmi_job_get_out_param(const LmiJob *job, const gchar *param);

/**
 * Set the value of output parameter of CIM method whose invocation led to creation
 * of given job.
 *
 * @param param Name of output parameter which is compared in case-insensitive way
 *      to others.
 * @param value Any value of any type. If it is floating variant, it will be
 *      singed, otherwise it will be referenced. */
gboolean lmi_job_set_out_param(LmiJob *job, const gchar *param, GVariant *value);

/**
 * Get a name of job's state.
 */
const gchar *lmi_job_state_to_string(LmiJobStateEnum state);
/**
 * Get a name of job's property.
 */
const gchar *lmi_job_prop_to_string(LmiJobPropEnum prop);

/**
 * Use this function in pair with `lmi_job_critical_section_end()` to surround
 * critical section, where job's internal data must stay untouched by other
 * threads.
 *
 * @note Accessing and modifying job from concurrent threads is itself thread-safe.
 *      This allows for exclusive use of job across several get and sets.
 *
 * Don't forget to call `lmi_job_critical_section_end()` when you're finished.
 */
void lmi_job_critical_section_begin(const LmiJob *job);

/**
 * Use at the end of critical section starting with
 * `lmi_job_critical_section_begin()`.
 */
void lmi_job_critical_section_end(const LmiJob *job);

/**
 * Serialize job object to output stream.
 *
 * @return TRUE if job was successfully written.
 */
gboolean lmi_job_dump(const LmiJob *job, GOutputStream *stream);

/**
 * Serialize job object to a file.
 *
 * @return TRUE if file was successfully written.
 */
gboolean lmi_job_dump_to_file(const LmiJob *job, const gchar *file_path);

/**
 * Deserialize job object from input stream.
 */
LmiJob *lmi_job_load(GInputStream *stream, GType job_type);

/**
 * Deserialize job object from file.
 */
LmiJob *lmi_job_load_from_file(const gchar *file_path, GType job_type);

/**
 * Block until the job is finished.
 */
void lmi_job_wait_until_finished(LmiJob *job);

#endif /* end of include guard: LMI_JOB_H */
