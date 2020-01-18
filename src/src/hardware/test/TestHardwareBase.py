# -*- encoding: utf-8 -*-
# Copyright (C) 2014 Red Hat, Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Authors: Robin Hack <rhack@redhat.com>
#
"""
Common utilities and base class for all hardware tests.
"""

from collections import OrderedDict
from lmi.test.lmibase import LmiTestCase
import os
import re
import subprocess
from hwdata import PCI
from pprint import pprint

def parse_whole_dmi_line(line):
    key = line.strip().split(':')[0].strip()
    value = line.strip().split(':')[1].strip()
    return (key, value)

def parse_flag_lines(flags):
    # And god said: We must divide them by the mighty bracket char!
    return map(lambda flag: flag.split("(")[0].lower(), flags)

def parse_dmi_output(dmioutput):

    r = re.compile('^Handle 0x')

    devices = {}
    device = {}

    # Feed the hungry machine with states.
    handle = False
    skip_line = False
    key = None
    multiline_values = []

    # Houston. Launch in 4... 3... 2... 1...
    for line in dmioutput.split('\n'):
        # Are we in handle context?
        if handle:
            # skip line after Handle 0x
            if skip_line:
                skip_line = False
                continue

            # We found blank line.
            if not line.strip():
                # Maybe multiline values ends with new line.
                # We must handle it
                if len(multiline_values) > 0:
                    device[key] = multiline_values

                devices[handle] = device

                # Carefully reset all states.
                device = {}
                # Handle context ends here.
                handle = False
                skip_line = False
                key = None
                # reset here if ve are in multiline mode
                # and input is damaged
                multiline_values = []
                continue

            if len(multiline_values) > 0 and line.find(":") != -1:
                device[key] = multiline_values
                multiline_values = []

            # We have line like:
            # Current Speed: 2900 MHz
            # I call that whole dmi line.
            if len(line.split(":")) == 2:
                (key, value) = parse_whole_dmi_line(line)
                device[key] = value
                continue
            # We have line like:
            # Flags:
            # or line like:
            # FXSR (FXSAVE and FXSTOR instructions supported)
            if len(line.split(":")) == 1:
                if line.find(":") == -1:
                    multiline_values.append(line.strip())
                else:
                    # We need to wait for another lines.
                    key = line.strip().split(":")[0]
                continue
        else:
            # match for Handle 0x
            # this starts handle context
            if r.match(line):
                handle = line.split()[1][:-1]
                # skip one line of dmidecode output
                skip_line = True
                continue
    return devices

def get_dmi_output(type_p):
    p = subprocess.Popen(["dmidecode", "-t", str(type_p)], stdout=subprocess.PIPE)
    return p.communicate()[0]

class HardwareTestCase(LmiTestCase):
    """
    Base class for all LMI Hardware test classes.
    """
    USE_EXCEPTIONS = True

    @classmethod
    def setUpClass(cls):
        LmiTestCase.setUpClass.im_func(cls)


class HardwareProcessorTestCase(HardwareTestCase):
    """
    Base class for all LMI ProcessorHardware test classes.
    """
    # this is stolen from:
    # linux/arch/x86/include/asm/cpufeature.h
    proc_flag_table = {
        'fpu' : 0,
        'vme' : 1,
        'de' : 2,
        'pse' : 3,
        'tsc' : 4,
        'msr' : 5,
        'pae' : 6,
        'mce' : 7,
        'cx8' : 8,
        'apic' : 9,
        'sep' : 11,
        'mtrr' : 12,
        'pge' : 13,
        'mca' : 14,
        'cmov' : 15,
        'pat' : 16,
        'pse36' : 17,
        'pn' : 18,
        'clflush' : 19,
        'dts' : 21,
        'acpi' : 22,
        'mmx' : 23,
        'fxsr' : 24,
        'sse' : 25,
        'sse2' : 26,
        'ss' : 27,
        'ht' : 28,
        'tm' : 29,
        'ia64' : 30,
        'pbe' : 31,
        'syscall' : 43,
        'mp' : 51,
        'nx' : 52,
        'mmxext' : 54,
        'fxsr_opt' : 57,
        'pdpe1gb' : 58,
        'rdtscp' : 59,
        'lm' : 61,
        '3dnowext' : 62,
        '3dnow' : 63,
        'recovery' : 64,
        'longrun' : 65,
        'lrti' : 67,
        'cxmmx' : 96,
        'k6_mtrr' : 97,
        'cyrix_arr' : 98,
        'centaur_mcr' : 99,
        'k8' : 100,
        'k7' : 101,
        'p3' : 102,
        'p4' : 103,
        'constant_tsc' : 104,
        'up' : 105,
        'fxsave_leak' : 106,
        'arch_perfmon' : 107,
        'pebs' : 108,
        'bts' : 109,
        'syscall32' : 110,
        'sysenter32' : 111,
        'rep_good' : 112,
        'mfence_rdtsc' : 113,
        'lfence_rdtsc' : 114,
        '11ap' : 115,
        'nopl' : 116,
        'xtopology' : 118,
        'tsc_reliable' : 119,
        'nonstop_tsc' : 120,
        'clflush_monitor' : 121,
        'extd_apicid' : 122,
        'amd_dcm' : 123,
        'aperfmperf' : 124,
        'eagerfpu' : 125,
        'pni' : 128,
        'pclmulqdq' : 129,
        'dtes64' : 130,
        'monitor' : 131,
        'ds_cpl' : 132,
        'vmx' : 133,
        'smx' : 134,
        'est' : 135,
        'tm2' : 136,
        'ssse3' : 137,
        'cid' : 138,
        'fma' : 140,
        'cx16' : 141,
        'xtpr' : 142,
        'pdcm' : 143,
        'pcid' : 145,
        'dca' : 146,
        'sse4_1' : 147,
        'sse4_2' : 148,
        'x2apic' : 149,
        'movbe' : 150,
        'popcnt' : 151,
        'tsc_deadline_timer' : 152,
        'aes' : 153,
        'xsave' : 154,
        'osxsave' : 155,
        'avx' : 156,
        'f16c' : 157,
        'rdrand' : 158,
        'hypervisor' : 159,
        'rng' : 162,
        'rng_en' : 163,
        'ace' : 166,
        'ace_en' : 167,
        'ace2' : 168,
        'ace2_en' : 169,
        'phe' : 170,
        'phe_en' : 171,
        'pmm' : 172,
        'pmm_en' : 173,
        'lahf_lm' : 192,
        'cmp_legacy' : 193,
        'svm' : 194,
        'extapic' : 195,
        'cr8_legacy' : 196,
        'abm' : 197,
        'sse4a' : 198,
        'misalignsse' : 199,
        '3dnowprefetch' : 200,
        'osvw' : 201,
        'ibs' : 202,
        'xop' : 203,
        'skinit' : 204,
        'wdt' : 205,
        'lwp' : 207,
        'fma4' : 208,
        'tce' : 209,
        'nodeid_msr' : 211,
        'tbm' : 213,
        'topoext' : 214,
        'perfctr_core' : 215,
        'ida' : 224,
        'arat' : 225,
        'cpb' : 226,
        'epb' : 227,
        'xsaveopt' : 228,
        'pln' : 229,
        'pts' : 230,
        'dtherm' : 231,
        'hw_pstate' : 232,
        'tpr_shadow' : 256,
        'vnmi' : 257,
        'flexpriority' : 258,
        'ept' : 259,
        'vpid' : 260,
        'npt' : 261,
        'lbrv' : 262,
        'svm_lock' : 263,
        'nrip_save' : 264,
        'tsc_scale' : 265,
        'vmcb_clean' : 266,
        'flushbyasid' : 267,
        'decodeassists' : 268,
        'pausefilter' : 269,
        'pfthreshold' : 270,
        'fsgsbase' : 288,
        'tsc_adjust' : 289,
        'bmi1' : 291,
        'hle' : 292,
        'avx2' : 293,
        'smep' : 295,
        'bmi2' : 296,
        'erms' : 297,
        'invpcid' : 298,
        'rtm' : 299,
        'rdseed' : 306,
        'adx' : 307,
        'smap' : 308
    }

    def get_cpus_info_dmidecode(self):
        dmidecode = parse_dmi_output(get_dmi_output(4))

        if len(dmidecode.keys()) == 0:
            return None

        # There are no cpu flags in dmidecoude output in virtual machine
        # enviroment.
            for handle in dmidecode.keys():
                try:
                    dmidecode[handle]["Flags"] = parse_flag_lines(dmidecode[handle]["Flags"])
                except KeyError:
                    dmidecode[handle]["Flags"] = []

        return dmidecode

    # Get info only for physical processors
    def get_cpus_info_proc(self):
        """
        Return the information in /proc/cpuinfo
        as a dictionary in the following format:
        cpu_info[0]={...}
        cpu_info[1]={...}
        """
        cpu_info = OrderedDict()
        proc_info = OrderedDict()
        nprocs = 0
        physical_id = -1
        with open('/proc/cpuinfo') as f:
            for line in f:
                if not line.strip():
                    # end of one processor
                    cpu_info[nprocs] = proc_info
                    nprocs = nprocs+1
                    # Reset
                    proc_info = OrderedDict()
                else:
                    if len(line.split(':')) == 2:
                        proc_info[line.split(':')[0].strip()] = line.split(':')[1].strip()
                    else:
                        proc_info[line.split(':')[0].strip()] = ''
        return cpu_info

    def get_my_arch(self):
        return os.uname()[4]


class HardwareMemoryTestCase(HardwareTestCase):

    def get_huge_pages_sizes(self):
        # regexp is perfect because:
        # ./mm/hugetlb.c: snprintf(h->name, HSTATE_NAME_LEN, "hugepages-%lukB", huge_page_size(h)/1024);

        r = re.compile('hugepages-(\d+)kB')
        dirs = os.listdir("/sys/kernel/mm/hugepages/")
        huge_pages_sizes = []
        for dir in dirs:
            huge_pages_sizes.append(int(r.findall(dir)[0]))
        return huge_pages_sizes


class HardwarePhysicalMemoryTestCase(HardwareTestCase):

    form_factor_table = {
        'Unknown': 0,
        'Other': 1,
        'SIP': 2,
        'DIP': 3,
        'ZIP': 4,
        'SOJ': 5,
        'Proprietary': 6,
        'SIMM': 7,
        'DIMM': 8,
        'TSOP': 9,
        'PGA': 10,
        'RIMM': 11,
        'SODIMM': 12,
        'SRIMM': 13,
        'SMD': 14,
        'SSMP': 15,
        'QFP': 16,
        'TQFP': 17,
        'SOIC': 18,
        'LCC': 19,
        'PLCC': 20,
        'BGA': 21,
        'FPBGA': 22,
        'LGA': 23
    }

    memory_type_table = {
        'Unknown': 0,
        'Other': 1,
        'DRAM': 2,
        'Synchronous DRAM': 3,
        'Cache DRAM': 4,
        'EDO': 5,
        'EDRAM': 6,
        'VRAM': 7,
        'SRAM': 8,
        'RAM': 9,
        'ROM': 10,
        'Flash': 11,
        'EEPROM': 12,
        'FEPROM': 13,
        'EPROM': 14,
        'CDRAM': 15,
        '3DRAM': 16,
        'SDRAM': 17,
        'SGRAM': 18,
        'RDRAM': 19,
        'DDR': 20,
        'DDR-2': 21,
        'BRAM': 22,
        'FB-DIMM': 23,
        'DDR3': 24,
        'FBD2': 25
    }

    def get_physical_memory(self):
        dmidecode_mem_17 = parse_dmi_output(get_dmi_output(17))

        tuples = []

        for handle in dmidecode_mem_17.keys():
            module = dmidecode_mem_17[handle]
            if module["Size"] == "No Module Installed":
                continue

            def convert(x):
                if x == -1:
                    return None
                else:
                    return x

            tuples.append((
                int(module.get("Total Width", "0").split(" ")[0]),              # 0
                int(module.get("Data Width", "0").split(" ")[0]),               # 1
                int(module.get("Size", 0).split(" ")[0]) * 1024 * 1024,         # 2
                module.get("Serial Number", "0"),                               # 3
                # skipped for now
                #module.get("Bank Locator", ""),
                #module.get("Locator", ""),
                self.form_factor_table[module.get("Form Factor", "Unknown")],   # 4
                self.memory_type_table[module.get("Type", "Unknown")],          # 5
                module.get("Manufacturer", ""),                                 # 6
                module.get("Part Number", ""),                                  # 7
                convert(int(module.get("Speed", "-1 Mhz").split(" ")[0]))       # 8
            ))

        # I just belive that order is achieved...
        # I need concate two dmidecode outputs to one.
        empty_output = True
        dmidecode_mem_6 = parse_dmi_output(get_dmi_output(6))
        c = 0
        for handle in dmidecode_mem_6:
            module = dmidecode_mem_6[handle]

            tuples[c] = tuples[c] + (convert(int(module.get("Current Speed", "-1 ns").split(" ")[0])))
            empty_output = False
            c += 1
        # on virtual machines is this "normal" state.
        if empty_output:
            for c in xrange(0, len(tuples)):
                tuples[c] += (None,)

        return tuples

class HardwareBaseboardTestCase(HardwareTestCase):

    def get_baseboards(self):
        dmidecode_boards = parse_dmi_output(get_dmi_output(2))

        tuples = []

        for handle in dmidecode_boards.keys():
            baseboard = dmidecode_boards[handle]

            tuples.append((
                baseboard.get("Serial Number", "0"),
                baseboard.get("Manufacturer", ""),
                baseboard.get("Product Name", ""),
                baseboard.get("Version", "")
            ))

        return tuples


class HardwarePCITestCase(HardwareTestCase):

    def get_pci_device_attr(self, device, attr_p):
        f = open ("/sys/bus/pci/devices/%s/%s" % (device, attr_p), "r")
        attr = f.readline ()
        f.close()
        return attr

    IS_BRIDGE = 1 << 0
    IS_DEVICE = 1 << 1
    IS_BOTH = (IS_BRIDGE | IS_DEVICE)

    def get_pci_devices(self, mask):

        pci_devices = []

        for device in os.listdir("/sys/bus/pci/devices/"):

            device_class = self.get_pci_device_attr(device, "class")
            vendor_id = self.get_pci_device_attr(device, "vendor")
            device_id = self.get_pci_device_attr(device, "device")
            subsystem_device_id = self.get_pci_device_attr(device, "subsystem_device")
            subsystem_vendor_id = self.get_pci_device_attr(device, "subsystem_vendor")

            # Is device bridge? Class == 0x06?
            is_bridge = ((int(device_class, 16) >> 16) == 0x06)
            device = device.replace("0000:","")

            add_device = False
            if (mask & self.IS_BRIDGE) and is_bridge:
                add_device = True

            if (mask & self.IS_DEVICE) and not is_bridge:
                add_device = True

            if add_device:
                add_device = False
                pci_devices.append((
                    unicode(device),
                    int(device_id, 16),
                    int(device_class, 16) >> 16,
                    int(vendor_id, 16),
                    unicode(self.lookup_pci_vendor(vendor_id)),
                    unicode(self.lookup_pci_name(vendor_id, device_id)),
                    int(subsystem_device_id, 16),
                    int(subsystem_vendor_id, 16),
                    self.lookup_pci_vendor(subsystem_vendor_id)
                ))
                continue

        return pci_devices

    def get_pci_devices_lmi(self, lmi_pci_devices_instances):
        lmi_pci_devices = map(lambda x: (
            x.DeviceID,
            x.PCIDeviceID,
            x.ClassCode,
            x.VendorID,
            x.VendorName,
            x.PCIDeviceName,
            x.SubsystemID,
            x.SubsystemVendorID,
            x.SubsystemVendorName
        ), lmi_pci_devices_instances)
        return lmi_pci_devices


    def lookup_pci_vendor(self, vendor_id):
        pci = PCI()
        # Convert to string without conversion to decimal.
        # Achieve format of 4 hex numbers.
        vendor_id = "%.4x" % int(vendor_id, 16)
        pci_vendor = pci.get_vendor(vendor_id)
        if pci_vendor is not None:
            pci_vendor = unicode(pci_vendor)
        return pci_vendor


    def lookup_pci_name(self, vendor_id, device_id):
        pci = PCI()
        vendor_id = "%.4x" % int(vendor_id, 16)
        device_id = "%.4x" % int(device_id, 16)
        return pci.get_device(vendor_id, device_id)
