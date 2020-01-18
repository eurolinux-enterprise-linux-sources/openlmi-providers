/*
 * Copyright (C) 2014 Red Hat, Inc.  All rights reserved.
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
 * Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
*/

#include <konkret/konkret.h>
#include "LMI_HostedSystemService.h"
#include "util/serviceutil.h"

static const CMPIBroker* _cb;

static void LMI_HostedSystemServiceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, NULL);
}

static CMPIStatus LMI_HostedSystemServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedSystemServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_HostedSystemServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    SList *slist = NULL;
    char output[BUFLEN];
    const char *ns = KNameSpace(cop);
    const char *hostname = lmi_get_system_name_safe(cc);

    slist = service_find_all(output, sizeof(output));
    if (slist == NULL) {
        KReturn2(_cb, ERR_FAILED, "%s", output);
    }

    for (int i = 0; i < slist->cnt; i++) {
        LMI_HostedSystemService w;
        LMI_HostedSystemService_Init(&w, _cb, ns);

        LMI_HostedSystemService_SetObjectPath_Antecedent(&w, lmi_get_computer_system_safe(cc));

        LMI_ServiceRef ref;
        LMI_ServiceRef_Init(&ref, _cb, ns);
        LMI_ServiceRef_Set_Name(&ref, slist->name[i]);
        LMI_ServiceRef_Set_SystemName(&ref, hostname);
        LMI_ServiceRef_Set_CreationClassName(&ref, LMI_Service_ClassName);
        LMI_ServiceRef_Set_SystemCreationClassName(&ref, lmi_get_system_creation_class_name());

        LMI_HostedSystemService_Set_Dependent(&w, &ref);

        KReturnInstance(cr, w);
    }
    service_free_slist(slist);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedSystemServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_HostedSystemServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSystemServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSystemServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSystemServiceExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSystemServiceAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedSystemServiceAssociators(
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
        LMI_HostedSystemService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_HostedSystemServiceAssociatorNames(
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
        LMI_HostedSystemService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_HostedSystemServiceReferences(
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
        LMI_HostedSystemService_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_HostedSystemServiceReferenceNames(
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
        LMI_HostedSystemService_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_HostedSystemService,
    LMI_HostedSystemService,
    _cb,
    LMI_HostedSystemServiceInitialize(ctx))

CMAssociationMIStub(
    LMI_HostedSystemService,
    LMI_HostedSystemService,
    _cb,
    LMI_HostedSystemServiceInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_HostedSystemService",
    "LMI_HostedSystemService",
    "instance association")
