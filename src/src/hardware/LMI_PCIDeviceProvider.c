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
 * Authors: Tomas Smetana <tsmetana@redhat.com>
 *          Peter Schiffer <pschiffe@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_PCIDevice.h"
#include "LMI_Hardware.h"
#include "PCIDev.h"
#include "globals.h"

static const CMPIBroker* _cb = NULL;

struct pci_access *acc_dev = NULL;

static void LMI_PCIDeviceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);

    if (init_pci_access(&acc_dev, PCI_FILL_IDENT
                                | PCI_FILL_IRQ
                                | PCI_FILL_BASES
                                | PCI_FILL_ROM_BASE
                                | PCI_FILL_CLASS
                                | PCI_FILL_CAPS) != 0) {
        error("Failed to access the PCI bus.");
        abort();
    }
}

static CMPIStatus LMI_PCIDeviceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    cleanup_pci_access(&acc_dev);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PCIDeviceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(_cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PCIDeviceEnumInstances(
        CMPIInstanceMI* mi,
        const CMPIContext* cc,
        const CMPIResult* cr,
        const CMPIObjectPath* cop,
        const char** properties)
{
    LMI_PCIDevice lmi_dev;
    const char *ns = KNameSpace(cop);
    int i;
    CMPICount count;
    CMPIUint16 pci_cap;
    u8 rev, cache_line;
    u16 svid, subid, status, command_reg;
    struct pci_dev *dev;
    struct pci_cap *cap;
    char vendor_buf[NAME_BUF_SIZE], *vendor_name;
    char device_buf[NAME_BUF_SIZE], *device_name;
    char subsys_buf[NAME_BUF_SIZE], *subsys_name;
    char svendor_buf[NAME_BUF_SIZE], *svendor_name;
    char device_id_str[PCI_DEVID_STR_SIZE];
    char instance_id[INSTANCE_ID_LEN];

    for (dev = acc_dev->devices; dev; dev = dev->next) {
        /* Ignore PCI Bridges */
        /* Throw away the lower 8 bits denoting the subclass */
        if (((dev->device_class) >> 8) == LMI_PCIDevice_ClassCode_Bridge) {
            continue;
        }

        vendor_name = pci_lookup_name(acc_dev, vendor_buf, NAME_BUF_SIZE,
                PCI_LOOKUP_VENDOR, dev->vendor_id);
        device_name = pci_lookup_name(acc_dev, device_buf, NAME_BUF_SIZE,
                PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
        get_subid(dev, &svid, &subid);
        subsys_name = pci_lookup_name(acc_dev, subsys_buf, NAME_BUF_SIZE,
                PCI_LOOKUP_DEVICE | PCI_LOOKUP_SUBSYSTEM,
                dev->vendor_id, dev->device_id, svid, subid);
        svendor_name = pci_lookup_name(acc_dev, svendor_buf, NAME_BUF_SIZE,
                PCI_LOOKUP_VENDOR | PCI_LOOKUP_SUBSYSTEM, svid);
        status = pci_read_word(dev, PCI_STATUS);
        rev = pci_read_byte(dev, PCI_REVISION_ID);
        cache_line = pci_read_byte(dev, PCI_CACHE_LINE_SIZE);
        command_reg = pci_read_word(dev, PCI_COMMAND);

        snprintf(device_id_str, PCI_DEVID_STR_SIZE, "%02x:%02x.%u",
                dev->bus, dev->dev, dev->func);
        snprintf(instance_id, INSTANCE_ID_LEN,
                ORGID ":" ORGID "_" PCI_DEVICE_CLASS_NAME ":%s", device_id_str);

        LMI_PCIDevice_Init(&lmi_dev, _cb, ns);

        LMI_PCIDevice_Set_SystemCreationClassName(&lmi_dev,
                get_system_creation_class_name());
        LMI_PCIDevice_Set_SystemName(&lmi_dev, get_system_name());
        LMI_PCIDevice_Set_CreationClassName(&lmi_dev,
                ORGID "_" PCI_DEVICE_CLASS_NAME);
        LMI_PCIDevice_Set_Caption(&lmi_dev,
                "This object represents one logical PCI device contained in system.");

        LMI_PCIDevice_Set_PrimaryStatus(&lmi_dev,
                LMI_PCIDevice_PrimaryStatus_Unknown);
        LMI_PCIDevice_Set_HealthState(&lmi_dev,
                LMI_PCIDevice_HealthState_Unknown);

        LMI_PCIDevice_Set_DeviceID(&lmi_dev, device_id_str);
        LMI_PCIDevice_Set_BusNumber(&lmi_dev, dev->bus);
        LMI_PCIDevice_Set_DeviceNumber(&lmi_dev, dev->dev);
        LMI_PCIDevice_Set_FunctionNumber(&lmi_dev, dev->func);
        LMI_PCIDevice_Set_Name(&lmi_dev, device_name);
        LMI_PCIDevice_Set_ElementName(&lmi_dev, device_name);

        LMI_PCIDevice_Set_VendorID(&lmi_dev, dev->vendor_id);
        LMI_PCIDevice_Set_PCIDeviceID(&lmi_dev, dev->device_id);
        LMI_PCIDevice_Set_PCIDeviceName(&lmi_dev, device_name);
        if (vendor_name) {
            LMI_PCIDevice_Set_VendorName(&lmi_dev, vendor_name);
        }
        if (svid > 0 && svid < 65535) {
            LMI_PCIDevice_Set_SubsystemVendorID(&lmi_dev, svid);
            if (svendor_name) {
                LMI_PCIDevice_Set_SubsystemVendorName(&lmi_dev, svendor_name);
            }
        }
        if (subid > 0 && subid < 65535) {
            LMI_PCIDevice_Set_SubsystemID(&lmi_dev, subid);
            if (subsys_name) {
                LMI_PCIDevice_Set_SubsystemName(&lmi_dev, subsys_name);
            }
        }

        if (rev) {
            LMI_PCIDevice_Set_RevisionID(&lmi_dev, rev);
        }
        if (cache_line) {
            LMI_PCIDevice_Set_CacheLineSize(&lmi_dev, cache_line);
        }
        if (command_reg) {
            LMI_PCIDevice_Set_CommandRegister(&lmi_dev, command_reg);
        }

        /* Throw away the lower 8 bits denoting the subclass */
        LMI_PCIDevice_Set_ClassCode(&lmi_dev, ((dev->device_class) >> 8));

        if (status) {
            if ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_SLOW) {
                LMI_PCIDevice_Set_DeviceSelectTiming(&lmi_dev,
                        LMI_PCIDevice_DeviceSelectTiming_Slow);
            } else if ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_MEDIUM) {
                LMI_PCIDevice_Set_DeviceSelectTiming(&lmi_dev,
                        LMI_PCIDevice_DeviceSelectTiming_Medium);
            } else if ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_FAST) {
                LMI_PCIDevice_Set_DeviceSelectTiming(&lmi_dev,
                        LMI_PCIDevice_DeviceSelectTiming_Fast);
            }
        }

        if (dev->rom_base_addr) {
            LMI_PCIDevice_Set_ExpansionROMBaseAddress(&lmi_dev,
                    dev->rom_base_addr);
        }

        LMI_PCIDevice_Set_InstanceID(&lmi_dev, instance_id);
        LMI_PCIDevice_Set_InterruptPin(&lmi_dev,
                pci_read_byte(dev, PCI_INTERRUPT_PIN));
        LMI_PCIDevice_Set_LatencyTimer(&lmi_dev,
                pci_read_byte(dev, PCI_LATENCY_TIMER));

        /* PCI Base Addresses */
        /* Count them */
        count = 0;
        for (i = 0; i < 6; i++) {
            if (dev->base_addr[i]) {
                count++;
            }
        }
        /* Set PCI Base Addresses */
        if (count > 0) {
#ifdef PCI_HAVE_64BIT_ADDRESS
            LMI_PCIDevice_Init_BaseAddress64(&lmi_dev, count);
#else
            LMI_PCIDevice_Init_BaseAddress(&lmi_dev, count);
#endif
            count = 0;
            for (i = 0; i < 6; i++) {
                if (dev->base_addr[i]) {
#ifdef PCI_HAVE_64BIT_ADDRESS
                    LMI_PCIDevice_Set_BaseAddress64(&lmi_dev, count++,
                            dev->base_addr[i]);
#else
                    LMI_PCIDevice_Set_BaseAddress(&lmi_dev, count++,
                            dev->base_addr[i]);
#endif
                }
            }
        }

        /* PCI Capabilities */
        /* Count PCI Capabilities */
        count = 0;
        if (status & PCI_STATUS_66MHZ) {
            count++;
        }
        if (status & PCI_STATUS_UDF) {
            count++;
        }
        if (status & PCI_STATUS_FAST_BACK) {
            count++;
        }
        for (cap = dev->first_cap; cap; cap = cap->next) {
            pci_cap = get_capability(cap->id);
            if (pci_cap > 1) {
                count++;
            }
        }
        /* Get PCI Capabilities */
        if (count > 0) {
            LMI_PCIDevice_Init_Capabilities(&lmi_dev, count);
            count = 0;
            if (status & PCI_STATUS_66MHZ) {
                LMI_PCIDevice_Set_Capabilities(&lmi_dev, count++,
                        LMI_PCIDevice_Capabilities_Supports_66MHz);
            }
            if (status & PCI_STATUS_UDF) {
                LMI_PCIDevice_Set_Capabilities(&lmi_dev, count++,
                        LMI_PCIDevice_Capabilities_Supports_User_Definable_Features);
            }
            if (status & PCI_STATUS_FAST_BACK) {
                LMI_PCIDevice_Set_Capabilities(&lmi_dev, count++,
                        LMI_PCIDevice_Capabilities_Supports_Fast_Back_to_Back_Transactions);
            }
            for (cap = dev->first_cap; cap; cap = cap->next) {
                pci_cap = get_capability(cap->id);
                if (pci_cap > 1) {
                    LMI_PCIDevice_Set_Capabilities(&lmi_dev, count++, pci_cap);
                }
            }
        }

        KReturnInstance(cr, lmi_dev);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PCIDeviceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PCIDeviceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PCIDeviceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PCIDeviceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PCIDeviceExecQuery(
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
    LMI_PCIDevice,
    LMI_PCIDevice,
    _cb,
    LMI_PCIDeviceInitialize(ctx))

static CMPIStatus LMI_PCIDeviceMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PCIDeviceInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_PCIDevice_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_PCIDevice,
    LMI_PCIDevice,
    _cb,
    LMI_PCIDeviceInitialize(ctx))

KUint32 LMI_PCIDevice_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIDeviceRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIDevice_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIDeviceRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIDevice_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIDeviceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIDevice_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIDeviceRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIDevice_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIDeviceRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIDevice_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIDeviceRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIDevice_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIDeviceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIDevice_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIDeviceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint8 LMI_PCIDevice_BISTExecution(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIDeviceRef* self,
    CMPIStatus* status)
{
    KUint8 result = KUINT8_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PCIDevice",
    "LMI_PCIDevice",
    "instance method")
