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

#include <konkret/konkret.h>
#include "LMI_SoftwareInstallationJob.h"
#include "CIM_Error.h"
#include "openlmi.h"
#include "sw-utils.h"
#include "job_manager.h"
#include "lmi_sw_job.h"

#define KILL_JOB_SUCCESS 0
#define GET_ERROR_SUCCESS 0
#define REQUESTED_STATE_TERMINATE 4
#define REQUEST_STATE_CHANGE_COMPLETED_WITH_NO_ERROR 0

static const CMPIBroker* _cb = NULL;

static void LMI_SoftwareInstallationJobInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SoftwareInstallationJob_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_SoftwareInstallationJobCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_SoftwareInstallationJob_ClassName);
}

static CMPIStatus LMI_SoftwareInstallationJobEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SoftwareInstallationJobEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    gchar *jobid;
    LmiJob *job;
    CMPIInstance *inst;
    guint *jobnum;
    guint *jobnums = jobmgr_get_job_numbers(NULL);

    if (jobnums == NULL)
        CMReturn(CMPI_RC_ERR_FAILED);

    for (jobnum = jobnums; *jobnum != 0; ++jobnum) {
        job = jobmgr_get_job_by_number(*jobnum);
        if (!job)
            // job may have been deleted since numbers query
            continue;

        if (!LMI_IS_SW_INSTALLATION_JOB(job)) {
            g_object_unref(job);
            continue;
        }

        status = jobmgr_job_to_cim_instance(job, &inst);
        g_object_unref(job);
        if (status.rc) {
            if (status.msg) {
                jobid = lmi_job_get_jobid(job);
                lmi_warn("Failed to make cim instance out of job \"%s\": %s",
                               jobid, CMGetCharsPtr(status.msg, NULL));
                g_free(jobid);
                continue;
            }
        }

        CMReturnInstance(cr, inst);
    }

    g_free(jobnums);

    KSetStatus(&status, OK);
    return status;
}

static CMPIStatus LMI_SoftwareInstallationJobGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LmiJob *job;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIInstance *inst = NULL;
    gchar *jobid = NULL;

    if ((status = jobmgr_get_job_matching_op(cop, &job)).rc)
        return status;

    status = jobmgr_job_to_cim_instance(job, &inst);
    g_object_unref(job);
    if (status.rc) {
        if (status.msg) {
            jobid = lmi_job_get_jobid(job);
            lmi_warn("Failed to make cim instance out of job \"%s\": %s",
                           jobid, CMGetCharsPtr(status.msg, NULL));
            g_free(jobid);
            return status;
        }
    }
    CMReturnInstance(cr, inst);
    return status;
}

static CMPIStatus LMI_SoftwareInstallationJobCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareInstallationJobModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    LmiJob *job;
    LMI_SoftwareInstallationJob lsij;
    guint64 tbr;
    CMPIStatus status = {CMPI_RC_OK, NULL};

    if ((status = jobmgr_get_job_matching_op(cop, &job)).rc)
        return status;

    LMI_SoftwareInstallationJob_InitFromInstance(&lsij, _cb, ci);
    if (KHasValue(&lsij.Name))
        lmi_job_set_name(job, lsij.Name.chars);
    if (KHasValue(&lsij.Priority))
        lmi_job_set_priority(job, lsij.Priority.value);
    if (KHasValue(&lsij.TimeBeforeRemoval)) {
        tbr = CMGetBinaryFormat(lsij.TimeBeforeRemoval.value, NULL)/1000000L;
        /* treat it always as an interval */
        lmi_job_set_time_before_removal(job, tbr);
    }
    if (KHasValue(&lsij.DeleteOnCompletion))
        lmi_job_set_delete_on_completion(job, lsij.DeleteOnCompletion.value);

    g_object_unref(job);
    return status;
}

static CMPIStatus LMI_SoftwareInstallationJobDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    LmiJob *job;
    CMPIStatus status = {CMPI_RC_OK, NULL};

    if ((status = jobmgr_get_job_matching_op(cop, &job)).rc)
        return status;
    status = jobmgr_delete_job(job);
    g_object_unref(job);
    return status;
}

static CMPIStatus LMI_SoftwareInstallationJobExecQuery(
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
    LMI_SoftwareInstallationJob,
    LMI_SoftwareInstallationJob,
    _cb,
    LMI_SoftwareInstallationJobInitialize(ctx))

static CMPIStatus LMI_SoftwareInstallationJobMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareInstallationJobInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SoftwareInstallationJob_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SoftwareInstallationJob,
    LMI_SoftwareInstallationJob,
    _cb,
    LMI_SoftwareInstallationJobInitialize(ctx))

KUint32 LMI_SoftwareInstallationJob_KillJob(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationJobRef* self,
    const KBoolean* DeleteOnKill,
    CMPIStatus* status)
{
    CMPIObjectPath *cop;
    LmiJob *job;
    KUint32 result = KUINT32_INIT;

    if ((cop = LMI_SoftwareInstallationJobRef_ToObjectPath(
                    self, status)) == NULL)
        return result;
    *status = jobmgr_get_job_matching_op(cop, &job);
    CMRelease(cop);
    if (status->rc)
        return result;
    *status = jobmgr_terminate_job(job);
    if (!status->rc) {
        if (KHasValue(DeleteOnKill) && DeleteOnKill->value)
            *status = jobmgr_delete_job(job);
    }
    g_object_unref(job);
    if (!status->rc)
        KUint32_Set(&result, KILL_JOB_SUCCESS);
    return result;
}

KUint32 LMI_SoftwareInstallationJob_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationJobRef* self,
    const KUint16* RequestedState,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    const char *errmsg = NULL;
    CMPIObjectPath *cop;
    LmiJob *job;
    KUint32 result = KUINT32_INIT;

    if (!KHasValue(RequestedState)) {
        errmsg = "Missing RequestedState parameter.";
        goto err;
    }
    if (RequestedState->value != REQUESTED_STATE_TERMINATE) {
        errmsg = "The only supported RequestedState is TERMINATE.";
        goto err;
    }
    if (KHasValue(TimeoutPeriod)) {
        errmsg = "TimeoutPeriod parameter is not supported.";
        goto err;
    }

    if ((cop = LMI_SoftwareInstallationJobRef_ToObjectPath(
                    self, status)) == NULL)
        goto err;
    *status = jobmgr_get_job_matching_op(cop, &job);
    CMRelease(cop);
    if (status->rc)
        goto err;
    *status = jobmgr_terminate_job(job);
    if (!status->rc)
        KUint32_Set(&result, REQUEST_STATE_CHANGE_COMPLETED_WITH_NO_ERROR);
    g_object_unref(job);

err:
    if (errmsg) {
        lmi_error(errmsg);
        KSetStatus2(cb, status, ERR_INVALID_PARAMETER, errmsg);
    }
    if (!status->rc)
        KUint32_Set(&result, REQUEST_STATE_CHANGE_COMPLETED_WITH_NO_ERROR);
    return result;
}

KUint32 LMI_SoftwareInstallationJob_GetError(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationJobRef* self,
    KInstance* Error,
    CMPIStatus* status)
{
    CMPIObjectPath *cop;
    LmiJob *job = NULL;
    CMPIInstance *inst = NULL;
    KUint32 result = KUINT32_INIT;

    if ((cop = LMI_SoftwareInstallationJobRef_ToObjectPath(
                    self, status)) == NULL)
        goto err;
    *status = jobmgr_get_job_matching_op(cop, &job);
    CMRelease(cop);
    if (status->rc) {
        KSetStatus(status, ERR_NOT_FOUND);
        goto done;
    }
    JOB_CRITICAL_BEGIN(job);
    if (lmi_job_get_state(job) == LMI_JOB_STATE_ENUM_EXCEPTION) {
        if ((*status = jobmgr_job_to_cim_error(job, &inst)).rc)
            goto critical_end_err;
        KInstance_Set(Error, inst);
    }
    JOB_CRITICAL_END(job);
    g_clear_object(&job);

    KUint32_Set(&result, GET_ERROR_SUCCESS);
    goto done;

critical_end_err:
    JOB_CRITICAL_END(job);
    g_clear_object(&job);
err:
    if (!status->rc) {
        lmi_error("Memory allocation failed");
        KSetStatus(status, ERR_FAILED);
    }
done:
    return result;
}

KUint32 LMI_SoftwareInstallationJob_GetErrors(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationJobRef* self,
    KInstanceA* Errors,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KInstance Error;
    KInstance_Null(&Error);
    LMI_SoftwareInstallationJob_GetError(cb, mi, context, self, &Error, status);
    if (status->rc)
        return result;
    if (KHasValue(&Error)) {
        KInstanceA_Init(Errors, cb, 1);
        KInstanceA_Set(Errors, 0, (CMPIInstance *) Error.value);
    } else {
        KInstanceA_Init(Errors, cb, 0);
    }
    KUint32_Set(&result, GET_ERROR_SUCCESS);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SoftwareInstallationJob",
    "LMI_SoftwareInstallationJob",
    "instance method")
