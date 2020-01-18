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
#include "LMI_PortPhysicalConnector.h"
#include "utils.h"
#include "dmidecode.h"

CMPIUint16 get_connectorlayout(const char *dmi_val);

static const CMPIBroker* _cb = NULL;

static void LMI_PortPhysicalConnectorInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_PortPhysicalConnectorCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PortPhysicalConnectorEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PortPhysicalConnectorEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_PortPhysicalConnector lmi_port;
    const char *ns = KNameSpace(cop);
    CMPIUint16 conn_layout;
    char instance_id[BUFLEN];
    unsigned i;
    DmiPort *dmi_ports = NULL;
    unsigned dmi_ports_nb = 0;

    if (dmi_get_ports(&dmi_ports, &dmi_ports_nb) != 0 || dmi_ports_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_ports_nb; i++) {
        LMI_PortPhysicalConnector_Init(&lmi_port, _cb, ns);

        LMI_PortPhysicalConnector_Set_CreationClassName(&lmi_port,
                LMI_PortPhysicalConnector_ClassName);
        LMI_PortPhysicalConnector_Set_Caption(&lmi_port, "Physical Port");
        LMI_PortPhysicalConnector_Set_Description(&lmi_port,
                "This object represents one physical port on the chassis.");

        snprintf(instance_id, BUFLEN,
                LMI_ORGID ":" LMI_PortPhysicalConnector_ClassName ":%s",
                dmi_ports[i].name);
        conn_layout = get_connectorlayout(dmi_ports[i].type);

        LMI_PortPhysicalConnector_Set_Tag(&lmi_port, dmi_ports[i].name);
        LMI_PortPhysicalConnector_Set_ConnectorLayout(&lmi_port, conn_layout);
        LMI_PortPhysicalConnector_Set_ElementName(&lmi_port, dmi_ports[i].name);
        LMI_PortPhysicalConnector_Set_Name(&lmi_port, dmi_ports[i].name);
        LMI_PortPhysicalConnector_Set_InstanceID(&lmi_port, instance_id);

        if (conn_layout == LMI_PortPhysicalConnector_ConnectorLayout_Other) {
            if (strcmp(dmi_ports[i].type, "Other") != 0) {
                LMI_PortPhysicalConnector_Set_ConnectorDescription(&lmi_port,
                        dmi_ports[i].type);
            } else if (strcmp(dmi_ports[i].port_type, "Other") != 0) {
                LMI_PortPhysicalConnector_Set_ConnectorDescription(&lmi_port,
                        dmi_ports[i].port_type);
            } else {
                LMI_PortPhysicalConnector_Set_ConnectorDescription(&lmi_port,
                        dmi_ports[i].name);
            }
        }
        if (strstr(dmi_ports[i].type, "male") &&
                !strstr(dmi_ports[i].type, "female")) {
            LMI_PortPhysicalConnector_Set_ConnectorGender(&lmi_port,
                    LMI_PortPhysicalConnector_ConnectorGender_Male);
        } else {
            LMI_PortPhysicalConnector_Set_ConnectorGender(&lmi_port,
                    LMI_PortPhysicalConnector_ConnectorGender_Female);
        }

        KReturnInstance(cr, lmi_port);
    }

done:
    dmi_free_ports(&dmi_ports, &dmi_ports_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PortPhysicalConnectorGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PortPhysicalConnectorCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PortPhysicalConnectorModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PortPhysicalConnectorDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PortPhysicalConnectorExecQuery(
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
    LMI_PortPhysicalConnector,
    LMI_PortPhysicalConnector,
    _cb,
    LMI_PortPhysicalConnectorInitialize(ctx))

static CMPIStatus LMI_PortPhysicalConnectorMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PortPhysicalConnectorInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_PortPhysicalConnector_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

/*
 * Get connector layout according to the dmidecode.
 * @param dmi_val from dmidecode
 * @return CIM id of connector layout
 */
CMPIUint16 get_connectorlayout(const char *dmi_val)
{
    if (!dmi_val || !strlen(dmi_val)) {
        return 0; /* Unknown */
    }

    static struct {
        CMPIUint16 cim_val;     /* CIM value */
        char *dmi_val;          /* dmidecode value */
    } values[] = {
        {0,  "Unknown"},
        {1,  "Other"},
        /*
        {2,  "RS232"},
        */
        {3,  "BNC"},
        {4,  "RJ-11"},
        {5,  "RJ-45"},
        {6,  "DB-9 male"},
        {6,  "DB-9 female"},
        /*
        {7,  "Slot"},
        {8,  "SCSI High Density"},
        {9,  "SCSI Low Density"},
        {10, "Ribbon"},
        {11, "AUI"},
        {12, "Fiber SC"},
        {13, "Fiber ST"},
        {14, "FDDI-MIC"},
        {15, "Fiber-RTMJ"},
        {16, "PCI"},
        {17, "PCI-X"},
        {18, "PCI-E"},
        {19, "PCI-E x1"},
        {20, "PCI-E x2"},
        {21, "PCI-E x4"},
        {22, "PCI-E x8"},
        {23, "PCI-E x16"},
        {24, "PCI-E x32"},
        {25, "PCI-E x64"},
        */
    };

    size_t i, val_length = sizeof(values) / sizeof(values[0]);

    for (i = 0; i < val_length; i++) {
        if (strcmp(dmi_val, values[i].dmi_val) == 0) {
            return values[i].cim_val;
        }
    }

    return 1; /* Other */
}

CMMethodMIStub(
    LMI_PortPhysicalConnector,
    LMI_PortPhysicalConnector,
    _cb,
    LMI_PortPhysicalConnectorInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PortPhysicalConnector",
    "LMI_PortPhysicalConnector",
    "instance method")
