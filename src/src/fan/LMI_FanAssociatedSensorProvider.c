/*
 * Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
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
 *          Radek Novacek <rnovacek@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_FanAssociatedSensor.h"
#include "fan.h"

static const CMPIBroker* _cb;

static void LMI_FanAssociatedSensorInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
    init_linux_fan_module();
}

static CMPIStatus LMI_FanAssociatedSensorCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FanAssociatedSensorEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_FanAssociatedSensorEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    const char *ns = KNameSpace(cop);
    CMPIStatus status;

    struct cim_fan *sptr = NULL;
    struct fanlist *lptr = NULL, *fans = NULL;
    if (enum_all_fans(&fans) != 0 ) {
        KReturn2(_cb, ERR_FAILED, "Could not list get fan list.");
    }

    lptr = fans;
    // iterate fan list
    while (lptr) {
        sptr = lptr->f;
        LMI_FanAssociatedSensor w;
        LMI_FanAssociatedSensor_Init(&w, _cb, ns);

        LMI_FanRef fan;
        LMI_FanRef_Init(&fan, _cb, ns);
        LMI_FanRef_Set_CreationClassName(&fan, "LMI_Fan");
        LMI_FanRef_Set_DeviceID(&fan, sptr->device_id);
        LMI_FanRef_Set_SystemCreationClassName(&fan, lmi_get_system_creation_class_name());
        LMI_FanRef_Set_SystemName(&fan, lmi_get_system_name_safe(cc));

        LMI_FanSensorRef fanSensor;
        LMI_FanSensorRef_Init(&fanSensor, _cb, ns);
        LMI_FanSensorRef_Set_CreationClassName(&fanSensor, "LMI_FanSensor");
        LMI_FanSensorRef_Set_DeviceID(&fanSensor, sptr->device_id);
        LMI_FanSensorRef_Set_SystemCreationClassName(&fanSensor, lmi_get_system_creation_class_name());
        LMI_FanSensorRef_Set_SystemName(&fanSensor, lmi_get_system_name_safe(cc));

        LMI_FanAssociatedSensor_Set_Antecedent(&w, &fanSensor);
        LMI_FanAssociatedSensor_Set_Dependent(&w, &fan);

        status = __KReturnInstance((cr), &(w).__base);
        if (!KOkay(status)) {
            free_fanlist(fans);
            return status;
        }

        lptr = lptr->next;
    }
    free_fanlist(fans);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FanAssociatedSensorGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_FanAssociatedSensorCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FanAssociatedSensorModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FanAssociatedSensorDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FanAssociatedSensorExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FanAssociatedSensorAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FanAssociatedSensorAssociators(
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
        LMI_FanAssociatedSensor_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_FanAssociatedSensorAssociatorNames(
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
        LMI_FanAssociatedSensor_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_FanAssociatedSensorReferences(
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
        LMI_FanAssociatedSensor_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_FanAssociatedSensorReferenceNames(
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
        LMI_FanAssociatedSensor_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_FanAssociatedSensor,
    LMI_FanAssociatedSensor,
    _cb,
    LMI_FanAssociatedSensorInitialize(ctx))

CMAssociationMIStub(
    LMI_FanAssociatedSensor,
    LMI_FanAssociatedSensor,
    _cb,
    LMI_FanAssociatedSensorInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_FanAssociatedSensor",
    "LMI_FanAssociatedSensor",
    "instance association")
