#!/usr/bin/env python
#
# Copyright (C) 2012-2013 Red Hat, Inc.  All rights reserved.
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
# Authors: Michal Minar <miminar@redhat.com>
#
"""
Unit tests for LMI_InstalledSoftwareIdentity provider.
"""

import pywbem
import unittest

from lmi.test.lmibase import enable_lmi_exceptions
from lmi.test.util import mark_dangerous
from lmi.test.util import mark_tedious
import base
import rpmcache

class TestInstalledSoftwareIdentity(base.SoftwareBaseTestCase):
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_InstalledSoftwareIdentity"
    KEYS = ("InstalledSoftware", "System")

    def make_op(self, pkg, newer=True):
        """
        @return object path of InstalledSoftwareIdentity association
        """
        return self.cim_class.new_instance_name({
            "System" : self.system_iname,
            "InstalledSoftware" : self.ns.LMI_SoftwareIdentity. \
                new_instance_name({
                    "InstanceID" : 'LMI:LMI_SoftwareIdentity:'
                            + pkg.get_nevra(newer=newer, with_epoch="ALWAYS")
                })
            })

    def test_get_instance(self):
        """
        Tests GetInstance call on packages from our rpm cache.
        """
        for pkg in self.safe_pkgs:
            objpath = self.make_op(pkg)
            inst = objpath.to_instance()
            self.assertCIMNameEqual(inst.path, objpath,
                    "Object paths should match for package %s"%pkg)
            for key in self.KEYS:
                self.assertIn(key, inst.properties(),
                        "OP is missing \"%s\" key for package %s"%(key, pkg))
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key))
            self.assertCIMNameEqual(objpath, inst.path,
                    "Object paths should match for package %s"%pkg)

            # try it now with CIM_ComputerSystem
            system = objpath.System.copy()
            objpath.System.wrapped_object.classname = "CIM_ComputerSystem"
            objpath.System.wrapped_object.CreationClassName = \
                    "CIM_ComputerSystem"

            inst = objpath.to_instance()
            self.assertCIMNameEqual(objpath, inst.path,
                    "Object paths should match for package %s"%pkg)
            for key in self.KEYS:
                self.assertIn(key, inst.properties(),
                        "OP is missing \"%s\" key for package %s"%(key, pkg))
            self.assertCIMNameEqual(system, inst.System)
            self.assertCIMNameEqual(objpath, inst.path)

    def test_enum_instance_names_safe(self):
        """
        Tests EnumInstanceNames call on installed packages.
        """
        inames = self.cim_class.instance_names()
        self.assertGreater(len(inames), 0)
        objpath = self.make_op(self.safe_pkgs[0])
        for iname in inames:
            self.assertEqual(iname.namespace, 'root/cimv2')
            self.assertEqual(iname.classname, self.CLASS_NAME)
            self.assertEqual(sorted(iname.key_properties()), sorted(self.KEYS))
            self.assertCIMNameEqual(objpath.System, iname.System)
        nevra_set = set(i.InstalledSoftware.InstanceID for i in inames)
        for pkg in self.safe_pkgs:
            nevra = 'LMI:LMI_SoftwareIdentity:' \
                    + pkg.get_nevra(with_epoch="ALWAYS")
            self.assertTrue(nevra in nevra_set, 'Missing nevra "%s".' % nevra)

    @mark_tedious
    def test_enum_instances(self):
        """
        Tests EnumInstances call on installed packages.
        """
        insts = self.cim_class.instances()
        self.assertGreater(len(insts), 0)
        for inst in insts:
            self.assertEqual(set(inst.path.key_properties()), set(self.KEYS))
            for key in self.KEYS:
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key))
        nevra_set = set(i.InstalledSoftware.InstanceID for i in insts)
        for pkg in self.safe_pkgs:
            nevra = 'LMI:LMI_SoftwareIdentity:' \
                    + pkg.get_nevra(with_epoch="ALWAYS")
            self.assertTrue(nevra in nevra_set, "Missing pkg %s in nevra_set."
                    % nevra)

    @unittest.skip("dangerous tests are not reliable")
    @mark_tedious
    @mark_dangerous
    def test_enum_instance_names(self):
        """
        Tests EnumInstanceNames call on dangerous packages.
        """
        pkg = self.dangerous_pkgs[0]

        rpmcache.ensure_pkg_installed(pkg)

        inames1 = self.cim_class.instance_names()
        self.assertGreater(len(inames1), 1)
        self.assertIn('LMI:LMI_SoftwareIdentity:'
                    + pkg.get_nevra(with_epoch="ALWAYS"),
                set(i.InstalledSoftware.InstanceID for i in inames1))

        rpmcache.remove_pkg(pkg.name)
        inames2 = self.cim_class.instance_names()
        self.assertEqual(len(inames1), len(inames2) + 1)
        self.assertNotIn('LMI:LMI_SoftwareIdentity:'
                    + pkg.get_nevra(with_epoch="ALWAYS"),
                set(i.InstalledSoftware.InstanceID for i in inames2))

        rpmcache.install_pkg(pkg)
        inames3 = self.cim_class.instance_names()
        self.assertEqual(len(inames1), len(inames3))
        self.assertIn('LMI:LMI_SoftwareIdentity:'
                    + pkg.get_nevra(with_epoch="ALWAYS"),
                set(i.InstalledSoftware.InstanceID for i in inames3))

    @enable_lmi_exceptions
    @mark_dangerous
    def test_create_instance(self):
        """
        Tests new instance creation for dangerous packages.
        """
        for pkg in self.dangerous_pkgs:
            if rpmcache.is_pkg_installed(pkg.name):
                rpmcache.remove_pkg(pkg.name)
            objpath = self.make_op(pkg)
            self.assertFalse(rpmcache.is_pkg_installed(pkg))

            properties = {
                    "InstalledSoftware" : objpath.InstalledSoftware,
                    "System"            : objpath.System
                }

            inst = self.cim_class.create_instance(properties)
            self.assertTrue(rpmcache.is_pkg_installed(pkg))
            self.assertCIMNameEqual(objpath, inst.path,
                    'Object path does not match for %s.' % pkg)

            # try to install second time
            self.assertRaisesCIM(pywbem.CIM_ERR_ALREADY_EXISTS,
                self.cim_class.create_instance, inst.properties_dict())

    @enable_lmi_exceptions
    @mark_dangerous
    def test_delete_instance(self):
        """
        Tests removing of dangerous packages by deleting instance name.
        """
        for pkg in self.dangerous_pkgs:
            self.ensure_pkg_installed(pkg)
            self.assertTrue(rpmcache.is_pkg_installed(pkg))
            objpath = self.make_op(pkg)
            inst = objpath.to_instance()
            inst.delete()
            self.assertFalse(rpmcache.is_pkg_installed(pkg),
                    "Failed to delete instance for %s." % pkg)

            # try to delete second time
            self.assertRaisesCIM(pywbem.CIM_ERR_NOT_FOUND, inst.delete)

    @mark_tedious
    def test_get_system_referents(self):
        """
        Test ReferenceNames for ComputerSystem.
        """
        objpath = self.make_op(self.safe_pkgs[0])
        refs = objpath.System.to_instance().associator_names(
                AssocClass=self.CLASS_NAME,
                Role="System",
                ResultRole="InstalledSoftware",
                ResultClass="LMI_SoftwareIdentity")
        self.assertGreater(len(refs), 0)
        for ref in refs:
            self.assertEqual(ref.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentity")
            self.assertEqual(ref.key_properties(), ["InstanceID"])
            self.assertTrue(ref.InstanceID.startswith(
                "LMI:LMI_SoftwareIdentity:"))

        nevra_set = set(i.InstanceID for i in refs)
        for pkg in self.safe_pkgs:
            nevra = 'LMI:LMI_SoftwareIdentity:' \
                    + pkg.get_nevra(with_epoch="ALWAYS")
            self.assertTrue(nevra in nevra_set, 'Missing nevra "%s".' % nevra)

    def test_get_installed_software_referents(self):
        """
        Test ReferenceNames for SoftwareIdentity.
        """
        for pkg in self.safe_pkgs:
            objpath = self.make_op(pkg)
            refs = objpath.InstalledSoftware.to_instance().associator_names(
                    AssocClass='LMI_InstalledSoftwareIdentity',
                    Role="InstalledSoftware",
                    ResultRole="System",
                    ResultClass=self.system_cs_name)
            self.assertEqual(len(refs), 1)
            ref = refs[0]
            self.assertCIMNameEqual(objpath.System, ref)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestInstalledSoftwareIdentity)

if __name__ == '__main__':
    unittest.main()
