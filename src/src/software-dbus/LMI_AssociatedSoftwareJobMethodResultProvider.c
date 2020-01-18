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
#include "LMI_AssociatedSoftwareJobMethodResult.h"
#include "job_manager.h"
#include "sw-utils.h"

static const CMPIBroker* _cb;

static void LMI_AssociatedSoftwareJobMethodResultInitialize(
        const CMPIContext *ctx)
{
    software_init(LMI_AssociatedSoftwareJobMethodResult_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_AssociatedSoftwareJobMethodResult_ClassName);
}

static CMPIStatus enum_instances(const CMPIResult *cr, const char *ns,
        const short names)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIObjectPath *jop, *mrop;
    LmiJob *job = NULL;
    guint *jobnum;
    gchar *jobid = NULL;
    guint *jobnums = jobmgr_get_job_numbers(NULL);

    if (jobnums == NULL)
        goto err;

    for (jobnum = jobnums; *jobnum != 0; ++jobnum) {
        job = jobmgr_get_job_by_number(*jobnum);
        if (!job)
            /* job may have been deleted since numbers query */
            continue;

        if (lmi_job_get_method_name(job) == NULL) {
            /* Method result can not be created for job with MethodName unset.
             * Such a job is too new. */
            goto next;
        }

        jobid = lmi_job_get_jobid(job);

        status = jobmgr_job_to_cim_op(job, &jop);
        if (status.rc) {
            if (status.msg) {
                lmi_warn("Failed to make job object path out of job \"%s\": %s",
                               jobid, CMGetCharsPtr(status.msg, NULL));
            } else {
                lmi_warn("Failed to make job object path out of job \"%s\"",
                        jobid);
            }
            goto next;
        }

        status = jobmgr_job_to_method_result_op(job, NULL, &mrop);
        if (status.rc) {
            if (status.msg) {
                lmi_warn("Failed to make method result object path"
                       " out of job \"%s\": %s", jobid,
                       CMGetCharsPtr(status.msg, NULL));
            } else {
                lmi_warn("Failed to make method result object path"
                       " out of job \"%s\"", jobid);
            }
            CMRelease(jop);
            goto next;
        }

        LMI_AssociatedSoftwareJobMethodResult w;
        LMI_AssociatedSoftwareJobMethodResult_Init(&w, _cb, ns);
        LMI_AssociatedSoftwareJobMethodResult_SetObjectPath_Job(&w, jop);
        LMI_AssociatedSoftwareJobMethodResult_SetObjectPath_JobParameters(
                &w, mrop);

        if (names) {
            KReturnObjectPath(cr, w);
        } else {
            KReturnInstance(cr, w);
        }

next:
        g_free(jobid);
        jobid = NULL;
        g_clear_object(&job);
    }
    g_free(jobnums);

    return status;

err:
    if (!status.rc) {
        lmi_error("Memory allocation failed");
        KSetStatus(&status, ERR_FAILED);
    }
    return status;
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return enum_instances(cr, KNameSpace(cop), 1);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return enum_instances(cr, KNameSpace(cop), 0);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    gchar buf[BUFLEN];
    LmiJob *job;

    LMI_AssociatedSoftwareJobMethodResultRef ref;
    LMI_AssociatedSoftwareJobMethodResultRef_InitFromObjectPath(&ref, _cb, cop);

    if (!KHasValue(&ref.Job)) {
        KSetStatus2(_cb, &status, ERR_INVALID_PARAMETER, "Missing Job key property!");
        return status;
    }
    if (!KHasValue(&ref.JobParameters)) {
        KSetStatus2(_cb, &status, ERR_INVALID_PARAMETER,
                "Missing JobParameters property!");
        return status;
    }

    if ((status = jobmgr_get_job_matching_op(ref.Job.value, &job)).rc)
        return status;

    g_snprintf(buf, BUFLEN, SW_METHOD_RESULT_INSTANCE_ID_PREFIX "%u",
            lmi_job_get_number(job));
    g_object_unref(job);
    LMI_SoftwareMethodResultRef mr;
    LMI_SoftwareMethodResultRef_InitFromObjectPath(&mr, _cb, ref.JobParameters.value);
    if (g_ascii_strcasecmp(mr.InstanceID.chars, buf)) {
        KSetStatus2(_cb, &status, ERR_NOT_FOUND,
                "InstanceID of JobParameters does no match Job's InstanceID!");
        return status;
    }

    LMI_AssociatedSoftwareJobMethodResult w;
    LMI_AssociatedSoftwareJobMethodResult_InitFromObjectPath(&w, _cb, cop);

    KReturnInstance(cr, w);
    return status;
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultAssociators(
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
        LMI_AssociatedSoftwareJobMethodResult_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultAssociatorNames(
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
        LMI_AssociatedSoftwareJobMethodResult_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultReferences(
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
        LMI_AssociatedSoftwareJobMethodResult_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_AssociatedSoftwareJobMethodResultReferenceNames(
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
        LMI_AssociatedSoftwareJobMethodResult_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_AssociatedSoftwareJobMethodResult,
    LMI_AssociatedSoftwareJobMethodResult,
    _cb,
    LMI_AssociatedSoftwareJobMethodResultInitialize(ctx))

CMAssociationMIStub(
    LMI_AssociatedSoftwareJobMethodResult,
    LMI_AssociatedSoftwareJobMethodResult,
    _cb,
    LMI_AssociatedSoftwareJobMethodResultInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AssociatedSoftwareJobMethodResult",
    "LMI_AssociatedSoftwareJobMethodResult",
    "instance association")
