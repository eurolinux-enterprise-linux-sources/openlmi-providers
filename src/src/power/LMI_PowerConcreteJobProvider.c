/*
 * Copyright (C) 2012-2013 Red Hat, Inc.  All rights reserved.
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
 * Authors: Radek Novacek <rnovacek@redhat.com>
 */

#include <konkret/konkret.h>
#include <stdint.h>
#include "LMI_PowerConcreteJob.h"
#include "globals.h"

static const CMPIBroker* _cb = NULL;

#include "power.h"

static void LMI_PowerConcreteJobInitializeInstance(CMPIInstanceMI *mi,
        const CMPIContext *ctx)
{
    mi->hdl = power_ref(_cb, ctx);
}

static void LMI_PowerConcreteJobInitializeMethod(CMPIMethodMI *mi,
        const CMPIContext *ctx)
{
    mi->hdl = power_ref(_cb, ctx);
}

static CMPIStatus LMI_PowerConcreteJobCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    if (power_get_jobs(mi->hdl) != NULL) {
        // We have jobs running -> do not unload
        CMReturn(CMPI_RC_DO_NOT_UNLOAD);
    }
    power_unref(mi->hdl);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PowerConcreteJobEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PowerConcreteJobEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus status;
    const char *ns = KNameSpace(cop);

    PowerStateChangeJob *powerStateChangeJob;
    GList *plist = power_get_jobs(mi->hdl);
    char *instanceid;

    while (plist) {
        powerStateChangeJob = plist->data;
        LMI_PowerConcreteJob concreteJob;
        LMI_PowerConcreteJob_Init(&concreteJob, _cb, ns);
        if (asprintf(&instanceid, ORGID ":LMI_PowerConcreteJob:%ld", job_id(powerStateChangeJob)) < 0) {
            KReturn2(_cb, ERR_FAILED, "Memory allocation failed");
        }
        LMI_PowerConcreteJob_Set_InstanceID(&concreteJob, instanceid);
        free(instanceid);
        LMI_PowerConcreteJob_Set_JobState(&concreteJob, job_state(powerStateChangeJob));
        LMI_PowerConcreteJob_Set_TimeOfLastStateChange(&concreteJob, CMNewDateTimeFromBinary(_cb, ((uint64_t) job_timeOfLastChange(powerStateChangeJob)) * 1000000, 0, &status));

        KReturnInstance(cr, concreteJob);
        plist = g_list_next(plist);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PowerConcreteJobGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PowerConcreteJobCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PowerConcreteJobModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PowerConcreteJobDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PowerConcreteJobExecQuery(
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
    LMI_PowerConcreteJob,
    LMI_PowerConcreteJob,
    _cb,
    LMI_PowerConcreteJobInitializeInstance(&mi, ctx))

static CMPIStatus LMI_PowerConcreteJobMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PowerConcreteJobInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_PowerConcreteJob_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_PowerConcreteJob,
    LMI_PowerConcreteJob,
    _cb,
    LMI_PowerConcreteJobInitializeMethod(&mi, ctx))

KUint32 LMI_PowerConcreteJob_KillJob(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerConcreteJobRef* self,
    const KBoolean* DeleteOnKill,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PowerConcreteJob_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerConcreteJobRef* self,
    const KUint16* RequestedState,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PowerConcreteJob_GetError(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerConcreteJobRef* self,
    KInstance* Error,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PowerConcreteJob_GetErrors(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerConcreteJobRef* self,
    KInstanceA* Errors,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PowerConcreteJob_ResumeWithAction(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerConcreteJobRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PowerConcreteJob_ResumeWithInput(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PowerConcreteJobRef* self,
    const KStringA* Inputs,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PowerConcreteJob",
    "LMI_PowerConcreteJob",
    "instance method")
