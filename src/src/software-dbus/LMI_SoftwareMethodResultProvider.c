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
#include "LMI_SoftwareMethodResult.h"
#include "lmi_job.h"
#include "job_manager.h"
#include "sw-utils.h"

static const CMPIBroker* _cb = NULL;

static void LMI_SoftwareMethodResultInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SoftwareMethodResult_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_SoftwareMethodResultCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_SoftwareMethodResult_ClassName);
}

static CMPIStatus LMI_SoftwareMethodResultEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SoftwareMethodResultEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
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

        status = jobmgr_job_to_method_result_instance(job, NULL, &inst);
        if (status.rc) {
            gchar *jobid = lmi_job_get_jobid(job);
            if (status.msg) {
                lmi_warn("Failed to make method result instance out of job"
                       " \"%s\": %s", jobid, CMGetCharsPtr(status.msg, NULL));
            } else {
                lmi_warn("Failed to make method result instance out of job"
                       " \"%s\"", jobid);
            }
            g_free(jobid);
        } else {
            CMReturnInstance(cr, inst);
        }
        g_clear_object(&job);
    }

    g_free(jobnums);

    return status;
}

static CMPIStatus LMI_SoftwareMethodResultGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LmiJob *job = NULL;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIInstance *inst;
    guint64 number;
    gchar *endptr;
    gchar errbuf[BUFLEN] = "";
    LMI_SoftwareMethodResultRef smrop;

    LMI_SoftwareMethodResultRef_InitFromObjectPath(&smrop, _cb, cop);
    if (!KHasValue(&smrop.InstanceID)) {
        g_snprintf(errbuf, BUFLEN, "Missing InstanceID key property!");
        goto err;
    }

    if (g_ascii_strncasecmp(SW_METHOD_RESULT_INSTANCE_ID_PREFIX,
                smrop.InstanceID.chars,
                SW_METHOD_RESULT_INSTANCE_ID_PREFIX_LEN))
    {
        g_snprintf(errbuf, BUFLEN, "Unexpected prefix \"%s\" in InstanceID!",
                smrop.InstanceID.chars);
        goto err;
    }

    number = g_ascii_strtoull(smrop.InstanceID.chars
                    + SW_METHOD_RESULT_INSTANCE_ID_PREFIX_LEN, &endptr, 10);
    if (number == G_MAXUINT64 || number == 0 || number > (guint64) G_MAXINT32) {
        g_snprintf(errbuf, BUFLEN, "Missing or invalid method result number in"
               " InstanceID \"%s\"!", smrop.InstanceID.chars);
        goto err;
    }

    if (*endptr != '\0') {
        g_snprintf(errbuf, BUFLEN,
                "Junk after job number in method result's InstanceID \"%s\"!",
                smrop.InstanceID.chars);
        goto err;
    }

    if ((job = jobmgr_get_job_by_number(number)) == NULL) {
        g_snprintf(errbuf, BUFLEN, "Job #%u does not exist.", (guint32) number);
        goto err;
    }

    status = jobmgr_job_to_method_result_instance(job, NULL, &inst);
    if (status.rc) {
        gchar *jobid = lmi_job_get_jobid(job);
        if (status.msg) {
            g_snprintf(errbuf, BUFLEN, "Failed to make method result instance"
                   " out of job \"%s\": %s", jobid,
                   CMGetCharsPtr(status.msg, NULL));
        } else {
            g_snprintf(errbuf, BUFLEN, "Failed to make method result instance"
                   " out of job \"%s\"", jobid);
        }
        g_free(jobid);
        goto err;
    }
    g_object_unref(job);
    CMReturnInstance(cr, inst);
    return status;

err:
    g_clear_object(&job);
    if (errbuf[0]) {
        lmi_error(errbuf);
        if (!status.rc)
            KSetStatus2(_cb, &status, ERR_NOT_FOUND, errbuf);
    } else if (!status.rc) {
        KSetStatus(&status, ERR_NOT_FOUND);
    }
    return status;
}

static CMPIStatus LMI_SoftwareMethodResultCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareMethodResultModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareMethodResultDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareMethodResultExecQuery(
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
    LMI_SoftwareMethodResult,
    LMI_SoftwareMethodResult,
    _cb,
    LMI_SoftwareMethodResultInitialize(ctx))

static CMPIStatus LMI_SoftwareMethodResultMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareMethodResultInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SoftwareMethodResult_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SoftwareMethodResult,
    LMI_SoftwareMethodResult,
    _cb,
    LMI_SoftwareMethodResultInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SoftwareMethodResult",
    "LMI_SoftwareMethodResult",
    "instance method")
