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
#include "LMI_PCIBridge.h"
#include "LMI_Hardware.h"
#include "PCIDev.h"
#include "globals.h"

CMPIUint16 get_bridge_type(const char *bridge_type);

static const CMPIBroker* _cb = NULL;

struct pci_access *acc_bridge = NULL;

static void LMI_PCIBridgeInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);

    if (init_pci_access(&acc_bridge, PCI_FILL_IDENT
                                    | PCI_FILL_IRQ
                                    | PCI_FILL_BASES
                                    | PCI_FILL_ROM_BASE
                                    | PCI_FILL_CLASS
                                    | PCI_FILL_CAPS) != 0) {
        error("Failed to access the PCI bus.");
        abort();
    }
}

static CMPIStatus LMI_PCIBridgeCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    cleanup_pci_access(&acc_bridge);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PCIBridgeEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_PCIBridgeEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_PCIBridge lmi_dev;
    const char *ns = KNameSpace(cop);
    int i;
    CMPICount count;
    CMPIUint16 pci_cap;
    u8 rev, cache_line, subordinate_bus, secondary_bus, primary_bus;
    u16 svid, subid, status, command_reg, sec_status;
    u32 io_base, io_limit, io_type, mem_base, mem_limit, mem_type, pref_base,
        pref_limit, pref_type, pref_base_upper = 0, pref_limit_upper = 0;
    struct pci_dev *dev;
    struct pci_cap *cap;
    char vendor_buf[NAME_BUF_SIZE], *vendor_name;
    char device_buf[NAME_BUF_SIZE], *device_name;
    char subsys_buf[NAME_BUF_SIZE], *subsys_name;
    char svendor_buf[NAME_BUF_SIZE], *svendor_name;
    char class_buf[NAME_BUF_SIZE], *class;
    char device_id_str[PCI_DEVID_STR_SIZE];
    char instance_id[INSTANCE_ID_LEN];

    for (dev = acc_bridge->devices; dev; dev = dev->next) {
        /* Use only PCI Bridges */
        /* Throw away the lower 8 bits denoting the subclass */
        if (((dev->device_class) >> 8) != LMI_PCIBridge_ClassCode_Bridge) {
            continue;
        }

        vendor_name = pci_lookup_name(acc_bridge, vendor_buf, NAME_BUF_SIZE,
                PCI_LOOKUP_VENDOR, dev->vendor_id);
        device_name = pci_lookup_name(acc_bridge, device_buf, NAME_BUF_SIZE,
                PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
        get_subid(dev, &svid, &subid);
        subsys_name = pci_lookup_name(acc_bridge, subsys_buf, NAME_BUF_SIZE,
                PCI_LOOKUP_DEVICE | PCI_LOOKUP_SUBSYSTEM,
                dev->vendor_id, dev->device_id, svid, subid);
        svendor_name = pci_lookup_name(acc_bridge, svendor_buf, NAME_BUF_SIZE,
                PCI_LOOKUP_VENDOR | PCI_LOOKUP_SUBSYSTEM, svid);
        class = pci_lookup_name(acc_bridge, class_buf, NAME_BUF_SIZE,
                PCI_LOOKUP_CLASS, dev->device_class);
        status = pci_read_word(dev, PCI_STATUS);
        rev = pci_read_byte(dev, PCI_REVISION_ID);
        cache_line = pci_read_byte(dev, PCI_CACHE_LINE_SIZE);
        command_reg = pci_read_word(dev, PCI_COMMAND);
        subordinate_bus = pci_read_byte(dev, PCI_SUBORDINATE_BUS);
        secondary_bus = pci_read_byte(dev, PCI_SECONDARY_BUS);
        primary_bus = pci_read_byte(dev, PCI_PRIMARY_BUS);
        io_base = pci_read_byte(dev, PCI_IO_BASE);
        io_limit = pci_read_byte(dev, PCI_IO_LIMIT);
        io_type = io_base & PCI_IO_RANGE_TYPE_MASK;
        mem_base = pci_read_word(dev, PCI_MEMORY_BASE);
        mem_limit = pci_read_word(dev, PCI_MEMORY_LIMIT);
        mem_type = mem_base & PCI_MEMORY_RANGE_TYPE_MASK;
        pref_base = pci_read_word(dev, PCI_PREF_MEMORY_BASE);
        pref_limit = pci_read_word(dev, PCI_PREF_MEMORY_LIMIT);
        pref_type = pref_base & PCI_PREF_RANGE_TYPE_MASK;
        sec_status = pci_read_word(dev, PCI_SEC_STATUS);

        snprintf(device_id_str, PCI_DEVID_STR_SIZE, "%02x:%02x.%u",
                dev->bus, dev->dev, dev->func);
        snprintf(instance_id, INSTANCE_ID_LEN,
                ORGID ":" ORGID "_" PCI_BRIDGE_CLASS_NAME ":%s", device_id_str);

        LMI_PCIBridge_Init(&lmi_dev, _cb, ns);

        LMI_PCIBridge_Set_SystemCreationClassName(&lmi_dev,
                get_system_creation_class_name());
        LMI_PCIBridge_Set_SystemName(&lmi_dev, get_system_name());
        LMI_PCIBridge_Set_CreationClassName(&lmi_dev,
                ORGID "_" PCI_BRIDGE_CLASS_NAME);
        LMI_PCIBridge_Set_Caption(&lmi_dev,
                "This object represents one PCI bridge contained in system.");

        LMI_PCIBridge_Set_PrimaryStatus(&lmi_dev,
                LMI_PCIBridge_PrimaryStatus_Unknown);
        LMI_PCIBridge_Set_HealthState(&lmi_dev,
                LMI_PCIBridge_HealthState_Unknown);

        LMI_PCIBridge_Set_DeviceID(&lmi_dev, device_id_str);
        LMI_PCIBridge_Set_BusNumber(&lmi_dev, dev->bus);
        LMI_PCIBridge_Set_DeviceNumber(&lmi_dev, dev->dev);
        LMI_PCIBridge_Set_FunctionNumber(&lmi_dev, dev->func);
        LMI_PCIBridge_Set_Name(&lmi_dev, device_name);
        LMI_PCIBridge_Set_ElementName(&lmi_dev, device_name);

        LMI_PCIBridge_Set_VendorID(&lmi_dev, dev->vendor_id);
        LMI_PCIBridge_Set_PCIDeviceID(&lmi_dev, dev->device_id);
        LMI_PCIBridge_Set_PCIDeviceName(&lmi_dev, device_name);
        if (vendor_name) {
            LMI_PCIBridge_Set_VendorName(&lmi_dev, vendor_name);
        }
        if (svid > 0 && svid < 65535) {
            LMI_PCIBridge_Set_SubsystemVendorID(&lmi_dev, svid);
            if (svendor_name) {
                LMI_PCIBridge_Set_SubsystemVendorName(&lmi_dev, svendor_name);
            }
        }
        if (subid > 0 && subid < 65535) {
            LMI_PCIBridge_Set_SubsystemID(&lmi_dev, subid);
            if (subsys_name) {
                LMI_PCIBridge_Set_SubsystemName(&lmi_dev, subsys_name);
            }
        }
        if (class) {
            LMI_PCIBridge_Set_BridgeType(&lmi_dev, get_bridge_type(class));
        } else {
            LMI_PCIBridge_Set_BridgeType(&lmi_dev,
                    LMI_PCIBridge_BridgeType_Other);
        }

        if (rev) {
            LMI_PCIBridge_Set_RevisionID(&lmi_dev, rev);
        }
        if (cache_line) {
            LMI_PCIBridge_Set_CacheLineSize(&lmi_dev, cache_line);
        }
        if (command_reg) {
            LMI_PCIBridge_Set_CommandRegister(&lmi_dev, command_reg);
        }
        if (subordinate_bus) {
            LMI_PCIBridge_Set_SubordinateBusNumber(&lmi_dev, subordinate_bus);
        }
        if (secondary_bus) {
            LMI_PCIBridge_Set_SecondayBusNumber(&lmi_dev, secondary_bus);
        }
        LMI_PCIBridge_Set_PrimaryBusNumber(&lmi_dev, primary_bus);

        /* Throw away the lower 8 bits denoting the subclass */
        LMI_PCIBridge_Set_ClassCode(&lmi_dev, ((dev->device_class) >> 8));

        if (status) {
            if ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_SLOW) {
                LMI_PCIBridge_Set_DeviceSelectTiming(&lmi_dev,
                        LMI_PCIBridge_DeviceSelectTiming_Slow);
            } else if ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_MEDIUM) {
                LMI_PCIBridge_Set_DeviceSelectTiming(&lmi_dev,
                        LMI_PCIBridge_DeviceSelectTiming_Medium);
            } else if ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_FAST) {
                LMI_PCIBridge_Set_DeviceSelectTiming(&lmi_dev,
                        LMI_PCIBridge_DeviceSelectTiming_Fast);
            }
        }
        if (sec_status) {
            LMI_PCIBridge_Set_SecondaryStatusRegister(&lmi_dev, sec_status);
            if ((sec_status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_SLOW) {
                LMI_PCIBridge_Set_SecondaryBusDeviceSelectTiming(&lmi_dev,
                        LMI_PCIBridge_SecondaryBusDeviceSelectTiming_Slow);
            } else if ((sec_status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_MEDIUM) {
                LMI_PCIBridge_Set_SecondaryBusDeviceSelectTiming(&lmi_dev,
                        LMI_PCIBridge_SecondaryBusDeviceSelectTiming_Medium);
            } else if ((sec_status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_FAST) {
                LMI_PCIBridge_Set_SecondaryBusDeviceSelectTiming(&lmi_dev,
                        LMI_PCIBridge_SecondaryBusDeviceSelectTiming_Fast);
            }
        }

        if (dev->rom_base_addr) {
            LMI_PCIBridge_Set_ExpansionROMBaseAddress(&lmi_dev,
                    dev->rom_base_addr);
        }

        LMI_PCIBridge_Set_InstanceID(&lmi_dev, instance_id);
        LMI_PCIBridge_Set_InterruptPin(&lmi_dev,
                pci_read_byte(dev, PCI_INTERRUPT_PIN));
        LMI_PCIBridge_Set_LatencyTimer(&lmi_dev,
                pci_read_byte(dev, PCI_LATENCY_TIMER));
        LMI_PCIBridge_Set_SecondaryLatencyTimer(&lmi_dev,
                pci_read_byte(dev, PCI_SEC_LATENCY_TIMER));

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
            LMI_PCIBridge_Init_BaseAddress64(&lmi_dev, count);
#else
            LMI_PCIBridge_Init_BaseAddress(&lmi_dev, count);
#endif
            count = 0;
            for (i = 0; i < 6; i++) {
                if (dev->base_addr[i]) {
#ifdef PCI_HAVE_64BIT_ADDRESS
                    LMI_PCIBridge_Set_BaseAddress64(&lmi_dev, count++,
                            dev->base_addr[i]);
#else
                    LMI_PCIBridge_Set_BaseAddress(&lmi_dev, count++,
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
            LMI_PCIBridge_Init_Capabilities(&lmi_dev, count);
            count = 0;
            if (status & PCI_STATUS_66MHZ) {
                LMI_PCIBridge_Set_Capabilities(&lmi_dev, count++,
                        LMI_PCIBridge_Capabilities_Supports_66MHz);
            }
            if (status & PCI_STATUS_UDF) {
                LMI_PCIBridge_Set_Capabilities(&lmi_dev, count++,
                        LMI_PCIBridge_Capabilities_Supports_User_Definable_Features);
            }
            if (status & PCI_STATUS_FAST_BACK) {
                LMI_PCIBridge_Set_Capabilities(&lmi_dev, count++,
                        LMI_PCIBridge_Capabilities_Supports_Fast_Back_to_Back_Transactions);
            }
            for (cap = dev->first_cap; cap; cap = cap->next) {
                pci_cap = get_capability(cap->id);
                if (pci_cap > 1) {
                    LMI_PCIBridge_Set_Capabilities(&lmi_dev, count++, pci_cap);
                }
            }
        }

        /* IO base and limit addresses */
        if (io_type == (io_limit & PCI_IO_RANGE_TYPE_MASK)
                && (io_type == PCI_IO_RANGE_TYPE_16 || io_type == PCI_IO_RANGE_TYPE_32)) {
            io_base = (io_base & PCI_IO_RANGE_MASK) << 8;
            io_limit = (io_limit & PCI_IO_RANGE_MASK) << 8;
            if (io_type == PCI_IO_RANGE_TYPE_32) {
                io_base |= (pci_read_word(dev, PCI_IO_BASE_UPPER16) << 16);
                io_limit |= (pci_read_word(dev, PCI_IO_LIMIT_UPPER16) << 16);
                if (io_base <= io_limit && io_base > 0 && io_limit > 0) {
                    LMI_PCIBridge_Set_IOBaseUpper16(&lmi_dev, io_base >> 16);
                    LMI_PCIBridge_Set_IOLimitUpper16(&lmi_dev, io_limit >> 16);
                }
            } else if (io_base <= io_limit && io_base > 0 && io_limit > 0) {
                LMI_PCIBridge_Set_IOBase(&lmi_dev, io_base >> 12);
                LMI_PCIBridge_Set_IOLimit(&lmi_dev, io_limit >> 12);
            }
        }

        /* Memory base and limit addresses behind the bridge */
        if (mem_type == (mem_limit & PCI_MEMORY_RANGE_TYPE_MASK) && !mem_type) {
            mem_base = (mem_base & PCI_MEMORY_RANGE_MASK) << 16;
            mem_limit = (mem_limit & PCI_MEMORY_RANGE_MASK) << 16;
            if (mem_base <= mem_limit && mem_base > 0 && mem_limit > 0) {
                LMI_PCIBridge_Set_MemoryBase(&lmi_dev, mem_base >> 20);
                LMI_PCIBridge_Set_MemoryLimit(&lmi_dev, mem_limit >> 20);
            }
        }

        /* Prefetchable memory base and limit addresses behind the bridge */
        if (pref_type == (pref_limit & PCI_PREF_RANGE_TYPE_MASK)
                && (pref_type == PCI_PREF_RANGE_TYPE_32 || pref_type == PCI_PREF_RANGE_TYPE_64)) {
            if (pref_type == PCI_PREF_RANGE_TYPE_64) {
                pref_base_upper = pci_read_long(dev, PCI_PREF_BASE_UPPER32);
                pref_limit_upper = pci_read_long(dev, PCI_PREF_LIMIT_UPPER32);
                if (pref_base_upper <= pref_limit_upper
                        && pref_base_upper > 0 && pref_limit_upper > 0) {
                    LMI_PCIBridge_Set_PrefetchBaseUpper32(&lmi_dev, pref_base_upper);
                    LMI_PCIBridge_Set_PrefetchLimitUpper32(&lmi_dev, pref_limit_upper);
                }
            }
            if ((pref_type == PCI_PREF_RANGE_TYPE_64 && pref_base_upper == 0 && pref_limit_upper == 0)
                    || pref_type == PCI_PREF_RANGE_TYPE_32) {
                pref_base = (pref_base & PCI_PREF_RANGE_MASK) << 16;
                pref_limit = (pref_limit & PCI_PREF_RANGE_MASK) << 16;
                if (pref_base <= pref_limit && pref_base > 0 && pref_limit > 0) {
                    LMI_PCIBridge_Set_PrefetchMemoryBase(&lmi_dev, pref_base >> 20);
                    LMI_PCIBridge_Set_PrefetchMemoryLimit(&lmi_dev, pref_limit >> 20);
                }
            }
        }

        KReturnInstance(cr, lmi_dev);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PCIBridgeGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_PCIBridgeCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PCIBridgeModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PCIBridgeDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_PCIBridgeExecQuery(
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
    LMI_PCIBridge,
    LMI_PCIBridge,
    _cb,
    LMI_PCIBridgeInitialize(ctx))

static CMPIStatus LMI_PCIBridgeMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_PCIBridgeInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_PCIBridge_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_PCIBridge,
    LMI_PCIBridge,
    _cb,
    LMI_PCIBridgeInitialize(ctx))

KUint32 LMI_PCIBridge_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIBridgeRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIBridge_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIBridgeRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIBridge_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIBridgeRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIBridge_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIBridgeRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIBridge_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIBridgeRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIBridge_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIBridgeRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIBridge_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIBridgeRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_PCIBridge_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIBridgeRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint8 LMI_PCIBridge_BISTExecution(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_PCIBridgeRef* self,
    CMPIStatus* status)
{
    KUint8 result = KUINT8_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

CMPIUint16 get_bridge_type(const char *bridge_type)
{
    static struct {
        CMPIUint16 cim_val;     /* CIM value */
        char *bridge_type;      /* bridge type */
    } values[] = {
        {0,   "Host bridge"},
        {1,   "ISA bridge"},
        {2,   "EISA bridge"},
        {3,   "MicroChannel bridge"},
        {4,   "PCI bridge"},
        {5,   "PCMCIA bridge"},
        {6,   "NuBus bridge"},
        {7,   "CardBus bridge"},
        {8,   "RACEway bridge"},
        /*
        {9,   "AGP"},
        {10,  "PCIe"},
        {11,  "PCIe-to-PCI"},
        */
        {128, "Other"},
    };

    size_t i, val_length = sizeof(values) / sizeof(values[0]);

    for (i = 0; i < val_length; i++) {
        if (strcmp(bridge_type, values[i].bridge_type) == 0) {
            return values[i].cim_val;
        }
    }

    return 128; /* Other */
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_PCIBridge",
    "LMI_PCIBridge",
    "instance method")
