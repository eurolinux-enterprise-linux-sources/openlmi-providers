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
Unit tests for ``LMI_ResourceForSoftwareIdentity`` provider.
"""
from lmi.test import unittest
from util import USE_PKCON

import package
import swbase

ENABLED_STATE_ENABLED = 2

class TestResourceForSoftwareIdentity(swbase.SwTestCase):
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_ResourceForSoftwareIdentity"
    KEYS = ("ManagedElement", "AvailableSAP")

    def make_op(self, pkg=None, repo=None):
        """
        :returns: Object path of ``LMI_ResourceForSoftwareIdentity``
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        objpath = self.cim_class.new_instance_name({
            "AvailableSAP" : self.ns.LMI_SoftwareIdentityResource. \
                new_instance_name({
                    "CreationClassName" : "LMI_SoftwareIdentityResource",
                    "SystemCreationClassName" : self.system_cs_name,
                    "SystemName" : self.SYSTEM_NAME
                }),
            "ManagedElement" :
                self.ns.LMI_SoftwareIdentity.new_instance_name({})
            })
        if repo is not None:
            objpath.AvailableSAP.wrapped_object["Name"] = repo.repoid
        elif pkg is not None:
            objpath.AvailableSAP.wrapped_object["Name"] = pkg.repoid
        if pkg is not None:
            objpath.ManagedElement.wrapped_object["InstanceID"] = (
                      'LMI:LMI_SoftwareIdentity:' + pkg.nevra)
        return objpath

    @swbase.test_with_repos('stable', 'updates', 'updates-testing', 'misc')
    def test_get_instance(self):
        """
        Test ``GetInstance()`` call on ``LMI_ResourceForSoftwareIdentity``.
        """
        for repo in self.repodb.values():
            self.assertTrue(repo.status)
            for pkg in repo.packages:
                objpath = self.make_op(pkg, repo)
                inst = objpath.to_instance()
                self.assertNotEqual(inst, None)
                self.assertEqual(set(inst.path.key_properties()),
                        set(self.KEYS))
                for key in self.KEYS:
                    self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key),
                        'Key property "%s" does not match for package "%s#%s"!'
                        % (key, repo.repoid, pkg))

    @swbase.run_for_backends('yum')
    @swbase.test_with_repos(**{'updates' : False})
    def test_get_instance_disabled_repo(self):
        """
        Test ``GetInstance()`` call on ``LMI_ResourceForSoftwareIdentity``
        with disabled repository.
        """
        repo = self.get_repo('updates')
        self.assertFalse(repo.status)
        for pkg in repo.packages:
            objpath = self.make_op(pkg, repo)
            inst = objpath.to_instance()
            self.assertNotEqual(inst, None)
            self.assertEqual(set(inst.path.key_properties()),
                    set(self.KEYS))
            for key in self.KEYS:
                self.assertCIMNameEqual(
                    getattr(inst, key), getattr(inst.path, key),
                    'Key property "%s" does not match for package "%s#%s"!' %
                    (key, repo.repoid, pkg))

    @swbase.test_with_repos('stable')
    def test_repo_identity_names(self):
        """
        Test ``AssociatorNames()`` call on ``LMI_ResourceForSoftwareIdentity``.
        """
        repo = self.get_repo('stable')
        self.assertTrue(repo.status)
        if not USE_PKCON:
            self.assertGreater(repo.pkg_count, 0)

        objpath = self.make_op(repo=repo)
        refs = objpath.AvailableSAP.to_instance().associator_names(
                AssocClass=self.CLASS_NAME,
                Role="AvailableSAP",
                ResultRole="ManagedElement",
                ResultClass="LMI_SoftwareIdentity")
        if USE_PKCON:
            self.assertGreater(len(refs), 0,
                    'repository "%s" is missing software identities'
                    % repo.name)
        else:
            self.assertEqual(len(refs), repo.pkg_count,
                    'repository "%s" is missing software identities'
                    % repo.name)
        for ref in refs:
            self.assertEqual(ref.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentity")
            self.assertEqual(ref.key_properties(), ["InstanceID"])
            self.assertTrue(
                    ref.InstanceID.startswith("LMI:LMI_SoftwareIdentity:"))

        nevra_set = set(i.InstanceID for i in refs)
        for pkg in repo.packages:
            nevra = 'LMI:LMI_SoftwareIdentity:'+pkg.nevra
            self.assertTrue(nevra in nevra_set,
                    'Missing nevra "%s" for repo "%s".' % (nevra,
                        repo.repoid))
            nevra_set.remove(nevra)
        self.assertEqual(len(nevra_set), 0,
                "all packages from repository have been listed")

    @swbase.run_for_backends('yum')
    @swbase.test_with_repos(stable=False)
    def test_disabled_repo_identity_names(self):
        """
        Test ``AssociatorNames()`` call on ``LMI_ResourceForSoftwareIdentity``.
        """
        repo = self.get_repo('stable')
        self.assertFalse(repo.status)
        self.assertGreater(repo.pkg_count, 0)

        objpath = self.make_op(repo=repo)
        refs = objpath.AvailableSAP.to_instance().associator_names(
                AssocClass=self.CLASS_NAME,
                Role="AvailableSAP",
                ResultRole="ManagedElement",
                ResultClass="LMI_SoftwareIdentity")
        self.assertEqual(len(refs), repo.pkg_count,
                'repository "%s" is missing software identities'
                % repo.name)
        for ref in refs:
            self.assertEqual(ref.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentity")
            self.assertEqual(ref.key_properties(), ["InstanceID"])
            self.assertTrue(
                    ref.InstanceID.startswith("LMI:LMI_SoftwareIdentity:"))

    @swbase.run_for_backends('yum')
    @swbase.test_with_repos('updates')
    @swbase.test_with_packages(**{
            'updates#pkg1' : True,
            'updates#pkg2' : True,
            'updates#pkg3' : False,
            'updates#pkg4' : False
    })
    def test_repo_identities(self):
        """
        Test ``Associators()`` call on ``LMI_ResourceForSoftwareIdentity``.
        """
        repo = self.get_repo('updates')
        self.assertTrue(repo.status)
        self.assertGreater(repo.pkg_count, 0)

        objpath = self.make_op(repo=repo)
        refs = objpath.AvailableSAP.to_instance().associators(
                AssocClass=self.CLASS_NAME,
                Role="AvailableSAP",
                ResultRole="ManagedElement",
                ResultClass="LMI_SoftwareIdentity")
        self.assertGreater(len(refs), 0,
            'no software identities associated to repo "%s"'
                % repo.repoid)
        for ref in refs:
            self.assertEqual(ref.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentity")
            self.assertEqual(ref.path.key_properties(), ["InstanceID"])

        nevra_dict = dict((i.ElementName, i) for i in refs)
        installed_count = 0
        for pkg in repo.packages:
            self.assertTrue(pkg.nevra in nevra_dict,
                    'Missing package "%s" in repo "%s".' % (pkg, repo.repoid))
            if package.is_pkg_installed(pkg):
                self.assertNotEqual(nevra_dict[pkg.nevra].InstallDate, None,
                        "InstallDate property is set for installed and"
                        " available package %s" % pkg)
                installed_count += 1
            else:
                self.assertEqual(nevra_dict[pkg.nevra].InstallDate, None,
                        "InstallDate property is unset for available package"
                        " %s" % pkg)
            del nevra_dict[pkg.nevra]
        self.assertEqual(len(nevra_dict), 0,
                "all packages from repository have been listed")
        self.assertEqual(installed_count, 2)

    @swbase.test_with_repos('stable')
    def test_get_pkg_repository_name(self):
        """
        Try to get software identities associated with repository.
        """
        repo = self.get_repo('stable')
        self.assertTrue(repo.status)
        for pkg in repo.packages:
            objpath = self.make_op(pkg, repo)
            inst = objpath.ManagedElement.to_instance()
            self.assertNotEqual(inst, None,
                    "GetInstance() succeeds for package %s" % pkg)
            refs = inst.associator_names(
                    AssocClass=self.CLASS_NAME,
                    Role="ManagedElement",
                    ResultRole="AvailableSAP",
                    ResultClass="LMI_SoftwareIdentityResource")
            self.assertEqual(1, len(refs),
                    'No repo found for pkg "%s".' % pkg.nevra)
            ref = refs[0]
            self.assertEqual(ref.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentityResource")
            self.assertEqual(set(ref.key_properties()),
                    set(("SystemCreationClassName", "SystemName",
                        "Name", "CreationClassName",)))
            self.assertEqual(ref.Name, pkg.repoid,
                    'Repository name does not match for pkg "%s"'
                    % pkg.nevra)

    @swbase.test_with_repos('updates')
    @swbase.test_with_packages(**{
        'updates#pkg1' : True,
        'updates#pkg2' : True,
        'updates#pkg3' : False,
        'updates#pkg4' : False
    })
    def test_get_pkg_repository(self):
        """
        Try to get repository for installed and not installed package.
        """
        repo = self.get_repo('updates')
        self.assertTrue(repo.status)
        for pkg in repo.packages:
            objpath = self.make_op(pkg, repo)
            inst = objpath.ManagedElement
            self.assertNotEqual(inst, None,
                    "GetInstance() succeeds for package %s" % pkg)
            refs = inst.to_instance().associators(
                    AssocClass=self.CLASS_NAME,
                    Role="ManagedElement",
                    ResultRole="AvailableSAP",
                    ResultClass="LMI_SoftwareIdentityResource")
            self.assertEqual(1, len(refs),
                    'No repo found for pkg "%s".' % pkg.nevra)
            ref = refs[0]
            self.assertEqual(ref.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentityResource")
            self.assertEqual(set(ref.path.key_properties()),
                    set(("SystemCreationClassName", "SystemName",
                        "Name", "CreationClassName",)))
            self.assertEqual(ref.Name, pkg.repoid,
                    'Repository name does not match for pkg "%s"'
                    % pkg.nevra)
            self.assertEqual(ref.ElementName, pkg.repoid)
            self.assertEqual(ref.EnabledState, ENABLED_STATE_ENABLED)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestResourceForSoftwareIdentity)

if __name__ == '__main__':
    unittest.main()
