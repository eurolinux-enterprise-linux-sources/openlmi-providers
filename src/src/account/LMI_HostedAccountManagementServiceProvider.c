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
 * Authors: Roman Rakus <rrakus@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_HostedAccountManagementService.h"
#include "CIM_ComputerSystem.h"
#include "LMI_AccountManagementService.h"

#include "macros.h"
#include "account_globals.h"

static const CMPIBroker* _cb;

static void LMI_HostedAccountManagementServiceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_HostedAccountManagementServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedAccountManagementServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_HostedAccountManagementServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_AccountManagementServiceRef lamsref;
    LMI_HostedAccountManagementService lhs;

    const char *nameSpace = KNameSpace(cop);
    const char *hostname = lmi_get_system_name_safe(cc);

    LMI_AccountManagementServiceRef_Init(&lamsref, _cb, nameSpace);
    LMI_AccountManagementServiceRef_Set_Name(&lamsref, LAMSNAME);
    LMI_AccountManagementServiceRef_Set_SystemCreationClassName(&lamsref,
      lmi_get_system_creation_class_name());
    LMI_AccountManagementServiceRef_Set_SystemName(&lamsref, hostname);
    LMI_AccountManagementServiceRef_Set_CreationClassName(&lamsref,
      LMI_AccountManagementService_ClassName);

    LMI_HostedAccountManagementService_Init(&lhs, _cb, nameSpace);
    LMI_HostedAccountManagementService_SetObjectPath_Antecedent(&lhs,
            lmi_get_computer_system_safe(cc));
    LMI_HostedAccountManagementService_Set_Dependent(&lhs, &lamsref);

    KReturnInstance(cr, lhs);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedAccountManagementServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_HostedAccountManagementServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedAccountManagementServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedAccountManagementServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedAccountManagementServiceExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedAccountManagementServiceAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedAccountManagementServiceAssociators(
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
        LMI_HostedAccountManagementService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_HostedAccountManagementServiceAssociatorNames(
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
        LMI_HostedAccountManagementService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_HostedAccountManagementServiceReferences(
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
        LMI_HostedAccountManagementService_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_HostedAccountManagementServiceReferenceNames(
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
        LMI_HostedAccountManagementService_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_HostedAccountManagementService,
    LMI_HostedAccountManagementService,
    _cb,
    LMI_HostedAccountManagementServiceInitialize(ctx))

CMAssociationMIStub(
    LMI_HostedAccountManagementService,
    LMI_HostedAccountManagementService,
    _cb,
    LMI_HostedAccountManagementServiceInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_HostedAccountManagementService",
    "LMI_HostedAccountManagementService",
    "instance association")
