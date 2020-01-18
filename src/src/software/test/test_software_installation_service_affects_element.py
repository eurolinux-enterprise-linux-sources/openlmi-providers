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
Unit tests for ``LMI_SoftwareInstallationServiceAffectsElement`` provider.
"""

import unittest

import swbase

class TestSoftwareInstallationServiceAffectsElement(swbase.SwTestCase):
    """
    Basic cim operations test for
    ``LMI_SoftwareInstallationServiceAffectsElement``.
    """

    CLASS_NAME = "LMI_SoftwareInstallationServiceAffectsElement"
    KEYS = ("AffectedElement", "AffectingElement")

    def make_op(self, pkg=None):
        """
        :returns Object path of
            ``LMI_SoftwareInstallationServiceAffectsElement``.
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        op = self.cim_class.new_instance_name({
            "AffectingElement" : self.ns.LMI_SoftwareInstallationService. \
                new_instance_name({
                    "Name" : 'LMI:LMI_SoftwareInstallationService',
                    "SystemName" : self.SYSTEM_NAME,
                    "SystemCreationClassName" : self.system_cs_name,
                    "CreationClassName" : "LMI_SoftwareInstallationService"
                }),
            })
        if pkg is not None:
            op.wrapped_object["AffectedElement"] = \
                    self.ns.LMI_SoftwareIdentity.new_instance_name({
                        "InstanceID" : 'LMI:LMI_SoftwareIdentity:' + pkg.nevra
                    }).wrapped_object
        else:
            op.wrapped_object["AffectedElement"] = \
                    self.system_iname.wrapped_object
        return op

    @swbase.test_with_repos(**{
        'updates' : True,
        'misc' : True,
        'stable' : False,
        'updates-testing': False
    })
    @swbase.test_with_packages(**{
        'updates#pkg1' : True,
        'pkg2' : False,
        'pkg3' : False,
        'pkg4' : False
    })
    def test_get_instance_on_available_packages(self):
        """
        Test ``GetInstance()`` call on
        ``LMI_SoftwareInstallationServiceAffectsElement``.
        """
        for repoid in ('updates', 'misc'):
            repo = self.get_repo(repoid)
            for pkg in repo.packages:
                objpath = self.make_op(pkg)
                inst = objpath.to_instance()
                self.assertNotEqual(inst, None,
                        'Package "%s" is affected by installation service'
                        % pkg)
                self.assertCIMNameEqual(inst.path, objpath,
                        "Object paths should match for package %s" % pkg)
                self.assertEqual(set(inst.path.key_properties()),
                        set(self.KEYS))

    @swbase.test_with_repos(misc=False)
    @swbase.test_with_packages(**{
        'misc#pkg2' : True,
    })
    def test_get_instance_with_not_available_package(self):
        """
        Test ``GetInstance()`` call on
        ``LMI_SoftwareInstallationServiceAffectsElement`` with not available
        but installed package.
        """
        repo = self.get_repo('misc')
        for pkg in repo.packages:
            objpath = self.make_op(pkg)
            inst = objpath.to_instance()
            self.assertEqual(inst, None, 'package should not be affected')

    def test_get_instance_on_computer_system(self):
        """
        Test ``GetInstance()`` call on
        ``LMI_SoftwareInstallationServiceAffectsElement`` on computer system.
        """
        objpath = self.make_op()
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None, "computer system shall be affected")
        self.assertCIMNameEqual(inst.path, objpath)

    @swbase.test_with_repos(**{
        'stable' : True,
        'updates': False,
        'updates-testing': True,
        'misc' : False
    })
    @swbase.test_with_packages(**{
        'stable#pkg1' : True,
        'updates#pkg2' : True,
        'pkg3' : False,
        'pkg4' : False
    })
    def test_enum_instances(self):
        """
        Test ``EnumInstances()`` call on
        ``LMI_SoftwareInstallationServiceAffectsElement``.
        """
        insts = self.cim_class.instances()
        self.assertGreater(len(insts), 0)
        for inst in insts:
            self.assertEqual(set(inst.path.key_properties()), set(self.KEYS))
            for key in self.KEYS:
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key))

        nevra_set = set(i.AffectedElement.InstanceID[
                len('LMI:LMI_SoftwareIdentity:'):] for i in insts
                if i.AffectedElement.classname == 'LMI_SoftwareIdentity')
        # There is also computer system associated
        self.assertEqual(len(nevra_set) + 1, len(insts))
        for repo in self.repodb.values():
            if not repo.repoid in (
                    'openlmi-sw-test-repo-stable',
                    'openlmi-sw-test-repo-updates-testing'):
                continue
            for pkg in repo.packages:
                self.assertIn(pkg.nevra, nevra_set)
                nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)

        objpath = self.make_op()
        cs = None # computer system instance name
        for i in insts:
            if i.AffectedElement.classname == objpath.AffectedElement.classname:
                cs = i.AffectedElement
                break
        self.assertNotEqual(cs, None,
                "computer system is one of affected elements")
        self.assertCIMNameEqual(cs, objpath.AffectedElement)

    @swbase.test_with_repos(**{
        'stable' : True,
        'updates': False,
        'updates-testing': True,
        'misc' : False
    })
    def test_enum_instance_names(self):
        """
        Test ``EnumInstanceNames()`` call on
        ``LMI_SoftwareInstallationServiceAffectsElement``.
        """
        inames = self.cim_class.instance_names()
        self.assertGreater(len(inames), 1)
        objpath = self.make_op(self.get_repo('stable')['pkg1'])
        for iname in inames:
            self.assertEqual(iname.namespace, 'root/cimv2')
            self.assertEqual(iname.classname, self.CLASS_NAME)
            self.assertEqual(sorted(iname.key_properties()), sorted(self.KEYS))
            self.assertCIMNameEqual(objpath.AffectingElement,
                    iname.AffectingElement)
        nevra_set = set(i.AffectedElement.InstanceID[
                len('LMI:LMI_SoftwareIdentity:'):] for i in inames
                if i.AffectedElement.classname == 'LMI_SoftwareIdentity')
        # There is also computer system associated
        self.assertEqual(len(nevra_set) + 1, len(inames))
        for repo in self.repodb.values():
            if not repo.repoid in (
                    'openlmi-sw-test-repo-stable',
                    'openlmi-sw-test-repo-updates-testing'):
                continue
            for pkg in repo.packages:
                self.assertIn(pkg.nevra, nevra_set)
                nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)

        objpath = self.make_op()
        cs = None # computer system instance name
        for i in inames:
            if i.AffectedElement.classname == objpath.AffectedElement.classname:
                cs = i.AffectedElement
                break
        self.assertNotEqual(cs, None,
                "computer system is one of affected elements")
        self.assertCIMNameEqual(cs, objpath.AffectedElement)

    def test_get_service_computer_system(self):
        """
        Try to get computer system associated with service.
        """
        objpath = self.make_op()
        service = objpath.AffectingElement.to_instance()
        self.assertNotEqual(service, None,
                "GetInstance call succeeds on service")
        refs = service.associator_names(
                AssocClass='LMI_SoftwareInstallationServiceAffectsElement',
                ResultClass=self.system_cs_name,
                Role="AffectingElement",
                ResultRole="AffectedElement")
        self.assertEqual(len(refs), 1)
        cs = refs[0]
        self.assertCIMNameEqual(cs, objpath.AffectedElement)

    @swbase.test_with_repos(**{
        'updates' : True,
        'misc' : True,
        'stable' : False,
        'updates-testing' : False
    })
    def test_get_service_package_names(self):
        """
        Try to get all package names associated with installation service.
        """
        objpath = self.make_op()
        service = objpath.AffectingElement.to_instance()
        self.assertNotEqual(service, None,
                "GetInstance call succeeds on service")
        refs = service.associator_names(
                AssocClass='LMI_SoftwareInstallationServiceAffectsElement',
                ResultClass="LMI_SoftwareIdentity",
                Role="AffectingElement",
                ResultRole="AffectedElement")
        self.assertGreater(len(refs), 0)

        for ref in refs:
            self.assertEqual(ref.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentity")
            self.assertEqual(ref.key_properties(), ["InstanceID"])
            self.assertTrue(ref.InstanceID.startswith(
                "LMI:LMI_SoftwareIdentity:"))

        nevra_set = set(   i.InstanceID[len('LMI:LMI_SoftwareIdentity:'):]
                       for i in refs)
        for repo in self.repodb.values():
            if not repo.repoid in (
                    'openlmi-sw-test-repo-updates',
                    'openlmi-sw-test-repo-misc'):
                continue
            for pkg in repo.packages:
                self.assertIn(pkg.nevra, nevra_set)
                nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)

    def test_get_computer_system_service(self):
        """
        Try to get software installation service associated with computer
        system.
        """
        objpath = self.make_op()
        refs = objpath.AffectedElement.to_instance().associator_names(
                AssocClass=self.CLASS_NAME,
                Role="AffectedElement",
                ResultRole="AffectingElement",
                ResultClass='LMI_SoftwareInstallationService')
        self.assertEqual(len(refs), 1)
        ref = refs[0]
        self.assertCIMNameEqual(objpath.AffectingElement, ref)

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages(**{
        'stable#pkg1' : False
    })
    def test_get_package_installation_service(self):
        """
        Try to get installation service associated to package.
        """
        pkg = self.get_repo('stable')['pkg1']
        objpath = self.make_op(pkg)
        refs = objpath.AffectedElement.to_instance().associator_names(
                AssocClass=self.CLASS_NAME,
                Role="AffectedElement",
                ResultRole="AffectingElement",
                ResultClass='LMI_SoftwareInstallationService')
        self.assertEqual(len(refs), 1)
        ref = refs[0]
        self.assertCIMNameEqual(objpath.AffectingElement, ref)

    @swbase.test_with_repos(**{
        'stable' : True,
        'updates' : False,
        'updates-testing' : False,
        'misc' : False
    })
    @swbase.test_with_packages(**{
        'stable#pkg1' : True,
        'pkg2' : False,
        'stable#pkg3' : True,
        'pkg3' : False
    })
    def test_service_affected_elements(self):
        """
        Try to get all affected elements associated with installation service.
        """
        pkg = self.get_repo('stable')['pkg1']
        objpath = self.make_op(pkg)
        service = objpath.AffectingElement.to_instance()
        self.assertNotEqual(service, None)
        refs = service.associators(
                AssocClass=self.CLASS_NAME,
                Role="AffectingElement",
                ResultRole="AffectedElement")
        # number of packages in stable (4) + computer system (1)
        self.assertEqual(len(refs), 5)
        nevra_set = set()
        for ref in refs:
            if ref.classname == self.system_cs_name:
                self.assertCIMNameEqual(ref.path, self.system_iname)
            else:
                self.assertEqual(ref.classname, 'LMI_SoftwareIdentity')
                self.assertEqual(ref.namespace, 'root/cimv2')
                nevra_set.add(ref.ElementName)
        for pkg in self.get_repo('stable').packages:
            self.assertIn(pkg.nevra, nevra_set)
            nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareInstallationServiceAffectsElement)

if __name__ == '__main__':
    unittest.main()
