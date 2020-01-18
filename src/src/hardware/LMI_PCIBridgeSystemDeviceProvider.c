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
#include "LMI_PCIBridgeSystemDevice.h"
#include "utils.h"
#include "PCIDev.h"

static const CMPIBroker* _cb;

struct pci_access *acc_system_bridge = NULL;

static void LMI_PCIBridgeSystemDeviceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);

    if (init_pci_access(&acc_system_bridge, PCI_FILL_CLASS) != 0) {
        lmi_error("Failed to access the PCI bus.");
        abort();
    }
}

static CMPIStatus LMI_PCIBridgeSystemDeviceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    cleanup_pci_access(&acc_system_bridge);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_PCIBridgeSystemDevice lmi_pci_sys_device;
    LMI_PCIBridgeRef lmi_dev;
    const char *ns = KNameSpace(cop);
    struct pci_dev *dev;
    char device_id_str[BUFLEN];

    for (dev = acc_system_bridge->devices; dev; dev = dev->next) {
        /* Use only PCI Bridges */
        /* Throw away the lower 8 bits denoting the subclass */
        if (((dev->device_class) >> 8) != LMI_PCIBridge_ClassCode_Bridge) {
            continue;
        }

        LMI_PCIBridgeSystemDevice_Init(&lmi_pci_sys_device, _cb, ns);

        snprintf(device_id_str, BUFLEN, "%02x:%02x.%u",
                dev->bus, dev->dev, dev->func);

        LMI_PCIBridgeRef_Init(&lmi_dev, _cb, ns);
        LMI_PCIBridgeRef_Set_SystemCreationClassName(&lmi_dev,
                lmi_get_system_creation_class_name());
        LMI_PCIBridgeRef_Set_SystemName(&lmi_dev, lmi_get_system_name_safe(cc));
        LMI_PCIBridgeRef_Set_CreationClassName(&lmi_dev,
                LMI_PCIBridge_ClassName);
        LMI_PCIBridgeRef_Set_DeviceID(&lmi_dev, device_id_str);

        LMI_PCIBridgeSystemDevice_SetObjectPath_GroupComponent(
                &lmi_pci_sys_device, lmi_get_computer_system_safe(cc));
        LMI_PCIBridgeSystemDevice_Set_PartComponent(&lmi_pci_sys_device,
                &lmi_dev);

        KReturnInstance(cr, lmi_pci_sys_device);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceAssociators(
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
        LMI_PCIBridgeSystemDevice_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceAssociatorNames(
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
        LMI_PCIBridgeSystemDevice_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceReferences(
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
        LMI_PCIBridgeSystemDevice_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_PCIBridgeSystemDeviceReferenceNames(
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
        LMI_PCIBridgeSystemDevice_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_PCIBridgeSystemDevice,
    LMI_PCIBridgeSystemDevice,
    _cb,
    LMI_PCIBridgeSystemDeviceInitialize(ctx))

CMAssociationMIStub(
    LMI_PCIBridgeSystemDevice,
    LMI_PCIBridgeSystemDevice,
    _cb,
    LMI_PCIBridgeSystemDeviceInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PCIBridgeSystemDevice",
    "LMI_PCIBridgeSystemDevice",
    "instance association")
