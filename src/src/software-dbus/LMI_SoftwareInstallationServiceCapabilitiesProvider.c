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
 * Authors: Peter Schiffer <pschiffe@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_SoftwareInstallationServiceCapabilities.h"
#include "LMI_SoftwareInstallationService.h"
#include "sw-utils.h"
#include "config.h"

static const CMPIBroker* _cb = NULL;

static void LMI_SoftwareInstallationServiceCapabilitiesInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SoftwareInstallationServiceCapabilities_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_SoftwareInstallationServiceCapabilitiesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_SoftwareInstallationServiceCapabilities_ClassName);
}

static CMPIStatus LMI_SoftwareInstallationServiceCapabilitiesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SoftwareInstallationServiceCapabilitiesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    char instance_id[BUFLEN], buf[BUFLEN];

    LMI_SoftwareInstallationServiceCapabilities w;
    LMI_SoftwareInstallationServiceCapabilities_Init(&w, _cb, KNameSpace(cop));

    LMI_SoftwareInstallationServiceCapabilities_Set_CanAddToCollection(&w, 1);
    LMI_SoftwareInstallationServiceCapabilities_Init_SupportedInstallOptions(&w, 3);
    LMI_SoftwareInstallationServiceCapabilities_Set_SupportedInstallOptions_Install(&w, 0);
    LMI_SoftwareInstallationServiceCapabilities_Set_SupportedInstallOptions_Update(&w, 1);
    LMI_SoftwareInstallationServiceCapabilities_Set_SupportedInstallOptions_Uninstall(&w, 2);
    LMI_SoftwareInstallationServiceCapabilities_Init_SupportedSynchronousActions(&w, 1);
    LMI_SoftwareInstallationServiceCapabilities_Set_SupportedSynchronousActions_None_supported(&w, 0);
    LMI_SoftwareInstallationServiceCapabilities_Init_SupportedTargetTypes(&w, 0);
    LMI_SoftwareInstallationServiceCapabilities_Init_SupportedURISchemes(&w, 3);
    LMI_SoftwareInstallationServiceCapabilities_Set_SupportedURISchemes_file(&w, 0);
    LMI_SoftwareInstallationServiceCapabilities_Set_SupportedURISchemes_http(&w, 1);
    LMI_SoftwareInstallationServiceCapabilities_Set_SupportedURISchemes_https(&w, 2);
    LMI_SoftwareInstallationServiceCapabilities_Set_ElementName(&w,
            "Software Installation Service Capabilities");

    create_instance_id(LMI_SoftwareInstallationServiceCapabilities_ClassName,
            NULL, instance_id, BUFLEN);
    LMI_SoftwareInstallationServiceCapabilities_Set_InstanceID(&w, instance_id);

    create_instance_id(LMI_SoftwareInstallationService_ClassName, NULL,
            instance_id, BUFLEN);
    snprintf(buf, BUFLEN, "Capabilities of %s", instance_id);
    LMI_SoftwareInstallationServiceCapabilities_Set_Caption(&w, buf);
    snprintf(buf, BUFLEN,
            "This instance provides information about capabilities of %s.",
            instance_id);
    LMI_SoftwareInstallationServiceCapabilities_Set_Description(&w, buf);

#ifndef RPM_FOUND
    LMI_SoftwareInstallationServiceCapabilities_Init_SupportedAsynchronousActions(&w, 2);
#else
    LMI_SoftwareInstallationServiceCapabilities_Init_SupportedAsynchronousActions(&w, 3);
    LMI_SoftwareInstallationServiceCapabilities_Set_SupportedAsynchronousActions_Verify_Software_Identity(&w, 2);
#endif
    LMI_SoftwareInstallationServiceCapabilities_Set_SupportedAsynchronousActions_Install_From_Software_Identity(&w, 0);
    LMI_SoftwareInstallationServiceCapabilities_Set_SupportedAsynchronousActions_Install_from_URI(&w, 1);

    LMI_SoftwareInstallationServiceCapabilities_Init_SupportedExtendedResourceTypes(&w, 1);
    gchar *backend_name = NULL;
    PkControl *control = pk_control_new();
    if (control && pk_control_get_properties(control, NULL, NULL)) {
        g_object_get(control, "backend-name", &backend_name, NULL);
        if (!backend_name || !*backend_name) {
            LMI_SoftwareInstallationServiceCapabilities_Set_SupportedExtendedResourceTypes_Unknown(&w, 0);
        } else if (strcmp(backend_name, "yum") == 0
                || strcmp(backend_name, "zypp") == 0
                || strcmp(backend_name, "urpmi") == 0
                || strcmp(backend_name, "hif") == 0
                || strcmp(backend_name, "poldek") == 0) {
            LMI_SoftwareInstallationServiceCapabilities_Set_SupportedExtendedResourceTypes_Linux_RPM(&w, 0);
        } else if (strcmp(backend_name, "apt") == 0
                || strcmp(backend_name, "aptcc") == 0) {
            LMI_SoftwareInstallationServiceCapabilities_Set_SupportedExtendedResourceTypes_Debian_linux_Package(&w, 0);
        } else {
            LMI_SoftwareInstallationServiceCapabilities_Set_SupportedExtendedResourceTypes_Other(&w, 0);
        }
        g_free(backend_name);
        backend_name = NULL;
        g_clear_object(&control);
    } else {
        LMI_SoftwareInstallationServiceCapabilities_Set_SupportedExtendedResourceTypes_Unknown(&w, 0);
    }

    KReturnInstance(cr, w);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareInstallationServiceCapabilitiesGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SoftwareInstallationServiceCapabilitiesCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareInstallationServiceCapabilitiesModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareInstallationServiceCapabilitiesDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareInstallationServiceCapabilitiesExecQuery(
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
    LMI_SoftwareInstallationServiceCapabilities,
    LMI_SoftwareInstallationServiceCapabilities,
    _cb,
    LMI_SoftwareInstallationServiceCapabilitiesInitialize(ctx))

static CMPIStatus LMI_SoftwareInstallationServiceCapabilitiesMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareInstallationServiceCapabilitiesInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SoftwareInstallationServiceCapabilities_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SoftwareInstallationServiceCapabilities,
    LMI_SoftwareInstallationServiceCapabilities,
    _cb,
    LMI_SoftwareInstallationServiceCapabilitiesInitialize(ctx))

KUint16 LMI_SoftwareInstallationServiceCapabilities_CreateGoalSettings(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareInstallationServiceCapabilitiesRef* self,
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
    "LMI_SoftwareInstallationServiceCapabilities",
    "LMI_SoftwareInstallationServiceCapabilities",
    "instance method")
