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
#include "LMI_HostedSoftwareCollection.h"
#include "sw-utils.h"

static const CMPIBroker* _cb;

static void LMI_HostedSoftwareCollectionInitialize(const CMPIContext *ctx)
{
    software_init(LMI_HostedSoftwareCollection_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_HostedSoftwareCollectionCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_HostedSoftwareCollection_ClassName);
}

static CMPIStatus LMI_HostedSoftwareCollectionEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_HostedSoftwareCollectionEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    char instance_id[BUFLEN] = "";

    create_instance_id(LMI_SystemSoftwareCollection_ClassName, NULL, instance_id,
                    BUFLEN);

    LMI_SystemSoftwareCollectionRef ssc;
    LMI_SystemSoftwareCollectionRef_Init(&ssc, _cb, KNameSpace(cop));
    LMI_SystemSoftwareCollectionRef_Set_InstanceID(&ssc, instance_id);

    LMI_HostedSoftwareCollection w;
    LMI_HostedSoftwareCollection_Init(&w, _cb, KNameSpace(cop));
    LMI_HostedSoftwareCollection_SetObjectPath_Antecedent(&w,
            lmi_get_computer_system_safe(cc));
    LMI_HostedSoftwareCollection_Set_Dependent(&w, &ssc);
    KReturnInstance(cr, w);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedSoftwareCollectionGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_HostedSoftwareCollectionCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSoftwareCollectionModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSoftwareCollectionDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSoftwareCollectionExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedSoftwareCollectionAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedSoftwareCollectionAssociators(
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
        LMI_HostedSoftwareCollection_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_HostedSoftwareCollectionAssociatorNames(
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
        LMI_HostedSoftwareCollection_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_HostedSoftwareCollectionReferences(
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
        LMI_HostedSoftwareCollection_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_HostedSoftwareCollectionReferenceNames(
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
        LMI_HostedSoftwareCollection_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_HostedSoftwareCollection,
    LMI_HostedSoftwareCollection,
    _cb,
    LMI_HostedSoftwareCollectionInitialize(ctx))

CMAssociationMIStub(
    LMI_HostedSoftwareCollection,
    LMI_HostedSoftwareCollection,
    _cb,
    LMI_HostedSoftwareCollectionInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_HostedSoftwareCollection",
    "LMI_HostedSoftwareCollection",
    "instance association")
