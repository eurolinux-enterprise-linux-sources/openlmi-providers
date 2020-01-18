/*
 * Copyright (C) 2013 Red Hat, Inc. All rights reserved.
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
 */

#include <konkret/konkret.h>
#include "LMI_ProcessorCapabilities.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"
#include "lscpu.h"

static const CMPIBroker* _cb = NULL;

static void LMI_ProcessorCapabilitiesInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_ProcessorCapabilitiesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorCapabilitiesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ProcessorCapabilitiesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_ProcessorCapabilities lmi_cpu_cap;
    CMPIUint16 cores = 1, threads = 1;
    const char *ns = KNameSpace(cop),
            *element_name_string = "Capabilities of processor ";
    char *error_msg = NULL, instance_id[INSTANCE_ID_LEN],
            element_name[ELEMENT_NAME_LEN];
    unsigned i, cpus_nb = 0;
    DmiProcessor *dmi_cpus = NULL;
    unsigned dmi_cpus_nb = 0;
    LscpuProcessor lscpu;

    if (dmi_get_processors(&dmi_cpus, &dmi_cpus_nb) != 0 || dmi_cpus_nb < 1) {
        dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);

        if (lscpu_get_processor(&lscpu) != 0) {
            error_msg = "Unable to get processor information.";
            goto done;
        }
    }

    if (dmi_cpus_nb > 0) {
        cpus_nb = dmi_cpus_nb;
    } else if (lscpu.processors > 0) {
        cpus_nb = lscpu.processors;
    } else {
        error_msg = "Unable to get processor information.";
        goto done;
    }

    for (i = 0; i < cpus_nb; i++) {
        LMI_ProcessorCapabilities_Init(&lmi_cpu_cap, _cb, ns);

        /* do we have output from dmidecode program? */
        if (dmi_cpus_nb > 0) {
            snprintf(instance_id, INSTANCE_ID_LEN,
                    ORGID ":" ORGID "_" CPU_CAP_CLASS_NAME ":%s",
                    dmi_cpus[i].id);
            snprintf(element_name, ELEMENT_NAME_LEN, "%s%s",
                    element_name_string, dmi_cpus[i].id);
            cores = dmi_cpus[i].cores;
            threads = dmi_cpus[i].threads;
        } else {
            snprintf(instance_id, INSTANCE_ID_LEN,
                    ORGID ":" ORGID "_" CPU_CAP_CLASS_NAME ":%u", i);
            snprintf(element_name, ELEMENT_NAME_LEN, "%s%u",
                    element_name_string, i);
            cores = lscpu.cores;
            threads = lscpu.threads_per_core * lscpu.cores;
        }

        LMI_ProcessorCapabilities_Set_InstanceID(&lmi_cpu_cap, instance_id);
        LMI_ProcessorCapabilities_Set_NumberOfProcessorCores(&lmi_cpu_cap,
                cores);
        LMI_ProcessorCapabilities_Set_NumberOfHardwareThreads(&lmi_cpu_cap,
                threads);
        LMI_ProcessorCapabilities_Set_ElementNameEditSupported(&lmi_cpu_cap, 0);
        LMI_ProcessorCapabilities_Set_Caption(&lmi_cpu_cap,
                "Processor Capabilities");
        LMI_ProcessorCapabilities_Set_Description(&lmi_cpu_cap,
                "This object represents (mainly multi-core and multi-thread) "
                "capabilities of processor in system.");
        LMI_ProcessorCapabilities_Set_ElementName(&lmi_cpu_cap, element_name);

        KReturnInstance(cr, lmi_cpu_cap);
    }

done:
    /* free lscpu only if it was used */
    if (dmi_cpus_nb < 1) {
        lscpu_free_processor(&lscpu);
    }
    dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);

    if (error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorCapabilitiesGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ProcessorCapabilitiesCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorCapabilitiesModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorCapabilitiesDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorCapabilitiesExecQuery(
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
    LMI_ProcessorCapabilities,
    LMI_ProcessorCapabilities,
    _cb,
    LMI_ProcessorCapabilitiesInitialize(ctx))

static CMPIStatus LMI_ProcessorCapabilitiesMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorCapabilitiesInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_ProcessorCapabilities_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_ProcessorCapabilities,
    LMI_ProcessorCapabilities,
    _cb,
    LMI_ProcessorCapabilitiesInitialize(ctx))

KUint16 LMI_ProcessorCapabilities_CreateGoalSettings(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorCapabilitiesRef* self,
    const KInstanceA* TemplateGoalSettings,
    KInstanceA* SupportedGoalSettings,
    CMPIStatus* status)
{
    KUint16 result = KUINT16_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ProcessorCapabilities",
    "LMI_ProcessorCapabilities",
    "instance method")
