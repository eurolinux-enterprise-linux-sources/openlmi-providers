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
# Authors: Michal Minar <miminar@redhat.com>
#
"""
Unit tests for ``LMI_SoftwareIdentity`` provider.
"""

from datetime import datetime, timedelta

from lmi.test import unittest
from lmi.test import wbem
from lmi.test.lmibase import enable_lmi_exceptions

import package
import swbase
import util

class TestSoftwareIdentity(swbase.SwTestCase):
    """
    Basic cim operations test on ``LMI_SoftwareIdentity``.
    """

    CLASS_NAME = "LMI_SoftwareIdentity"
    KEYS = ("InstanceID", )

    def make_op(self, pkg):
        """
        :returns: Object path of ``LMI_SoftwareIdentity``
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        return util.make_pkg_op(self.ns, pkg)

    def _check_package_instance(self, pkg, inst):
        """
        Check properties of ``LMI_SoftwareIdentity`` instance and its
        correspondence to represented *pkg*.
        """
        self.assertNotEqual(inst, None,
                "instance must exist for package %s" % pkg)
        objpath = self.make_op(pkg)
        self.assertCIMNameEqual(objpath, inst.path)
        for key in self.KEYS:
            self.assertEqual(getattr(inst, key), getattr(inst.path, key))
        self.assertEqual(set(inst.path.key_properties()), set(self.KEYS))
        self.assertEqual(pkg.name, inst.Name)

        self.assertEqual(inst.InstanceID, 'LMI:LMI_SoftwareIdentity:'
                + pkg.nevra)
        self.assertEqual(inst.Caption, pkg.summary)
        self.assertIsInstance(inst.Description, basestring)
        self.assertEqual(inst.VersionString, pkg.evra,
                "VersionString does not match evra for pkg %s" % pkg)
        self.assertTrue(inst.IsEntity)
        self.assertEqual(inst.Name, pkg.name,
                "Name does not match for pkg %s" % pkg)
        self.assertNotEqual(inst.Epoch, None,
                "Epoch does not match for pkg %s" % pkg)
        self.assertEqual(inst.Epoch, pkg.epoch)
        self.assertEqual(inst.Version, pkg.ver,
                "Version does not match for pkg %s" % pkg)
        self.assertEqual(inst.Release, pkg.rel,
                "Release does not match for pkg %s" % pkg)
        self.assertEqual(inst.Architecture, pkg.arch,
                "Architecture does not match for pkg %s" % pkg)
        self.assertEqual(inst.ElementName, pkg.nevra,
                "ElementName must match nevra of pkg %s" % pkg)
        self.assertTrue(inst.InstanceID.endswith(inst.ElementName),
                "InstanceID has package nevra at its end for pkg %s" % pkg)

        if not util.USE_PKCON and package.is_pkg_installed(pkg):
            self.assertNotEqual(inst.InstallDate, None)
        else:
            self.assertEqual(inst.InstallDate, None)

    @swbase.test_with_packages(**{'stable#pkg1' : True})
    def test_get_instance_installed(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentity`` with
        installed package.
        """
        pkg = self.get_repo('stable')['pkg1']
        self.assertTrue(package.is_pkg_installed(pkg))

        objpath = self.make_op(pkg)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None, "GetInstance is successful on %s" % pkg)
        if not util.USE_PKCON:
            self.assertIsNotNone(inst.InstallDate)
            time_stamp = datetime.now(
                    # lmiwbem has differently named modules
                    getattr(wbem, 'lmiwbem_types',
                        getattr(wbem, 'cim_types', None)).MinutesFromUTC(0)) \
                    - timedelta(seconds=5)
            self.assertGreater(inst.InstallDate.datetime, time_stamp)
        self._check_package_instance(pkg, inst)

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages(**{'stable#pkg2' : False})
    def test_get_instance_not_installed(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentity`` with not
        installed package.
        """
        pkg = self.get_repo('stable')['pkg2']
        self.assertFalse(package.is_pkg_installed(pkg))

        self._check_package_instance(pkg, self.make_op(pkg).to_instance())

    @swbase.test_with_packages('stable#pkg1', 'stable#pkg4')
    def test_get_instance_without_epoch(self):
        """
        Test ``GetInstance()`` on ``LMI_SoftwareIdentity`` with epoch part
        omitted.
        """
        pkg = self.get_repo('stable')['pkg1']
        objpath = self.make_op(pkg)
        objpath.wrapped_object['InstanceID'] = 'LMI:LMI_SoftwareIdentity:' \
                + pkg.get_nevra('NEVER')
        self.assertFalse('0:' in objpath.InstanceID)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)
        self.assertTrue('0:' in pkg.nevra)
        self.assertTrue(inst.InstanceID.endswith(pkg.nevra))

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages(**{'stable#pkg3' : False})
    def test_refresh_after_install(self):
        """
        Test instance refresh of ``LMI_SoftwareIdentity`` after package
        installation.
        """
        pkg = self.get_repo('stable')['pkg3']
        self.assertFalse(package.is_pkg_installed(pkg))
        inst = self.make_op(pkg).to_instance()
        self.assertEqual(inst.InstallDate, None)
        self._check_package_instance(pkg, inst)
        refs = inst.associator_names(
                AssocClass="LMI_InstalledSoftwareIdentity",
                Role="InstalledSoftware",
                ResultRole="System",
                ResultClass=self.system_cs_name)
        self.assertEqual(len(refs), 0)

        # install it
        package.install_pkgs(pkg)
        self.assertTrue(package.is_pkg_installed(pkg))
        inst.refresh()
        self._check_package_instance(pkg, inst)
        refs = inst.associator_names(
                AssocClass="LMI_InstalledSoftwareIdentity",
                Role="InstalledSoftware",
                ResultRole="System",
                ResultClass=self.system_cs_name)
        self.assertEqual(len(refs), 1)
        if not util.USE_PKCON:
            self.assertNotEqual(inst.InstallDate, None)

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages('stable#pkg4')
    def test_refresh_after_uninstall(self):
        """
        Test instance refresh of ``LMI_SoftwareIdentity`` after package
        uninstallation.
        """
        pkg = self.get_repo('stable')['pkg4']
        self.assertTrue(package.is_pkg_installed(pkg))
        inst = self.make_op(pkg).to_instance()
        self._check_package_instance(pkg, inst)
        refs = inst.associator_names(
                AssocClass="LMI_InstalledSoftwareIdentity",
                Role="InstalledSoftware",
                ResultRole="System",
                ResultClass=self.system_cs_name)
        self.assertEqual(len(refs), 1)
        if not util.USE_PKCON:
            self.assertNotEqual(inst.InstallDate, None)

        # uninstall it
        package.remove_pkgs(pkg.name)
        self.assertFalse(package.is_pkg_installed(pkg))
        inst.refresh()
        self.assertEqual(inst.InstallDate, None)
        self._check_package_instance(pkg, inst)
        refs = inst.associator_names(
                AssocClass="LMI_InstalledSoftwareIdentity",
                Role="InstalledSoftware",
                ResultRole="System",
                ResultClass=self.system_cs_name)
        self.assertEqual(len(refs), 0)

    @enable_lmi_exceptions
    @swbase.test_with_packages('stable#pkg3')
    def test_get_instance_invalid(self):
        """
        Test ``GetInstance()`` call on invalid object path.
        """
        pkg = self.get_repo('stable')['pkg3']
        # leave out arch part
        nvr = '%s-%s-%s' % (pkg.name, pkg.ver, pkg.rel)
        objpath = self.make_op(nvr)
        self.assertRaisesCIM(wbem.CIM_ERR_NOT_FOUND, objpath.to_instance)

        # leave out version part
        nra = '%s-%s.%s' % (pkg.name, pkg.rel, pkg.arch)
        objpath = self.make_op(nra)
        self.assertRaisesCIM(wbem.CIM_ERR_NOT_FOUND, objpath.to_instance)

        # leave out release part
        nva = '%s-%s.%s' % (pkg.name, pkg.ver, pkg.arch)
        objpath = self.make_op(nva)
        self.assertRaisesCIM(wbem.CIM_ERR_NOT_FOUND, objpath.to_instance)

        # leave out name part
        vra = '%s-%s.%s' % (pkg.ver, pkg.rel, pkg.arch)
        objpath = self.make_op(vra)
        if util.USE_PKCON:
            self.assertRaisesCIM(wbem.CIM_ERR_NOT_FOUND, objpath.to_instance)
        else:
            self.assertRaisesCIM(wbem.CIM_ERR_INVALID_PARAMETER,
                    objpath.to_instance)

        # leave out just epoch
        nvra = '%s-%s-%s.%s' % (pkg.name, pkg.ver, pkg.rel, pkg.arch)
        objpath = self.make_op(nvra)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)

    @swbase.test_with_packages('misc#funny-version', 'misc#funny-release')
    def test_get_instance_funny(self):
        """
        Test ``GetInstance()`` on package with quite odd nevra.
        """
        pkg = self.get_repo('misc')['funny-version']
        self.assertTrue(package.is_pkg_installed(pkg))
        objpath = self.make_op(pkg)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None, "GetInstance is successful on %s" % pkg)
        self._check_package_instance(pkg, inst)

        pkg = self.get_repo('misc')['funny-release']
        self.assertTrue(package.is_pkg_installed(pkg))
        objpath = self.make_op(pkg)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None, "GetInstance is successful on %s" % pkg)
        self._check_package_instance(pkg, inst)

    @swbase.run_for_backends('yum')
    @enable_lmi_exceptions
    def test_enum_instance_names(self):
        """
        Test ``EnumInstanceNames()`` call on ``LMI_SoftwareIdentity``.
        """
        self.assertRaisesCIM(wbem.CIM_ERR_NOT_SUPPORTED,
                self.cim_class.instance_names)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareIdentity)

if __name__ == '__main__':
    unittest.main()
