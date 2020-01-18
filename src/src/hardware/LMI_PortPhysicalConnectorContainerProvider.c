/*
 * Copyright (C) 2013 Red Hat, Inc. All rights reserved.
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
#include "LMI_PortPhysicalConnectorContainer.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb;

static void LMI_PortPhysicalConnectorContainerInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_PortPhysicalConnectorContainer lmi_port_container;
    LMI_PortPhysicalConnectorRef lmi_port;
    LMI_ChassisRef lmi_chassis;
    const char *ns = KNameSpace(cop);
    unsigned i;
    DmiChassis dmi_chassis;
    DmiPort *dmi_ports = NULL;
    unsigned dmi_ports_nb = 0;

    if (dmi_get_chassis(&dmi_chassis) != 0) {
        goto done;
    }
    if (dmi_get_ports(&dmi_ports, &dmi_ports_nb) != 0 || dmi_ports_nb < 1) {
        goto done;
    }

    LMI_ChassisRef_Init(&lmi_chassis, _cb, ns);
    LMI_ChassisRef_Set_CreationClassName(&lmi_chassis,
            ORGID "_" CHASSIS_CLASS_NAME);
    LMI_ChassisRef_Set_Tag(&lmi_chassis, dmi_get_chassis_tag(&dmi_chassis));

    for (i = 0; i < dmi_ports_nb; i++) {
        LMI_PortPhysicalConnectorContainer_Init(&lmi_port_container, _cb, ns);

        LMI_PortPhysicalConnectorRef_Init(&lmi_port, _cb, ns);
        LMI_PortPhysicalConnectorRef_Set_CreationClassName(&lmi_port,
                ORGID "_" PORT_PHYS_CONN_CLASS_NAME);
        LMI_PortPhysicalConnectorRef_Set_Tag(&lmi_port, dmi_ports[i].name);

        LMI_PortPhysicalConnectorContainer_Set_GroupComponent(
                &lmi_port_container, &lmi_chassis);
        LMI_PortPhysicalConnectorContainer_Set_PartComponent(
                &lmi_port_container, &lmi_port);

        KReturnInstance(cr, lmi_port_container);
    }

done:
    dmi_free_chassis(&dmi_chassis);
    dmi_free_ports(&dmi_ports, &dmi_ports_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerAssociators(
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
        LMI_PortPhysicalConnectorContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerAssociatorNames(
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
        LMI_PortPhysicalConnectorContainer_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerReferences(
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
        LMI_PortPhysicalConnectorContainer_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_PortPhysicalConnectorContainerReferenceNames(
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
        LMI_PortPhysicalConnectorContainer_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_PortPhysicalConnectorContainer,
    LMI_PortPhysicalConnectorContainer,
    _cb,
    LMI_PortPhysicalConnectorContainerInitialize(ctx))

CMAssociationMIStub( 
    LMI_PortPhysicalConnectorContainer,
    LMI_PortPhysicalConnectorContainer,
    _cb,
    LMI_PortPhysicalConnectorContainerInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PortPhysicalConnectorContainer",
    "LMI_PortPhysicalConnectorContainer",
    "instance association")
