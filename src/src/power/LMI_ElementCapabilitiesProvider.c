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
#include "LMI_ElementCapabilities.h"
#include "globals.h"

static const CMPIBroker* _cb;

static void LMI_ElementCapabilitiesInitialize(const CMPIContext *ctx)
{
}

static CMPIStatus LMI_ElementCapabilitiesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ElementCapabilitiesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ElementCapabilitiesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    const char *ns = KNameSpace(cop);

    LMI_ElementCapabilities w;
    LMI_ElementCapabilities_Init(&w, _cb, ns);

    LMI_PowerManagementServiceRef powerManagementServiceRef;
    LMI_PowerManagementServiceRef_Init(&powerManagementServiceRef, _cb, ns);
    LMI_PowerManagementServiceRef_Set_Name(&powerManagementServiceRef, get_system_name());
    LMI_PowerManagementServiceRef_Set_SystemName(&powerManagementServiceRef, get_system_name());
    LMI_PowerManagementServiceRef_Set_CreationClassName(&powerManagementServiceRef, "LMI_PowerManagementService");
    LMI_PowerManagementServiceRef_Set_SystemCreationClassName(&powerManagementServiceRef, get_system_creation_class_name());

    LMI_ElementCapabilities_Set_ManagedElement(&w, &powerManagementServiceRef);

    LMI_PowerManagementCapabilitiesRef powerManagementCapabilitiesRef;
    LMI_PowerManagementCapabilitiesRef_Init(&powerManagementCapabilitiesRef, _cb, ns);
    LMI_PowerManagementCapabilitiesRef_Set_InstanceID(&powerManagementCapabilitiesRef, ORGID ":LMI_PowerManagementCapabilities");

    LMI_ElementCapabilities_Set_Capabilities(&w, &powerManagementCapabilitiesRef);

    KReturnInstance(cr, w);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ElementCapabilitiesGetInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ElementCapabilitiesCreateInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ElementCapabilitiesModifyInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ElementCapabilitiesDeleteInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ElementCapabilitiesExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ElementCapabilitiesAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ElementCapabilitiesAssociators(
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
        LMI_ElementCapabilities_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_ElementCapabilitiesAssociatorNames(
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
        LMI_ElementCapabilities_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_ElementCapabilitiesReferences(
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
        LMI_ElementCapabilities_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_ElementCapabilitiesReferenceNames(
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
        LMI_ElementCapabilities_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_ElementCapabilities,
    LMI_ElementCapabilities,
    _cb,
    LMI_ElementCapabilitiesInitialize(ctx))

CMAssociationMIStub( 
    LMI_ElementCapabilities,
    LMI_ElementCapabilities,
    _cb,
    LMI_ElementCapabilitiesInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ElementCapabilities",
    "LMI_ElementCapabilities",
    "instance association")
