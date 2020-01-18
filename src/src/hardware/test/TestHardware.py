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

from TestHardwareBase import *
from lmi.shell import LMIInstance
import os

class TestHardwareProcessor(HardwareProcessorTestCase):
    """
    Class for testing LMI_Processor class
    """

    CLASS_NAME = "LMI_Processor"

    def test_processor_flags_x86(self):
        """
        Hardware: Test for processor flags.
        This test is valid only on x86 architectures.
        """

        lmi_cpu_instances = self.cim_class.instances()
        self.assertTrue(lmi_cpu_instances)

        cpus_info = self.get_cpus_info_proc()
        self.assertTrue(cpus_info)
        cpus_dmi_info = self.get_cpus_info_dmidecode()

        # Compare only count of physical cpus.
        self.assertEqual(len(lmi_cpu_instances), len(cpus_dmi_info.keys()))

        cpu_arch = self.get_my_arch()

        # Get only one set of cpu flags.
        cpu_flags = cpus_info[0]["flags"].split()
        # Translate to numeric values.
        cpu_flags = sorted(map(lambda f: self.proc_flag_table[f], cpu_flags))

        # All architectures of cpus should be same.
        # Flags too. But we are good testers right?
        # Maybe some memory corruption or failing broker...
        # Who knows. We are little paranoid here.
        for lmi_cpu in lmi_cpu_instances:

            # Same architecture on all cpus?
            self.assertEqual(cpu_arch, lmi_cpu.Architecture)

            lmi_cpu_flags = lmi_cpu.Flags
            self.assertTrue (lmi_cpu_flags)

            # Same cpu flags on both sides?
            self.assertEqual(sorted(lmi_cpu_flags), sorted(cpu_flags))


class TestHardwareMemory(HardwareMemoryTestCase):
    """
    Class for testing LMI_Memory class
    """

    CLASS_NAME = "LMI_Memory"

    def test_huge_page_sizes(self):
        """ Get supported sizes of hugepages """
        lmi_mem_instances = self.cim_class.instances()
        self.assertTrue(lmi_mem_instances)

        huge_pages_sizes = self.get_huge_pages_sizes()
        self.assertEqual(len(lmi_mem_instances), len(huge_pages_sizes))

        for lmi_mem in lmi_mem_instances:
            self.assertEqual(lmi_mem.SupportedHugeMemoryPageSizes, huge_pages_sizes)

    def test_memory_page_size(self):
        lmi_mem_instance = self.cim_class.first_instance()
        self.assertTrue(lmi_mem_instance)

        page_size = os.sysconf("SC_PAGE_SIZE") / 1024
        lmi_page_size = lmi_mem_instance.StandardMemoryPageSize

        self.assertEqual (page_size, lmi_page_size)


class TestHardwarePhysicalMemory(HardwarePhysicalMemoryTestCase):
    """
    Class for testing LMI_PhysicalMemory class
    """

    CLASS_NAME = "LMI_PhysicalMemory"

    def setUp(self):
        self.lmi_phymem_instances = self.cim_class.instances()
        self.assertTrue(self.lmi_phymem_instances)

    def test_physical_memory(self):
        phymems = self.get_physical_memory()
        lmi_phymems = map(lambda x: (
            x.TotalWidth,
            x.DataWidth,
            x.Capacity,
            x.SerialNumber,
            #x.BankLabel,
            x.FormFactor,
            x.MemoryType,
            x.Manufacturer,
            x.PartNumber,
            x.ConfiguredMemoryClockSpeed,
            x.Speed
        ), self.lmi_phymem_instances)
        self.assertEqual(sorted(lmi_phymems), sorted(phymems))


class TestHardwareBaseboard(HardwareBaseboardTestCase):
    """
    Class for testing LMI_Baseboard class
    """

    CLASS_NAME = "LMI_Baseboard"

    def setUp(self):
        self.lmi_baseboard_instances = self.cim_class.instances()
        # On virtual machines there can be blank
        #self.assertTrue(self.lmi_baseboard_instances)

    def test_baseboard_basic_info(self):
        baseboards = self.get_baseboards()

        lmi_baseboards = map(lambda x: (
            x.SerialNumber,
            x.Manufacturer,
            x.Model,
            x.Version
        ), self.lmi_baseboard_instances)
        lmi_serials = map(lambda x: x.SerialNumber, self.lmi_baseboard_instances)

        self.assertEqual(sorted(lmi_baseboards), sorted(baseboards))


class TestHardwarePCIDevice(HardwarePCITestCase):
    """
    Class for testing LMI_PCIDevice class
    """

    CLASS_NAME = "LMI_PCIDevice"

    def setUp(self):
        self.lmi_pci_devices_instances = self.cim_class.instances()
        self.assertTrue(self.lmi_pci_devices_instances)

    def test_pci_devices(self):
        lmi_pci_devices = self.get_pci_devices_lmi(self.lmi_pci_devices_instances)
        pci_devices = self.get_pci_devices(self.IS_DEVICE)

        self.assertEqual(sorted(lmi_pci_devices), sorted(pci_devices))


class TestHardwarePCIBridge(HardwarePCITestCase):
    """
    Class for testing LMI_PCIBridge class
    """

    CLASS_NAME = "LMI_PCIBridge"

    def setUp(self):
        self.lmi_pci_bridges_instances = self.cim_class.instances()
        self.assertTrue(self.lmi_pci_bridges_instances)

    def test_pci_bridges(self):
        lmi_pci_bridges = self.get_pci_devices_lmi(self.lmi_pci_bridges_instances)
        pci_bridges = self.get_pci_devices(self.IS_BRIDGE)

        self.assertEqual(sorted(lmi_pci_bridges), sorted(pci_bridges))
