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
#include "LMI_SystemSoftwareCollection.h"
#include "sw-utils.h"

static const CMPIBroker* _cb = NULL;

static void LMI_SystemSoftwareCollectionInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SystemSoftwareCollection_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_SystemSoftwareCollectionCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_SystemSoftwareCollection_ClassName);
}

static CMPIStatus LMI_SystemSoftwareCollectionEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SystemSoftwareCollectionEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    char instance_id[BUFLEN] = "";

    create_instance_id(LMI_SystemSoftwareCollection_ClassName, NULL, instance_id,
                    BUFLEN);

    LMI_SystemSoftwareCollection w;
    LMI_SystemSoftwareCollection_Init(&w, _cb, KNameSpace(cop));
    LMI_SystemSoftwareCollection_Set_InstanceID(&w, instance_id);
    LMI_SystemSoftwareCollection_Set_Caption(&w,
            "System RPM Package Collection");
    KReturnInstance(cr, w);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SystemSoftwareCollectionGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SystemSoftwareCollectionCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SystemSoftwareCollectionModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SystemSoftwareCollectionDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SystemSoftwareCollectionExecQuery(
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
    LMI_SystemSoftwareCollection,
    LMI_SystemSoftwareCollection,
    _cb,
    LMI_SystemSoftwareCollectionInitialize(ctx))

static CMPIStatus LMI_SystemSoftwareCollectionMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SystemSoftwareCollectionInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SystemSoftwareCollection_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SystemSoftwareCollection,
    LMI_SystemSoftwareCollection,
    _cb,
    LMI_SystemSoftwareCollectionInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SystemSoftwareCollection",
    "LMI_SystemSoftwareCollection",
    "instance method")
