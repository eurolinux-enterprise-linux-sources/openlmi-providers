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

#include "LMI_AssociatedPowerManagementService.h"
#include "LMI_PowerManagementService.h"
#include "CIM_ComputerSystem.h"

#include "power.h"

#include "globals.h"

static const CMPIBroker* _cb;

static void LMI_AssociatedPowerManagementServiceInitialize(CMPIInstanceMI *mi,
        const CMPIContext *ctx)
{
    mi->hdl = power_ref(_cb, ctx);
}

static void LMI_AssociatedPowerManagementServiceAssociationInitialize(
        CMPIAssociationMI *mi, const CMPIContext *ctx)
{
    mi->hdl = power_ref(_cb, ctx);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    power_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    const char *ns = KNameSpace(cop);

    LMI_AssociatedPowerManagementService w;
    LMI_AssociatedPowerManagementService_Init(&w, _cb, ns);

    LMI_AssociatedPowerManagementService_SetObjectPath_UserOfService(&w, lmi_get_computer_system());

    LMI_PowerManagementServiceRef powerManagementServiceRef;
    LMI_PowerManagementServiceRef_Init(&powerManagementServiceRef, _cb, ns);
    LMI_PowerManagementServiceRef_Set_Name(&powerManagementServiceRef, get_system_name());
    LMI_PowerManagementServiceRef_Set_SystemName(&powerManagementServiceRef, get_system_name());
    LMI_PowerManagementServiceRef_Set_CreationClassName(&powerManagementServiceRef, "LMI_PowerManagementService");
    LMI_PowerManagementServiceRef_Set_SystemCreationClassName(&powerManagementServiceRef, get_system_creation_class_name());
    LMI_AssociatedPowerManagementService_Set_ServiceProvided(&w, &powerManagementServiceRef);

    int count;
    unsigned short *list = power_available_requested_power_states(mi->hdl, &count);
    if (list == NULL) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    LMI_AssociatedPowerManagementService_Init_AvailableRequestedPowerStates(&w, count);
    for (int i = 0; i < count; i++) {
        LMI_AssociatedPowerManagementService_Set_AvailableRequestedPowerStates(&w, i, list[i]);
    }
    free(list);

    LMI_AssociatedPowerManagementService_Set_TransitioningToPowerState(&w, power_transitioning_to_power_state(mi->hdl));
    LMI_AssociatedPowerManagementService_Set_PowerState(&w, 2);
    LMI_AssociatedPowerManagementService_Set_RequestedPowerState(&w, power_requested_power_state(mi->hdl));

    KReturnInstance(cr, w);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceGetInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceCreateInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceModifyInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceDeleteInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    power_unref(mi->hdl);
    mi->hdl = NULL;

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceAssociators(
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
    if (!assocClass) {
        assocClass = "LMI_AssociatedPowerManagementService";
    }

    return KDefaultAssociators(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_AssociatedPowerManagementService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    if (!assocClass) {
        assocClass = "LMI_AssociatedPowerManagementService";
    }

    return KDefaultAssociatorNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_AssociatedPowerManagementService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceReferences(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    if (!assocClass) {
        assocClass = "LMI_AssociatedPowerManagementService";
    }

    return KDefaultReferences(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_AssociatedPowerManagementService_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_AssociatedPowerManagementServiceReferenceNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role)
{
    if (!assocClass) {
        assocClass = "LMI_AssociatedPowerManagementService";
    }

    return KDefaultReferenceNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_AssociatedPowerManagementService_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_AssociatedPowerManagementService,
    LMI_AssociatedPowerManagementService,
    _cb,
    LMI_AssociatedPowerManagementServiceInitialize(&mi, ctx))

CMAssociationMIStub( 
    LMI_AssociatedPowerManagementService,
    LMI_AssociatedPowerManagementService,
    _cb,
    LMI_AssociatedPowerManagementServiceAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AssociatedPowerManagementService",
    "LMI_AssociatedPowerManagementService",
    "instance association")
