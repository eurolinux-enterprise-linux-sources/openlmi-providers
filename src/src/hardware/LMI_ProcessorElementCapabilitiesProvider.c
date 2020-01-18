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
#include "LMI_ProcessorElementCapabilities.h"
#include "LMI_ProcessorCapabilities.h"
#include "LMI_Processor.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"
#include "lscpu.h"

static const CMPIBroker* _cb;

static void LMI_ProcessorElementCapabilitiesInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_ProcessorElementCapabilities lmi_cpu_el_cap;
    LMI_ProcessorCapabilitiesRef lmi_cpu_cap;
    LMI_ProcessorRef lmi_cpu;
    const char *ns = KNameSpace(cop);
    char *error_msg = NULL, instance_id[INSTANCE_ID_LEN];
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
        LMI_ProcessorElementCapabilities_Init(&lmi_cpu_el_cap, _cb, ns);

        LMI_ProcessorRef_Init(&lmi_cpu, _cb, ns);
        LMI_ProcessorRef_Set_SystemCreationClassName(&lmi_cpu,
                get_system_creation_class_name());
        LMI_ProcessorRef_Set_SystemName(&lmi_cpu, get_system_name());
        LMI_ProcessorRef_Set_CreationClassName(&lmi_cpu,
                ORGID "_" CPU_CLASS_NAME);

        LMI_ProcessorCapabilitiesRef_Init(&lmi_cpu_cap, _cb, ns);

        /* do we have output from dmidecode program? */
        if (dmi_cpus_nb > 0) {
            LMI_ProcessorRef_Set_DeviceID(&lmi_cpu, dmi_cpus[i].id);

            snprintf(instance_id, INSTANCE_ID_LEN,
                    ORGID ":" ORGID "_" CPU_CAP_CLASS_NAME ":%s",
                    dmi_cpus[i].id);
        } else {
            char cpu_id[LONG_INT_LEN];
            snprintf(cpu_id, LONG_INT_LEN, "%u", i);
            LMI_ProcessorRef_Set_DeviceID(&lmi_cpu, cpu_id);

            snprintf(instance_id, INSTANCE_ID_LEN,
                    ORGID ":" ORGID "_" CPU_CAP_CLASS_NAME ":%u", i);
        }

        LMI_ProcessorCapabilitiesRef_Set_InstanceID(&lmi_cpu_cap, instance_id);

        LMI_ProcessorElementCapabilities_Set_ManagedElement(&lmi_cpu_el_cap,
                &lmi_cpu);
        LMI_ProcessorElementCapabilities_Set_Capabilities(&lmi_cpu_el_cap,
                &lmi_cpu_cap);
        LMI_ProcessorElementCapabilities_Init_Characteristics(
                &lmi_cpu_el_cap, 1);
        LMI_ProcessorElementCapabilities_Set_Characteristics(&lmi_cpu_el_cap,
                0, LMI_ProcessorElementCapabilities_Characteristics_Current);

        KReturnInstance(cr, lmi_cpu_el_cap);
    }

done:
    /* free lscpu only if it was used */
    if (dmi_cpus_nb < 1) {
        lscpu_free_processor(&lscpu);
    }
    dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);

    if (error_msg) {
        KReturn2(_cb, ERR_FAILED, error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesAssociators(
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
        LMI_ProcessorElementCapabilities_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesAssociatorNames(
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
        LMI_ProcessorElementCapabilities_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesReferences(
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
        LMI_ProcessorElementCapabilities_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_ProcessorElementCapabilitiesReferenceNames(
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
        LMI_ProcessorElementCapabilities_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_ProcessorElementCapabilities,
    LMI_ProcessorElementCapabilities,
    _cb,
    LMI_ProcessorElementCapabilitiesInitialize(ctx))

CMAssociationMIStub( 
    LMI_ProcessorElementCapabilities,
    LMI_ProcessorElementCapabilities,
    _cb,
    LMI_ProcessorElementCapabilitiesInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ProcessorElementCapabilities",
    "LMI_ProcessorElementCapabilities",
    "instance association")
