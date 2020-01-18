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
#include "LMI_AssociatedSoftwareInstallationServiceCapabilities.h"
#include "sw-utils.h"

static const CMPIBroker* _cb;

static void LMI_AssociatedSoftwareInstallationServiceCapabilitiesInitialize(const CMPIContext *ctx)
{
    software_init(LMI_AssociatedSoftwareInstallationServiceCapabilities_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(
            LMI_AssociatedSoftwareInstallationServiceCapabilities_ClassName);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    char instance_id[BUFLEN];

    create_instance_id(LMI_SoftwareInstallationServiceCapabilities_ClassName,
            NULL, instance_id, BUFLEN);

    LMI_SoftwareInstallationServiceCapabilitiesRef sisc;
    LMI_SoftwareInstallationServiceCapabilitiesRef_Init(&sisc, _cb, KNameSpace(cop));
    LMI_SoftwareInstallationServiceCapabilitiesRef_Set_InstanceID(&sisc, instance_id);

    create_instance_id(LMI_SoftwareInstallationService_ClassName,
            NULL, instance_id, BUFLEN);

    LMI_SoftwareInstallationServiceRef sis;
    LMI_SoftwareInstallationServiceRef_Init(&sis, _cb, KNameSpace(cop));
    LMI_SoftwareInstallationServiceRef_Set_CreationClassName(&sis,
            LMI_SoftwareInstallationService_ClassName);
    LMI_SoftwareInstallationServiceRef_Set_SystemCreationClassName(&sis,
            lmi_get_system_creation_class_name());
    LMI_SoftwareInstallationServiceRef_Set_SystemName(&sis,
            lmi_get_system_name_safe(cc));
    LMI_SoftwareInstallationServiceRef_Set_Name(&sis, instance_id);

    LMI_AssociatedSoftwareInstallationServiceCapabilities w;
    LMI_AssociatedSoftwareInstallationServiceCapabilities_Init(&w, _cb, KNameSpace(cop));
    LMI_AssociatedSoftwareInstallationServiceCapabilities_Set_Capabilities(&w, &sisc);
    LMI_AssociatedSoftwareInstallationServiceCapabilities_Set_ManagedElement(&w, &sis);
    LMI_AssociatedSoftwareInstallationServiceCapabilities_Init_Characteristics(&w, 2);
    LMI_AssociatedSoftwareInstallationServiceCapabilities_Set_Characteristics_Default(&w, 0);
    LMI_AssociatedSoftwareInstallationServiceCapabilities_Set_Characteristics_Current(&w, 1);

    KReturnInstance(cr, w);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesAssociators(
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
        LMI_AssociatedSoftwareInstallationServiceCapabilities_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesAssociatorNames(
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
        LMI_AssociatedSoftwareInstallationServiceCapabilities_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesReferences(
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
        LMI_AssociatedSoftwareInstallationServiceCapabilities_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_AssociatedSoftwareInstallationServiceCapabilitiesReferenceNames(
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
        LMI_AssociatedSoftwareInstallationServiceCapabilities_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_AssociatedSoftwareInstallationServiceCapabilities,
    LMI_AssociatedSoftwareInstallationServiceCapabilities,
    _cb,
    LMI_AssociatedSoftwareInstallationServiceCapabilitiesInitialize(ctx))

CMAssociationMIStub(
    LMI_AssociatedSoftwareInstallationServiceCapabilities,
    LMI_AssociatedSoftwareInstallationServiceCapabilities,
    _cb,
    LMI_AssociatedSoftwareInstallationServiceCapabilitiesInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AssociatedSoftwareInstallationServiceCapabilities",
    "LMI_AssociatedSoftwareInstallationServiceCapabilities",
    "instance association")
