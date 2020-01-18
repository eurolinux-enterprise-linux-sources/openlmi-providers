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
#include "LMI_AffectedSoftwareJobElement.h"
#include "LMI_SystemSoftwareCollection.h"
#include "sw-utils.h"
#include "lmi_sw_job.h"
#include "job_manager.h"

static const CMPIBroker* _cb;

static void LMI_AffectedSoftwareJobElementInitialize(const CMPIContext *ctx)
{
    software_init(LMI_AffectedSoftwareJobElement_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_AffectedSoftwareJobElementCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_AffectedSoftwareJobElement_ClassName);
}

static CMPIStatus LMI_AffectedSoftwareJobElementEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus fill_inst(LMI_AffectedSoftwareJobElement *inst,
                            const gchar *namespace,
                            CMPIObjectPath *job,
                            CMPIObjectPath *affected_element,
                            const gchar *description)
{
    LMI_AffectedSoftwareJobElement_Init(inst, _cb, namespace);
    LMI_AffectedSoftwareJobElement_SetObjectPath_AffectingElement(inst, job);
    LMI_AffectedSoftwareJobElement_Init_ElementEffects(inst, 1);
    LMI_AffectedSoftwareJobElement_Set_ElementEffects_Other(inst, 0);
    if (description != NULL) {
        LMI_AffectedSoftwareJobElement_Init_OtherElementEffectsDescriptions(inst, 1);
        LMI_AffectedSoftwareJobElement_Set_OtherElementEffectsDescriptions(
            inst, 0, description);
    } else {
        LMI_AffectedSoftwareJobElement_Init_OtherElementEffectsDescriptions(inst, 0);
    }
    LMI_AffectedSoftwareJobElement_SetObjectPath_AffectedElement(
            inst, affected_element);
    KReturn(OK);
}


static CMPIStatus LMI_AffectedSoftwareJobElementEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    LmiJob *job = NULL;
    CMPIObjectPath *jop;
    CMPIObjectPath *op;
    guint *jobnum;
    guint *jobnums = jobmgr_get_job_numbers(NULL);
    gchar *description;
    char instance_id[BUFLEN] = "";

    if (jobnums == NULL)
        CMReturn(CMPI_RC_ERR_FAILED);

    for (jobnum = jobnums; *jobnum != 0; ++jobnum) {
        job = jobmgr_get_job_by_number(*jobnum);
        if (!job)
            /* job may have been deleted since numbers query */
            continue;

        JOB_CRITICAL_BEGIN(job);

        status = jobmgr_job_to_cim_op(job, &jop);
        if (status.rc) {
            gchar *jobid = lmi_job_get_jobid(job);
            if (status.msg) {
                lmi_warn("Failed to make cim instance out of job \"%s\": %s",
                               jobid, CMGetCharsPtr(status.msg, NULL));
            } else {
                lmi_warn("Failed to make cim instance out of job \"%s\".",
                               jobid);
            }
            g_free(jobid);
            goto job_critical_end;
        }

        description = NULL;
        if (  LMI_IS_SW_INSTALLATION_JOB(job)
           && lmi_job_has_in_param(job, IN_PARAM_INSTALL_OPTIONS_NAME))
        {
            GVariant *variant = lmi_job_get_in_param(job,
                    IN_PARAM_INSTALL_OPTIONS_NAME);
            guint32 iopts = g_variant_get_uint32(variant);
            g_variant_unref(variant);
            if (iopts & INSTALLATION_OPERATION_INSTALL) {
                description = "Installing";
            } else if (iopts & INSTALLATION_OPERATION_UPDATE) {
                description = "Updating";
            } else if (iopts & INSTALLATION_OPERATION_REMOVE) {
                description = "Removing";
            } else {
                description = "Modifying";
            }
        } else if (LMI_IS_SW_VERIFICATION_JOB(job)) {
            description = "Verifying";
        }

        if (LMI_IS_SW_INSTALLATION_JOB(job)) {
            /* associate to SystemSoftwareCollection */
            create_instance_id(LMI_SystemSoftwareCollection_ClassName,
                    NULL, instance_id, BUFLEN);

            LMI_SystemSoftwareCollectionRef sc;
            LMI_SystemSoftwareCollectionRef_Init(&sc, _cb, KNameSpace(cop));
            LMI_SystemSoftwareCollectionRef_Set_InstanceID(&sc, instance_id);
            op = LMI_SystemSoftwareCollectionRef_ToObjectPath(&sc, &status);
            if (status.rc)
                goto mem_err;

            LMI_AffectedSoftwareJobElement w;
            status = fill_inst(&w, KNameSpace(cop), jop, op, description);
            if (status.rc) {
                CMRelease(op);
                goto mem_err;
            }
            KReturnInstance(cr, w);
        }

        {   /* associate to ComputerSystem */
            LMI_AffectedSoftwareJobElement w;
            status = fill_inst(&w, KNameSpace(cop), jop,
                    lmi_get_computer_system_safe(cc), description);
            if (status.rc)
                goto mem_err;
            KReturnInstance(cr, w);
        }

        if (lmi_job_has_out_param(job, OUT_PARAM_AFFECTED_PACKAGES)) {
            /* associate to affected packages */
            GVariant *affected_packages = lmi_job_get_out_param(
                    job, OUT_PARAM_AFFECTED_PACKAGES);
            GVariantIter iter;
            gchar *pkg_id = NULL;
            SwPackage sw_pkg;
            g_variant_iter_init(&iter, affected_packages);
            gchar buf[BUFLEN];
            while (g_variant_iter_next(&iter, "&s", &pkg_id)) {
                init_sw_package(&sw_pkg);
                if (create_sw_package_from_pk_pkg_id(pkg_id, &sw_pkg) != 0)
                    continue;
                sw_pkg_get_element_name(&sw_pkg, buf, BUFLEN);
                create_instance_id(LMI_SoftwareIdentity_ClassName, buf,
                        instance_id, BUFLEN);
                clean_sw_package(&sw_pkg);

                LMI_SoftwareIdentityRef sir;
                LMI_SoftwareIdentityRef_Init(&sir, _cb, KNameSpace(cop));
                LMI_SoftwareIdentityRef_Set_InstanceID(&sir, instance_id);
                op = LMI_SoftwareIdentityRef_ToObjectPath(&sir, &status);
                if (status.rc)
                    goto mem_err;

                LMI_AffectedSoftwareJobElement w;
                status = fill_inst(&w, KNameSpace(cop), jop, op, description);
                if (status.rc) {
                    CMRelease(op);
                    goto mem_err;
                }
                KReturnInstance(cr, w);
            }
            g_variant_unref(affected_packages);
        }

job_critical_end:
        JOB_CRITICAL_END(job);
        g_clear_object(&job);
    }

    g_free(jobnums);
    return status;

mem_err:
    if (job != NULL) {
        JOB_CRITICAL_END(job);
        g_object_unref(job);
    }
    if (status.msg != NULL) {
        lmi_error(CMGetCharsPtr(status.msg, NULL));
    } else {
        lmi_error("Memory allocation failed");
    }
    g_free(jobnums);
    return status;
}

static CMPIStatus LMI_AffectedSoftwareJobElementGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_AffectedSoftwareJobElementCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AffectedSoftwareJobElementModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AffectedSoftwareJobElementDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AffectedSoftwareJobElementExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AffectedSoftwareJobElementAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AffectedSoftwareJobElementAssociators(
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
        LMI_AffectedSoftwareJobElement_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_AffectedSoftwareJobElementAssociatorNames(
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
        LMI_AffectedSoftwareJobElement_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_AffectedSoftwareJobElementReferences(
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
        LMI_AffectedSoftwareJobElement_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_AffectedSoftwareJobElementReferenceNames(
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
        LMI_AffectedSoftwareJobElement_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_AffectedSoftwareJobElement,
    LMI_AffectedSoftwareJobElement,
    _cb,
    LMI_AffectedSoftwareJobElementInitialize(ctx))

CMAssociationMIStub(
    LMI_AffectedSoftwareJobElement,
    LMI_AffectedSoftwareJobElement,
    _cb,
    LMI_AffectedSoftwareJobElementInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AffectedSoftwareJobElement",
    "LMI_AffectedSoftwareJobElement",
    "instance association")
