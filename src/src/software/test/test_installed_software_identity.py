#!/usr/bin/env python
#
# Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
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
# Authors: Jan Grec <jgrec@redhat.com>
# Authors: Michal Minar <miminar@redhat.com>
#
"""
Unit tests for ``LMI_InstalledSoftwareIdentity`` provider.
"""

from lmi.test import wbem
from lmi.test import unittest
from lmi.shell import LMIDeletedObjectError
from lmi.test.lmibase import enable_lmi_exceptions
from lmi.test.util import mark_tedious

import package
import swbase
import util

class TestInstalledSoftwareIdentity(swbase.SwTestCase):
    """
    Basic cim operations test on `LMI_InstalledSoftwareIdentity``.
    """

    CLASS_NAME = "LMI_InstalledSoftwareIdentity"
    KEYS = ("InstalledSoftware", "System")

    def make_op(self, pkg):
        """
        :returns Object path of ``LMI_InstalledSoftwareIdentity``.
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        return self.cim_class.new_instance_name({
            "System" : self.system_iname,
            "InstalledSoftware" : self.ns.LMI_SoftwareIdentity. \
                new_instance_name({
                    "InstanceID" : 'LMI:LMI_SoftwareIdentity:' + pkg.nevra
                })
            })

    @swbase.test_with_packages('stable#pkg1')
    def test_get_instance(self):
        """
        Test ``GetInstance()`` call on ``LMI_InstalledSoftwareIdentity``.
        """
        repo = self.get_repo('stable')
        pkg = repo['pkg1']
        objpath = self.make_op(pkg)
        inst = objpath.to_instance()
        self.assertEqual(set(inst.path.key_properties()), set(self.KEYS))
        for key in self.KEYS:
            self.assertCIMNameEqual(
                    getattr(inst, key), getattr(inst.path, key))
        self.assertCIMNameEqual(inst.path, objpath,
                "Object paths should match for package %s" % pkg)

        if not util.USE_PKCON:
            # try it now with CIM_ComputerSystem
            system = objpath.System.copy()
            objpath.System.wrapped_object.classname = "CIM_ComputerSystem"
            objpath.System.wrapped_object.CreationClassName = \
                    "CIM_ComputerSystem"
    
            inst = objpath.to_instance()
            self.assertCIMNameEqual(inst.path, objpath,
                    "Object paths should match for package %s" % pkg)
            for key in self.KEYS:
                self.assertIn(key, inst.properties(),
                        "OP is missing \"%s\" key for package %s" % (key, pkg))
            self.assertCIMNameEqual(inst.System, system)
            self.assertCIMNameEqual(inst.path, objpath)

    @enable_lmi_exceptions
    @swbase.test_with_packages(**{'stable#pkg2' : False })
    def test_get_instance_not_installed(self):
        """
        Test ``GetInstance()`` call on ``LMI_InstalledSoftwareIdentity``.
        """
        repo = self.get_repo('stable')
        pkg = repo['pkg2']
        objpath = self.make_op(pkg)
        self.assertRaisesCIM(wbem.CIM_ERR_NOT_FOUND, objpath.to_instance)

    @mark_tedious
    @swbase.test_with_packages(**{
        'pkg3' : False,
        'pkg4' : False,
        'stable#pkg1' : True,
        'stable#pkg2' : False,
        'updates#*' : False,
        'updates-testing#pkg1' : False,
        'updates-testing#pkg2' : True,
        'misc#conflict1' : False,
        'misc#conflict2' : False,
        'misc#depend1' : False,
        'misc#depend2' : False,
        'misc#pkg2' : False,
        'misc#funny-version' : True,
        'misc#funny-release' : True,
        'misc#unicode-chars' : False,
    })
    def test_enum_instance_names(self):
        """
        Test ``EnumInstanceNames()`` call on ``LMI_InstalledSoftwareIdentity``.
        """
        inames = self.cim_class.instance_names()
        self.assertGreater(len(inames), 0)
        objpath = self.make_op(self.get_repo('stable')['pkg1'])
        for iname in inames:
            self.assertEqual(iname.namespace, 'root/cimv2')
            self.assertEqual(iname.classname, self.CLASS_NAME)
            self.assertEqual(sorted(iname.key_properties()), sorted(self.KEYS))
            self.assertCIMNameEqual(objpath.System, iname.System)
        nevra_set = set(i.InstalledSoftware.InstanceID[
                len('LMI:LMI_SoftwareIdentity:'):] for i in inames)
        db_installed = 0
        for repo in self.repodb.values():
            for pkg in repo.packages:
                if package.is_pkg_installed(pkg):
                    self.assertTrue(pkg.nevra in nevra_set)
                    db_installed += 1
                else:
                    self.assertFalse(pkg.nevra in nevra_set)
        # test that number of tested packages installed really match number
        # of requested packages
        self.assertEqual(db_installed, 4)

    @mark_tedious
    @swbase.test_with_packages(**{
        'pkg3' : False,
        'pkg4' : False,
        'stable#pkg1' : True,
        'stable#pkg2' : False,
        'updates#*' : False,
        'updates-testing#pkg1' : False,
        'updates-testing#pkg2' : True,
        'misc#conflict1' : False,
        'misc#conflict2' : False,
        'misc#depend1' : False,
        'misc#depend2' : False,
        'misc#pkg2' : False,
        'misc#funny-version' : True,
        'misc#funny-release' : True,
    })
    def test_enum_instances(self):
        """
        Test ``EnumInstances() call on ``LMI_InstalledSoftwareIdentity``.
        """
        insts = self.cim_class.instances()
        self.assertGreater(len(insts), 0)
        for inst in insts:
            self.assertEqual(set(inst.path.key_properties()), set(self.KEYS))
            for key in self.KEYS:
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key))
        nevra_set = set(i.InstalledSoftware.InstanceID[
                len('LMI:LMI_SoftwareIdentity:'):] for i in insts)
        db_installed = 0
        for repo in self.repodb.values():
            for pkg in repo.packages:
                if package.is_pkg_installed(pkg):
                    self.assertTrue(pkg.nevra in nevra_set)
                    db_installed += 1
                else:
                    self.assertFalse(pkg.nevra in nevra_set)
        # test that number of tested packages installed really match number
        # of requested packages
        self.assertEqual(db_installed, 4)

    @mark_tedious
    def test_list_installed_packages(self):
        """
        Test list of all installed packages.

        Use LMI_InstalledSoftwareIdentity instances to get installed packages.
        Compare all package names with:
            rpm -qa
        Cases:
            OpenLMI package list matches yum package list.
        """
        package_list = []
        for iname in self.cim_class.instance_names():
            path = iname.InstalledSoftware.InstanceID
            name = path[len("LMI:LMI_SoftwareIdentity:"):]
            package_list.append(name)

        self.assertNotEqual(len(package_list), 0,
                            "OpenLMI returned empty list of installed packages")

        rpm_packages = util.get_installed_packages()
        self.assertEqual(sorted(package_list), sorted(rpm_packages))

    @swbase.test_with_repos('stable',
            **{'updates' : False, 'updates-testing' : False, 'misc' : False })
    @swbase.test_with_packages(**{'stable#pkg1' : False})
    def test_install_package(self):
        """
        Test package installation  with ``LMI_InstalledSoftwareIdentity``.
        """
        repo = self.get_repo('stable')
        pkg = repo['pkg1']
        self.assertFalse(package.is_pkg_installed(pkg))
        objpath = self.make_op(pkg)

        properties = {
                "InstalledSoftware" : objpath.InstalledSoftware,
                "System"            : objpath.System
            }

        inst = self.cim_class.create_instance(properties)
        self.assertTrue(package.is_pkg_installed(pkg))
        self.assertCIMNameEqual(inst.path, objpath,
                'Object path does not match for %s.' % pkg)

    @swbase.test_with_repos('stable', 'misc',
            **{ 'updates' : False, 'updates-testing' : False })
    @swbase.test_with_packages(**{'pkg2' : False})
    def test_install_package_same_arch(self):
        """
        Test package installation  with ``LMI_InstalledSoftwareIdentity`` of
        package with specific architecture.
        """
        pkg_arch = self.get_repo('misc')['pkg2']
        op_arch = self.make_op(pkg_arch)
        properties = {
                "InstalledSoftware" : op_arch.InstalledSoftware,
                "System"            : op_arch.System
            }

        inst = self.cim_class.create_instance(properties)
        self.assertTrue(package.is_pkg_installed(pkg_arch))
        self.assertCIMNameEqual(inst.path, op_arch,
                'Object path does not match for %s.' % pkg_arch)

    @enable_lmi_exceptions
    @swbase.test_with_repos('stable',
            **{'updates' : False, 'updates-testing' : False, 'misc' : False })
    @swbase.test_with_packages('stable#pkg1')
    def test_install_already_installed(self):
        """
        Try to install package that is already installed with
        ``LMI_InstalledSoftwareIdentity``.
        """
        pkg = self.get_repo('stable')['pkg1']
        self.assertTrue(package.is_pkg_installed(pkg))
        objpath = self.make_op(pkg)

        properties = {
                "InstalledSoftware" : objpath.InstalledSoftware,
                "System"            : objpath.System
            }
        # try to install second time
        self.assertRaisesCIM(wbem.CIM_ERR_ALREADY_EXISTS,
            self.cim_class.create_instance, properties)

    @swbase.test_with_repos(**{'stable' : False})
    @swbase.test_with_packages(**{'stable#pkg1' : False})
    def test_install_from_disabled_repo(self):
        pkg = self.get_repo('stable')['pkg1']
        self.assertFalse(package.is_pkg_installed(pkg))
        objpath = self.make_op(pkg)

        properties = {
                "InstalledSoftware" : objpath.InstalledSoftware,
                "System"            : objpath.System
            }
        # try to install it
        inst = self.cim_class.create_instance(properties)
        self.assertEqual(inst, None)

    @enable_lmi_exceptions
    @swbase.test_with_packages('stable#pkg2')
    def test_remove_installed(self):
        """
        Test package removal with ``LMI_InstalledSoftwareIdentity``.
        """
        pkg = self.get_repo('stable')['pkg2']
        self.assertTrue(package.is_pkg_installed(pkg))
        objpath = self.make_op(pkg)
        inst = objpath.to_instance()
        inst.delete()
        self.assertFalse(package.is_pkg_installed(pkg))

        # try to delete second time
        self.assertRaises(LMIDeletedObjectError, inst.delete)
        self.assertRaisesCIM(wbem.CIM_ERR_NOT_FOUND, objpath.to_instance)

    @mark_tedious
    @swbase.test_with_packages(**{
        'updates#pkg3' : True,
        'stable#pkg1' : True,
        'pkg2' : False,
        'pkg4' : False,
        'updates-testing#*' : False,
        'misc#*' : False
    })
    def test_get_installed_package_names(self):
        """
        Try to get installed packages associated with computer system
        via ``LMI_InstalledSoftwareIdentity``.
        """
        pkg = self.get_repo('stable')['pkg1']
        objpath = self.make_op(pkg)
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
        nevra_set = set(   i.InstanceID[len('LMI:LMI_SoftwareIdentity:'):]
                       for i in refs)

        db_installed = 0
        for repo in self.repodb.values():
            for pkg in repo.packages:
                if package.is_pkg_installed(pkg):
                    self.assertTrue(pkg.nevra in nevra_set)
                    db_installed += 1
                else:
                    self.assertFalse(pkg.nevra in nevra_set)
        # test that number of tested packages installed really match number
        # of requested packages
        self.assertEqual(db_installed, 2)

    @swbase.test_with_packages('stable#pkg1')
    def test_get_installed_software_referents(self):
        """
        Try to get computer system associated with installed package via
        ``LMI_InstalledSoftwareIdentity``.
        """
        pkg = self.get_repo('stable')['pkg1']
        self.assertTrue(package.is_pkg_installed(pkg))
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
