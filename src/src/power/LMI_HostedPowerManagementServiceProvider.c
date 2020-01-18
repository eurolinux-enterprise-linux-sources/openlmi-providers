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
 * Authors: Radek Novacek <rnovacek@redhat.com>
 */

#include <cmpimacs.h>
#include <konkret/konkret.h>
#include "LMI_HostedPowerManagementService.h"
#include "power.h"

static const CMPIBroker* _cb;

static void LMI_HostedPowerManagementServiceInitialize(CMPIInstanceMI *mi,
        const CMPIContext *ctx)
{
    mi->hdl = power_ref(_cb, ctx);
}

static void LMI_HostedPowerManagementServiceAssociationInitialize(
        CMPIAssociationMI *mi, const CMPIContext *ctx)
{
    mi->hdl = power_ref(_cb, ctx);
}


static CMPIStatus LMI_HostedPowerManagementServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    power_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedPowerManagementServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_HostedPowerManagementServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    const char *ns = KNameSpace(cop);

    LMI_HostedPowerManagementService w;
    LMI_HostedPowerManagementService_Init(&w, _cb, ns);

    LMI_HostedPowerManagementService_SetObjectPath_Antecedent(&w, lmi_get_computer_system_safe(cc));

    LMI_PowerManagementServiceRef powerManagementServiceRef;
    LMI_PowerManagementServiceRef_Init(&powerManagementServiceRef, _cb, ns);
    LMI_PowerManagementServiceRef_Set_Name(&powerManagementServiceRef, lmi_get_system_name_safe(cc));
    LMI_PowerManagementServiceRef_Set_SystemName(&powerManagementServiceRef, lmi_get_system_name_safe(cc));
    LMI_PowerManagementServiceRef_Set_CreationClassName(&powerManagementServiceRef, "LMI_PowerManagementService");
    LMI_PowerManagementServiceRef_Set_SystemCreationClassName(&powerManagementServiceRef, lmi_get_system_creation_class_name());
    LMI_HostedPowerManagementService_Set_Dependent(&w, &powerManagementServiceRef);

    KReturnInstance(cr, w);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedPowerManagementServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_HostedPowerManagementServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedPowerManagementServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedPowerManagementServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedPowerManagementServiceExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedPowerManagementServiceAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    power_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedPowerManagementServiceAssociators(
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
        LMI_HostedPowerManagementService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_HostedPowerManagementServiceAssociatorNames(
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
        LMI_HostedPowerManagementService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_HostedPowerManagementServiceReferences(
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
        LMI_HostedPowerManagementService_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_HostedPowerManagementServiceReferenceNames(
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
        LMI_HostedPowerManagementService_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_HostedPowerManagementService,
    LMI_HostedPowerManagementService,
    _cb,
    LMI_HostedPowerManagementServiceInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_HostedPowerManagementService,
    LMI_HostedPowerManagementService,
    _cb,
    LMI_HostedPowerManagementServiceAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_HostedPowerManagementService",
    "LMI_HostedPowerManagementService",
    "instance association")
