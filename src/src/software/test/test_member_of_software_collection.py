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
Unit tests for ``LMI_MemberOfSoftwareCollection`` provider.
"""

from lmi.test import unittest

import swbase

class TestMemberOfSoftwareCollection(swbase.SwTestCase):
    """
    Basic cim operations test for ``LMI_MemberOfSoftwareCollection``.
    """

    CLASS_NAME = "LMI_MemberOfSoftwareCollection"
    KEYS = ("Collection", "Member")

    def make_op(self, pkg):
        """
        :returns Object path of ``LMI_MemberOfSoftwareCollection``.
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        return self.cim_class.new_instance_name({
            "Collection" : self.ns.LMI_SystemSoftwareCollection. \
                new_instance_name({
                    "InstanceID" : "LMI:LMI_SystemSoftwareCollection"
                }),
            "Member" : self.ns.LMI_SoftwareIdentity.new_instance_name({
                "InstanceID" : 'LMI:LMI_SoftwareIdentity:' + pkg.nevra
                })
            })

    @swbase.test_with_repos('stable', 'misc', 'updates-testing')
    def test_get_instance(self):
        """
        Test ``GetInstance()`` call on ``LMI_MemberOfSoftwareCollection``.
        """
        for repoid in ('stable', 'misc', 'updates-testing'):
            repo = self.get_repo(repoid)
            for pkg in repo.packages:
                objpath = self.make_op(pkg)
                inst = objpath.to_instance()
                self.assertNotEqual(inst, None,
                        'Package "%s" is available in SystemSoftwareCollection'
                        % pkg)
                self.assertCIMNameEqual(inst.path, objpath,
                        "Object paths should match for package %s" % pkg)
                self.assertEqual(set(inst.path.key_properties()),
                        set(self.KEYS))

    @swbase.test_with_repos(misc=False)
    def test_get_instance_with_disabled_repo(self):
        """
        Test ``GetInstance()`` call on ``LMI_MemberOfSoftwareCollection``
        with package from disabled repository.
        """
        repo = self.get_repo('misc')
        for pkg in repo.packages:
            objpath = self.make_op(pkg)
            inst = objpath.to_instance()
            self.assertEqual(inst, None,
                'package should not be accessible from disabled repository.')

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
        Test ``EnumInstances()`` call on ``LMI_MemberOfSoftwareCollection``.
        """
        insts = self.cim_class.instances()
        self.assertGreater(len(insts), 0)
        for inst in insts:
            self.assertEqual(set(inst.path.key_properties()), set(self.KEYS))
            for key in self.KEYS:
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key))
        nevra_set = set(i.Member.InstanceID[
                len('LMI:LMI_SoftwareIdentity:'):] for i in insts)
        for repo in self.repodb.values():
            if not repo.repoid in (
                    'openlmi-sw-test-repo-stable',
                    'openlmi-sw-test-repo-updates-testing'):
                continue
            for pkg in repo.packages:
                self.assertIn(pkg.nevra, nevra_set)
                nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)

    @swbase.test_with_repos(**{
        'stable' : True,
        'updates': False,
        'updates-testing': True,
        'misc' : False
    })
    def test_enum_instance_names(self):
        """
        Test ``EnumInstanceNames()`` call on ``LMI_MemberOfSoftwareCollection``.
        """
        inames = self.cim_class.instance_names()
        self.assertGreater(len(inames), 0)
        objpath = self.make_op(self.get_repo('stable')['pkg1'])
        for iname in inames:
            self.assertEqual(iname.namespace, 'root/cimv2')
            self.assertEqual(iname.classname, self.CLASS_NAME)
            self.assertEqual(sorted(iname.key_properties()), sorted(self.KEYS))
            self.assertCIMNameEqual(objpath.Collection, iname.Collection)
        nevra_set = set(i.Member.InstanceID[
                len('LMI:LMI_SoftwareIdentity:'):] for i in inames)
        for repo in self.repodb.values():
            if not repo.repoid in (
                    'openlmi-sw-test-repo-stable',
                    'openlmi-sw-test-repo-updates-testing'):
                continue
            for pkg in repo.packages:
                self.assertIn(pkg.nevra, nevra_set)
                nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)


    @swbase.test_with_repos('stable')
    def test_get_package_collections(self):
        """
        Try to get instance of software collection associated with package.
        """
        repo = self.get_repo('stable')
        for pkg in repo.packages:
            objpath = self.make_op(pkg)
            inst = objpath.Member.to_instance()
            self.assertNotEqual(inst, None,
                    "package %s is available" % pkg)
            refs = inst.associators(
                    AssocClass=self.CLASS_NAME,
                    Role="Member",
                    ResultRole="Collection",
                    ResultClass="LMI_SystemSoftwareCollection")
            self.assertEqual(len(refs), 1,
                "exactly one collection is associated with package %s" % pkg)
            self.assertCIMNameEqual(refs[0].path, objpath.Collection)
            self.assertEqual(refs[0].InstanceID, objpath.Collection.InstanceID)

    @swbase.test_with_repos('stable')
    def test_get_package_collection_names(self):
        """
        Try to get instance of software collection associated with package.
        """
        repo = self.get_repo('stable')
        for pkg in repo.packages:
            objpath = self.make_op(pkg)
            inst = objpath.Member.to_instance()
            self.assertNotEqual(inst, None,
                    "package %s is available" % pkg)
            refs = inst.associator_names(
                    AssocClass=self.CLASS_NAME,
                    Role="Member",
                    ResultRole="Collection",
                    ResultClass="LMI_SystemSoftwareCollection")
            self.assertEqual(len(refs), 1,
                "exactly one collection is associated with package %s" % pkg)
            self.assertCIMNameEqual(refs[0], objpath.Collection)

    @swbase.test_with_repos(**{
        'stable' : True,
        'updates' : False,
        'updates-testing' : True,
        'misc' : False
    })
    def test_get_collection_package_names(self):
        """
        Try to get package names associated to software collection.
        """
        pkg = self.get_repo('stable')['pkg1']
        objpath = self.make_op(pkg)
        collection = objpath.Collection.to_instance()
        self.assertNotEqual(collection, None,
                "GetInstance call succeeds on collection")
        refs = collection.associator_names(
                AssocClass=self.CLASS_NAME,
                Role="Collection",
                ResultRole="Member",
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
        for repo in self.repodb.values():
            if not repo.repoid in (
                    'openlmi-sw-test-repo-stable',
                    'openlmi-sw-test-repo-updates-testing'):
                continue
            for pkg in repo.packages:
                self.assertIn(pkg.nevra, nevra_set)
                nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)

    @swbase.run_for_backends('yum')
    @swbase.test_with_repos(**{
        'stable' : True,
        'updates' : False,
        'updates-testing' : True,
        'misc' : False
    })
    def test_get_collection_packages(self):
        """
        Try to get packages associated to software collection.
        """
        pkg = self.get_repo('stable')['pkg1']
        objpath = self.make_op(pkg)
        collection = objpath.Collection.to_instance()
        self.assertNotEqual(collection, None,
                "GetInstance call succeeds on collection")
        refs = collection.associators(
                AssocClass=self.CLASS_NAME,
                Role="Collection",
                ResultRole="Member",
                ResultClass="LMI_SoftwareIdentity")
        self.assertGreater(len(refs), 0)

        for ref in refs:
            self.assertEqual(ref.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentity")
            self.assertEqual(ref.path.key_properties(), ["InstanceID"])
            self.assertTrue(ref.InstanceID.startswith(
                "LMI:LMI_SoftwareIdentity:"))

        nevra_set = set(i.ElementName for i in refs)
        for repo in self.repodb.values():
            if not repo.repoid in (
                    'openlmi-sw-test-repo-stable',
                    'openlmi-sw-test-repo-updates-testing'):
                continue
            for pkg in repo.packages:
                self.assertIn(pkg.nevra, nevra_set)
                nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestMemberOfSoftwareCollection)

if __name__ == '__main__':
    unittest.main()
