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

/**
 * Job Manager module.
 *
 * Typical usage:
 *      1. Register callbacks for all type of jobs you want to support.
 *         If just one job type is enough, your registration will look
 *         like this:
 *
 *              jobmgr_register_job_type(
 *                      LMI_TYPE_JOB,           // job type
 *                      "LMI_SoftwareJob",      // CIM class name
 *                      job_to_cim_cb,          // conversion function
 *                      // function converting in/out parameters to CIM values
 *                      make_job_params_cb,
 *                      // return value job of async method
 *                      METHOD_RESULT_VALUE_TYPE_UINT32,
 *                      job_process_cb,         // job processing function
 *                      // whether the jobs can be processed in parallel
 *                      true,
 *                      // whether to make the jobs persistent
 *                      true);
 *
 *
 *      2. Place `jobmgr_init()` to your provider's init.
 *      3. Define your callbacks - see descriptions of
 *         `JobToCimInstanceCallback` and `JobProcessCallback`.
 *
 *      4. In your implementation of asynchronous method:
 *
 *          1. Check input arguments.
 *          2. Create a job with `jobmgr_new_job()`.
 *          3. Set its method name and input parameters according
 *             to method being implemented. Optionally set other
 *             properties such as priority, name, etc.
 *          4. Enqueue the job for execution with `jobmgr_run_job()`.
 *          5. Return standard value for "Asynchronous Job Started".
 *
 *      5. Place jobmgr_cleanup()` to your provider's cleanup.
 */

#ifndef JOB_MANAGER_H
#define JOB_MANAGER_H

#include <cmpimacs.h>
#include "lmi_job.h"

#ifndef JOB_MANAGER_PERSISTENT_STORAGE_PREFIX
    #define JOB_MANAGER_PERSISTENT_STORAGE_PREFIX "/var/lib/openlmi-providers/jobs"
#endif

#define CIM_CONCRETE_JOB_JOB_STATE_NEW                                  2
#define CIM_CONCRETE_JOB_JOB_STATE_STARTING                             3
#define CIM_CONCRETE_JOB_JOB_STATE_RUNNING                              4
#define CIM_CONCRETE_JOB_JOB_STATE_SUSPENDED                            5
#define CIM_CONCRETE_JOB_JOB_STATE_SHUTTING_DOWN                        6
#define CIM_CONCRETE_JOB_JOB_STATE_COMPLETED                            7
#define CIM_CONCRETE_JOB_JOB_STATE_TERMINATED                           8
#define CIM_CONCRETE_JOB_JOB_STATE_KILLED                               9
#define CIM_CONCRETE_JOB_JOB_STATE_EXCEPTION                           10
#define CIM_CONCRETE_JOB_JOB_STATE_SERVICE                             11
#define CIM_CONCRETE_JOB_JOB_STATE_QUERY_PENDING                       12

#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_UNKNOWN                     0
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_OTHER                       1
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_OK                          2
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_DEGRADED                    3
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_STRESSED                    4
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_PREDICTIVE_FAILURE          5
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_ERROR                       6
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_NON_RECOVERABLE_ERROR       7
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_STARTING                    8
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_STOPPING                    9
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_STOPPED                    10
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_IN_SERVICE                 11
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_NO_CONTACT                 12
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_LOST_COMMUNICATION         13
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_ABORTED                    14
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_DORMANT                    15
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_SUPPORTING_ENTITY_IN_ERROR 16
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_COMPLETED                  17
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_POWER_MODE                 18
#define CIM_CONCRETE_JOB_OPERATIONAL_STATUS_RELOCATING                 19

/**
 * Type of value that can be set to result property of job. This value is used
 * in CIM_InstMethodCall.ReturnValueType property of associated
 * LMI_MethodResult instance.
 */
typedef enum {
    METHOD_RESULT_VALUE_TYPE_BOOLEAN,
    METHOD_RESULT_VALUE_TYPE_STRING,
    METHOD_RESULT_VALUE_TYPE_CHAR16,
    METHOD_RESULT_VALUE_TYPE_UINT8,
    METHOD_RESULT_VALUE_TYPE_SINT8,
    METHOD_RESULT_VALUE_TYPE_UINT16,
    METHOD_RESULT_VALUE_TYPE_SINT16,
    METHOD_RESULT_VALUE_TYPE_UINT32,
    METHOD_RESULT_VALUE_TYPE_SINT32,
    METHOD_RESULT_VALUE_TYPE_UINT64,
    METHOD_RESULT_VALUE_TYPE_SINT64,
    METHOD_RESULT_VALUE_TYPE_DATETIME,
    METHOD_RESULT_VALUE_TYPE_REAL32,
    METHOD_RESULT_VALUE_TYPE_REAL64,
    METHOD_RESULT_VALUE_TYPE_REFERENCE,
    METHOD_RESULT_VALUE_TYPE_LAST,
}MethodResultValueTypeEnum;

/**
 * Callback that converts job instance to instance of `CIM_ConcreteJob`. Some
 * properties are already set. Modify them at will. This callback shall
 * be used at least to set `JobInParameters` and `JobOutParameters` properties.
 *
 * This callback is invoked anytime a new indication is about to be created and
 * sent.
 *
 * These properties will be already set:
 *
 *      * InstanceID
 *      * CommunicationStatus
 *      * DeleteOnCompletion
 *      * ElapsedTime
 *      * ElementName
 *      * ErrorCode
 *      * JobState
 *      * JobStatus
 *      * LocalOrUtcTime
 *      * MethodName
 *      * Name
 *      * PercentComplete
 *      * OperationalStatus
 *      * Priority
 *      * StartTime
 *      * TimeBeforeRemoval
 *      * TimeOfLastStateChange
 *      * TimeSubmitted
 */
typedef CMPIStatus (*JobToCimInstanceCallback) (const CMPIBroker *cb,
                                                const CMPIContext *ctx,
                                                const LmiJob *job,
                                                CMPIInstance *instance);

/**
 * Callback that fills properties of `__MethodResult_<MethodName>` instance
 * with regard to given job. This callback is called when making an instance of
 * `LMI_ConreteJob` to create values of properties `JobInParameters` and
 * `JobOutParameters`. And also when making an instance of `CIM_InstMethodCall`
 * to get a value of `MethodParameters` property. The latter is contained in
 * `PostCallIndication` and `PreCallIndication` properties of associated job
 * method result instance.
 *
 * @param include_input Whether the input parameters of asynchronous method
 *      shall be set.
 * @param include_output Whether the output parameters of asynchronous method
 *      shall be set.
 */
typedef CMPIStatus (*MakeJobParametersCallback) (const CMPIBroker *cb,
                                                 const CMPIContext *ctx,
                                                 const LmiJob *job,
                                                 gboolean include_input,
                                                 gboolean include_output,
                                                 CMPIInstance *instance);

/**
 * Callback invoked when job is run. It is a synchronous operation that:
 *
 *      1. starts an operation on back-end
 *      2. sets job's `jobid` obtained from back-end
 *      3. monitors the process and updates `percent-complete` property of job
 *      4. when the job is about to be successfully completed, its job
 *         output parameters need to be properly filled; this needs to be
 *         done before a call to `lmi_job_finish_ok()`
 *      5. calls one of `lmi_job_finish_ok()`, `lmi_job_finish_exception()`,
 *         `lmi_job_finish_terminate()`
 *
 * If *concurrent_processing* is on, multiple callbacks can be run
 * in parallel.
 *
 * @note This callback can be called twice or more times for the same job if
 *      the provider is restarted during its execution. Therefore you should
 *      check for job's status and especially jobid to see, whether this is
 *      a job that may have already finished.
 */
typedef void (*JobProcessCallback) (LmiJob *job, GCancellable *cancellable);

/**
 * Used in `jobmgr_init` to register job types.
 */
typedef struct _JobTypeEntry {
    GType job_type; 		/*!< Type representing a subclass of `LmiJob`
				 * that shall be handled by job manager. */
    const gchar *cim_class_name;/*!< Name of CIM class of generated CIM
			 * instances representing jobs. If `NULL`,
			 * `LMI_ConcreteJob` will be used. */
    JobToCimInstanceCallback convert_func; 	/*!< A callback that should set
			 * additional CIM instance's properties. Invoked
			 * right before an indication is sent. May be `NULL`. */
    MakeJobParametersCallback make_job_params_func;
                        /*!< A callback that should fill input and output
                         * parameters of job's asynchronous method to prepared
                         * method. May be `NULL`. */
    MethodResultValueTypeEnum return_value_type;
                        /*!< Type of value that can be set to result property
                         * of job. This value is used in
                         * CIM_InstMethodCall.ReturnValueType property of
                         * associated LMI_MethodResult instance. */
    JobProcessCallback process_func; 	/*!< A callback that should execute
			 * given job and finally set it to final state. */
    gboolean running_job_cancellable; 	/*!< Does a job support cancellation?
			 * If yes, *process_func* will be passed *cancellable*
			 * object that needs to be checked during job's
			 * processing for cancelled state. It enables the use
			 * of `jobmgr_terminate_job()` function. */
    gboolean use_persistent_storage; 	/*!< Whether to write job's state to
			 * file under persistent storage directory whenever it
			 * gets modified or created. Persistent storage is read
			 * upon job manager's initialization to populate
			 * internal structures with saved jobs. */
}JobTypeEntry;

/**
 * Initialize job manager. Call this function in your provider's initialization
 * function if you need to use job manager. Indication sender is initialized
 * by job manager as well.
 *
 * @note This needs to be called after `lmi_init()` and after all the job types
 *      are registered with `jobmgr_register_job_type()`.
 *
 * @param profile_name Base name of CIM profile or provider library. It must
 *      not contain any special characters because it is used as a directory
 *      name in job persistent storage path. For example if you implement
 *      *Software* provider, pass "Software" here. The same string is then used
 *      for indication filters registration with indication sender.
 * @param provider_name Name of provider e.g. "LMI_SoftwareInstallationService".
 *      This name must correspond with a *cb* object passed to this provider
 *      upon its initialization.
 * @param is_indication_provider Whether this provider implements CIM
 *      indication class. Such classes derive from `CIM_InstIndication` and
 *      typically `InstIndication` is a suffix of their names.
 * @param concurrent_processing Says whether multiple jobs can be run in
 *      parallel. If `false`, it will be ensured that at most one job can be in
 *      *RUNNING* state. The others will stay *NEW* until currently running job
 *      completes.
 * @param job_type_entries Optional job type information. If NULL or empty, it's
 *      expected that job types were already registered with
 *      `jobmgr_register_job_type()`.
 * @param n_job_types Number of entries in `job_type_entries` array.
 */
CMPIStatus jobmgr_init(const CMPIBroker *cb,
                       const CMPIContext *ctx,
                       const gchar *profile_name,
                       const gchar *provider_name,
                       gboolean is_indication_provider,
                       gboolean concurrent_processing,
                       const JobTypeEntry *job_type_entries,
                       gsize n_job_types);

/**
 * Cleanup job manager. Include this in you provider's cleanup handler if your
 * init called `jobmgr_init()`.
 *
 * @param provider_name The same name as passed to `jobmgr_init()`.
 */
CMPIStatus jobmgr_cleanup(const gchar *provider_name);

/**
 * Provider may define subclasses of `LmiJob` if it needs to differentiate
 * between jobs invoked from different CIM methods. Typical provider will
 * register just one `job_type` - `LmiJob` in particular.
 *
 * It also takes care of static filter registrations with indication sender.
 * @see ind_sender_add_static_filters().
 *
 * This shall be called before the `jobmgr_init()` at least once.
 * Any call done after the job manager is initialised will be ignored.
 *
 * @param job_type Type representing a subclass of `LmiJob` that shall be
 *      handled by job manager.
 * @param cim_class_name Name of CIM class of generated CIM instances
 *      representing jobs. If `NULL`, `LMI_ConcreteJob` will be used.
 * @param convert_func A callback that should set additional CIM instance's
 *      properties. Invoked right before an indication is sent.
 * @param make_job_params_func A callback that should fill input and output
 *      parameters of job's asynchronous method to prepaired method. May be
 *      `NULL`.
 * @param return_value_type Type of value that can be set to result property of
 *      job. This value is used in CIM_InstMethodCall.ReturnValueType property
 *      of associated LMI_MethodResult instance.
 * @param process_func A callback that should execute given job and finally
 *      set it to final state.
 * @param running_job_cancellable Does a job support cancellation? If yes,
 *     *process_func* will be passed *cancellable* object that needs to be checked
 *     during job's processing for cancelled state. It enables the use of
 *     `jobmgr_terminate_job()` function.
 * @param use_persistent_storage Whether to write job's state to file under
 *      persistent storage directory whenever it gets modified or created.
 *      Persistent storage is read upon job manager's initialization to populate
 *      internal structures with saved jobs.
 */
CMPIStatus jobmgr_register_job_type(GType job_type,
                                    const gchar *cim_class_name,
                                    JobToCimInstanceCallback convert_func,
                                    MakeJobParametersCallback make_job_params_func,
                                    MethodResultValueTypeEnum return_value_type,
                                    JobProcessCallback process_func,
                                    gboolean running_job_cancellable,
                                    gboolean use_persistent_storage);

/**
 * Create a new job. Caller should then set at least *method-name* property and
 * input parameters and then run it with `job_mgr_run_job()`.
 *
 * @note You should decrease its reference count with `g_object_unref()` after
 *      you are done with it.
 */
LmiJob *jobmgr_new_job(GType job_type);

/**
 * This needs to be called to run the newly created job after the essential
 * properties are set. Be sure not to modify job's state before calling this function.
 *
 * Depending on *concurrent_processing* option and number of pending jobs, given
 * job will be started either immediately or will be appended to pending ones.
 *
 * @note *method-name* and input parameters need to be set on given job before
 *      calling this function.
 */
CMPIStatus jobmgr_run_job(LmiJob *job);

/**
 * Get jobids of all known jobs.
 *
 * @note that jobid is not constant for the whole life of job.
 *  Prefer `jobmgr_get_job_numbers()`.
 *
 * Release the result with `g_strfreev()`.
 */
gchar **jobmgr_get_job_ids(guint *count);

/**
 * Get numbers of all known jobs.
 *
 * Release the result with `g_free()`.
 */
guint *jobmgr_get_job_numbers(guint *count);

/**
 * Get jobids of all pending jobs. Those in *NEW* state that were requested to
 * be run with `jobmgr_run_job()`.
 *
 * @note that jobid is not constant for the whole life of job.
 *  Prefer `jobmgr_get_job_pending_job_ids()`.
 *
 * Release the result with `g_strfreev()`.
 */
gchar **jobmgr_get_pending_job_ids(guint *count);

/**
 * Get numbers of all pending jobs. Those in *NEW* state that were requested to
 * be run with `jobmgr_run_job()`.
 *
 * Release the result with `g_free()`.
 */
guint *jobmgr_get_pending_job_numbers(guint *count);

/**
 * Get jobids of all running jobs. Those in *RUNNING* state.
 *
 * @note that jobid is not constant for the whole life of job.
 *  Prefer `jobmgr_get_job_running_job_ids()`.
 *
 * Release the result with `g_strfreev()`.
 */
gchar **jobmgr_get_running_job_ids(guint *count);

/**
 * Get numbers of all running jobs. Those in *RUNNING* state.
 *
 * Release the result with `g_free()`.
 */
guint *jobmgr_get_running_job_numbers(guint *count);

/**
 * Get a job with given `jobid`.
 *
 * @note You should decrease its reference count with `g_object_unref()` after
 *      you are done with it.
 */
LmiJob *jobmgr_get_job_by_id(const gchar *jobid);

/**
 * Get a job with given number.
 *
 * @note You should decrease its reference count with `g_object_unref()` after
 *      you are done with it.
 */
LmiJob *jobmgr_get_job_by_number(guint number);

/**
 * Get a job with given name.
 *
 * @note Name property is freely modifiable by client, thus it may not be
 *      unique. This function stops the search upon first matching job.
 *
 * @note You should decrease its reference count with `g_object_unref()` after
 *      you are done with it.
 */
LmiJob *jobmgr_get_job_by_name(const gchar *name);

/**
 * Delete the job. Only completed jobs can be deleted.
 *
 * @note Don't forget to unreference the job yourself after a call to this
 *      function.
 */
CMPIStatus jobmgr_delete_job(LmiJob *job);

/**
 * Terminate running job. This may succeed only if the *running_cancellable* flag
 * was set during job type's registration.
 */
CMPIStatus jobmgr_terminate_job(LmiJob *job);

/******************************************************************************
 * CIM related functionality
 *****************************************************************************/

/**
 * Get a CIM object path for a job.
 */
CMPIStatus jobmgr_job_to_cim_op(const LmiJob *job, CMPIObjectPath **op);

/**
 * Convert a job to CIM instance.
 *
 * This calls `convert_func()` callback provided during job type's registration
 * to fill additional properties.
 */
CMPIStatus jobmgr_job_to_cim_instance(const LmiJob *job,
                                      CMPIInstance **instance);

/**
 * Get an object path of `LMI_MethodResult` for given job.
 *
 * @param class_name: A desired cim class name of resulting method result
 *      instance. If `NULL`, it will be set to
 *      `LMI_<provider_name>MethodResult`.
 */
CMPIStatus jobmgr_job_to_method_result_op(const LmiJob *job,
                                          const gchar *class_name,
                                          CMPIObjectPath **op);

/**
 * Make an instance of `LMI_MethodResult` for given job.
 *
 * @param class_name: A desired cim class name of resulting method result
 *      instance. If `NULL`, it will be set to
 *      `LMI_<provider_name>MethodResult`.
 * @param instance: Pointer to a pointer where newly allocated method result
 *      instance will be stored.
 */
CMPIStatus jobmgr_job_to_method_result_instance(const LmiJob *job,
                                                const gchar *class_name,
                                                CMPIInstance **instance);

/**
 * Make a `CIM_Error` instance corresponding to failed job.
 *
 * If a job is not in `EXCEPTION` state, `instance` parameter won't be touched.
 */
CMPIStatus jobmgr_job_to_cim_error(const LmiJob *job, CMPIInstance **instance);

/**
 * Find and get job matching object path.
 */
CMPIStatus jobmgr_get_job_matching_op(const CMPIObjectPath *path, LmiJob **job);

#endif /* end of include guard: JOB_MANAGER_H */
