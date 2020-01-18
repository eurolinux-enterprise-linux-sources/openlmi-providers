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
Unit tests for ``LMI_SoftwareIdentityChecks`` provider.
"""

import os
import pywbem
import subprocess
import unittest

from lmi.test.lmibase import enable_lmi_exceptions
import package
import swbase
import util

SOFTWARE_ELEMENT_STATE_EXECUTABLE = 2

class TestSoftwareIdentityChecks(swbase.SwTestCase):
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_SoftwareIdentityChecks"
    KEYS = ("Check", "Element")

    def make_op(self, pkg, filepath):
        """
        :returns: Object path of ``LMI_SoftwareIdentityChecks``
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        return self.cim_class.new_instance_name({
            "Check" : self.ns.LMI_SoftwareIdentityFileCheck.new_instance_name({
                "CheckID" : 'LMI:LMI_SoftwareIdentityFileCheck',
                "Name" : filepath,
                "SoftwareElementID" : pkg.nevra,
                "SoftwareElementState" : SOFTWARE_ELEMENT_STATE_EXECUTABLE,
                "TargetOperatingSystem" : util.get_target_operating_system(),
                "Version" : pkg.evra
                }),
            "Element" : self.ns.LMI_SoftwareIdentity.new_instance_name({
                "InstanceID" : 'LMI:LMI_SoftwareIdentity:' + pkg.nevra
                })
            })

    def setUp(self):
        to_uninstall = set()
        for repo in self.repodb.values():
            for pkg in repo.packages:
                to_uninstall.add(pkg.name)
        to_uninstall = list(package.filter_installed_packages(to_uninstall))
        package.remove_pkgs(to_uninstall, suppress_stderr=True)

    @swbase.test_with_packages('stable#pkg1')
    def test_get_instance(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentityChecks``.
        """
        pkg = self.get_repo('stable')['pkg1']
        count = 0
        for filepath in pkg:
            objpath = self.make_op(pkg, filepath)
            inst = objpath.to_instance()
            self.assertNotEqual(inst, None,
                    "failed to get instance for %s:%s" % (pkg, filepath))
            self.assertCIMNameEqual(inst.path, objpath)
            for key in self.KEYS:
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key))
            count += 1
        self.assertEqual(count, 7, "there are 7 files in pkg1")

    @enable_lmi_exceptions
    @swbase.test_with_packages('stable#pkg1')
    def test_get_instance_invalid(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentityChecks``
        on not installed file.
        """
        pkg = self.get_repo('stable')['pkg1']
        objpath = self.make_op(pkg, '/not-installed-file')
        self.assertRaisesCIM(pywbem.CIM_ERR_NOT_FOUND, objpath.to_instance)

    @enable_lmi_exceptions
    def test_enum_instance_names(self):
        """
        Test ``EnumInstanceNames()`` call on ``LMI_SoftwareIdentityChecks``.
        Should not be supported.
        """
        self.assertRaisesCIM(pywbem.CIM_ERR_NOT_SUPPORTED,
                self.cim_class.instance_names)

    @swbase.test_with_packages('stable#pkg3')
    def test_package_file_checks(self):
        """
        Try to get file checks associated with package.
        """
        pkg = self.get_repo('stable')['pkg3']
        filepath = '/usr/share/openlmi-sw-test-pkg3'
        objpath = self.make_op(pkg, filepath)
        inst = objpath.Element.to_instance()
        self.assertNotEqual(inst, None,
                "get instance on %s:%s succeeds" % (pkg, filepath))
        refs = inst.associators(
                AssocClass=self.CLASS_NAME,
                Role="Element",
                ResultRole="Check",
                ResultClass="LMI_SoftwareIdentityFileCheck")
        self.assertEqual(len(refs), len(pkg))
        self.assertEqual(len(refs), 10, "there are 10 files in %s" % pkg.name)
        pkg_names = set(pkg.files)
        for ref in refs:
            self.assertIn(ref.Name, set(r.Name for r in refs))
            self.assertEqual(sorted(ref.path.key_properties()),
                ["CheckID", "Name", "SoftwareElementID",
                    "SoftwareElementState",
                    "TargetOperatingSystem", "Version"])
            self.assertEqual(ref.SoftwareElementID, pkg.nevra)
            self.assertIn(ref.Name, pkg_names)
            self.assertEqual(ref.FailedFlags, [],
                    "FailedFlags are empty for unmodified file %s:%s"
                    % (pkg, filepath))
            pkg_names.remove(ref.Name)
        self.assertEqual(len(pkg_names), 0)

    @swbase.test_with_packages('stable#pkg3')
    def test_package_file_check_names(self):
        """
        Try to get file check names associated with package.
        """
        pkg = self.get_repo('stable')['pkg3']
        objpath = self.make_op(pkg, '/usr/share/openlmi-sw-test-pkg3')
        refs = objpath.Element.to_instance().associator_names(
                AssocClass=self.CLASS_NAME,
                Role="Element",
                ResultRole="Check",
                ResultClass="LMI_SoftwareIdentityFileCheck")
        self.assertEqual(len(refs), len(pkg))
        self.assertEqual(len(refs), 10, "there are 10 files in %s" % pkg.name)
        pkg_names = set(pkg.files)
        for ref in refs:
            self.assertIn(ref.Name, set(r.Name for r in refs))
            self.assertEqual(sorted(ref.key_properties()),
                ["CheckID", "Name", "SoftwareElementID",
                    "SoftwareElementState",
                    "TargetOperatingSystem", "Version"])
            self.assertEqual(ref.SoftwareElementID, pkg.nevra)
            self.assertIn(ref.Name, pkg_names)
            pkg_names.remove(ref.Name)
        self.assertEqual(len(pkg_names), 0)

    @swbase.test_with_packages('stable#pkg3')
    def test_file_check_package(self):
        """
        Try to get package assocatied to file check.
        """
        pkg = self.get_repo('stable')['pkg3']
        filepath = '/usr/share/openlmi-sw-test-pkg3/README'
        objpath = self.make_op(pkg, filepath)
        inst = objpath.Check.to_instance()
        self.assertNotEqual(inst, None,
                "get instance for %s:%s succeeds" % (pkg, filepath))
        refs = inst.associators(
                AssocClass=self.CLASS_NAME,
                Role="Check",
                ResultRole="Element",
                ResultClass="LMI_SoftwareIdentity")
        self.assertEqual(len(refs), 1)
        ref = refs[0]
        self.assertCIMNameEqual(ref.path, objpath.Element)
        self.assertEqual(ref.ElementName, pkg.nevra)
        self.assertEqual(ref.VersionString, pkg.evra)

        # try with removed file
        os.remove(filepath)
        refs = objpath.Check.to_instance().associators(
                AssocClass=self.CLASS_NAME,
                Role="Check",
                ResultRole="Element",
                ResultClass="LMI_SoftwareIdentity")
        self.assertEqual(len(refs), 1)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareIdentityChecks)

if __name__ == '__main__':
    unittest.main()
