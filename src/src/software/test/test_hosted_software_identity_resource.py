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
Unit tests for ``LMI_HostedSoftwareIdentityResource`` provider.
"""

import unittest

import swbase

ENABLED_STATE_DISABLED = 3
ENABLED_STATE_ENABLED = 2

class TestHostedSoftwareIdentityResource(swbase.SwTestCase):
    """
    Basic cim operations test on ``LMI_HostedSoftwareIdentityResource.``
    """

    CLASS_NAME = "LMI_HostedSoftwareIdentityResource"
    KEYS = ("Antecedent", "Dependent")

    def make_op(self, repo):
        """
        :returns Object path of ``LMI_HostedSoftwareIdentityResource``.
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        return self.cim_class.new_instance_name({
            "Antecedent" : self.system_iname,
            "Dependent" : self.ns.LMI_SoftwareIdentityResource. \
                new_instance_name({
                    "CreationClassName"       : "LMI_SoftwareIdentityResource",
                    "SystemCreationClassName" : self.system_cs_name,
                    "SystemName"              : self.SYSTEM_NAME,
                    "Name"                    : repo.repoid
                })
            })

    @swbase.test_with_repos(**{
        'stable'          : True,
        'updates'         : True,
        'updates-testing' : False,
        'misc'            : False
    })
    def test_get_instance(self):
        """
        Test ``GetInstance()`` call on ``LMI_HostedSoftwareIdentityResource``.
        """
        for repo in self.repodb.values():
            objpath = self.make_op(repo)
            inst = objpath.to_instance()
            self.assertNotEqual(inst, None,
                    "GetInstance succeeds for repo %s" % repo.repoid)
            self.assertCIMNameEqual(objpath, inst.path,
                    "Object paths should match for repo %s" % repo.repoid)
            self.assertEqual(set(inst.path.key_properties()), set(self.KEYS))
            for key in self.KEYS:
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key),
                        'Key property "%s" does not match for repo "%s"!' %
                        (key, repo.repoid))

    def test_enum_instance_names(self):
        """
        Test ``EnumInstanceNames()`` call on
        ``LMI_HostedSoftwareIdentityResource``.
        """
        inames = self.cim_class.instance_names()
        repos = set(r for r in self.repodb)
        repos.update([r for r in self.other_repos])
        self.assertEqual(len(inames), len(repos))
        for iname in inames:
            self.assertIn(iname.Dependent.Name, repos)
            repoid = iname.Dependent.Name
            if repoid in self.repodb:
                repo = self.repodb[repoid]
            else:
                repo = self.other_repos[repoid]
            objpath = self.make_op(repo)
            self.assertCIMNameEqual(iname, objpath)
            self.assertEqual(set(iname.key_properties()), set(self.KEYS))
            repos.remove(iname.Dependent.Name)
        self.assertEqual(len(repos), 0)

    def test_enum_instances(self):
        """
        Test ``EnumInstanceNames()`` call on
        ``LMI_HostedSoftwareIdentityResource``.
        """
        insts = self.cim_class.instances()
        repos = set(r for r in self.repodb)
        repos.update([r for r in self.other_repos])
        self.assertEqual(len(insts), len(repos))
        for inst in insts:
            repoid = inst.Dependent.Name
            self.assertIn(repoid, repos)
            if repoid in self.repodb:
                repo = self.repodb[repoid]
            else:
                repo = self.other_repos[repoid]
            objpath = self.make_op(repo)
            self.assertCIMNameEqual(inst.path, objpath)
            self.assertEqual(set(inst.path.key_properties()), set(self.KEYS))
            for key in self.KEYS:
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key),
                        'Key property "%s" does not match for repo "%s"!' %
                        (key, repoid))
            repos.remove(repoid)
        self.assertEqual(len(repos), 0)

    @swbase.test_with_repos(**{
        'stable' : True,
        'updates' : True,
        'updates-testing' : False,
        'misc' : False
    })
    def test_get_computer_system_repositories(self):
        """
        Try to get repositories associated with computer system.
        """
        objpath = self.make_op(self.get_repo('stable'))
        refs = objpath.Antecedent.to_instance().associators(
                AssocClass=self.CLASS_NAME,
                Role="Antecedent",
                ResultRole="Dependent",
                ResultClass="LMI_SoftwareIdentityResource")
        repos = set(r for r in self.repodb)
        repos.update(self.other_repos.keys())
        self.assertEqual(len(refs), len(repos))
        for ref in refs:
            self.assertEqual(ref.path.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentityResource")
            repoid = ref.Name
            if repoid in self.repodb:
                repo = self.repodb[repoid]
            else:
                repo = self.other_repos[repoid]
            objpath = self.make_op(repo)
            self.assertCIMNameEqual(ref.path, objpath.Dependent)
            self.assertEqual(set(ref.path.key_properties()),
                    set(["CreationClassName", "Name",
                          "SystemCreationClassName", "SystemName"]))
            if repo.status:
                self.assertEqual(ref.EnabledState, ENABLED_STATE_ENABLED,
                        "EnabledState does not match for repo %s" % repo.repoid)
            else:
                self.assertEqual(ref.EnabledState, ENABLED_STATE_DISABLED,
                        "EnabledState does not match for repo %s" % repo.repoid)
            repos.remove(ref.Name)
        self.assertEqual(len(repos), 0)

    @swbase.test_with_repos(**{
        'stable' : True,
        'updates' : True,
        'updates-testing' : False,
        'misc' : False
    })
    def test_get_computer_system_repository_names(self):
        """
        Try to get repository names associated with computer system.
        """
        objpath = self.make_op(self.get_repo('stable'))
        refs = objpath.Antecedent.to_instance().associator_names(
                AssocClass=self.CLASS_NAME,
                Role="Antecedent",
                ResultRole="Dependent",
                ResultClass="LMI_SoftwareIdentityResource")
        repos = set(r for r in self.repodb)
        repos.update(self.other_repos.keys())
        self.assertEqual(len(refs), len(repos))
        for ref in refs:
            repoid = ref.Name
            if repoid in self.repodb:
                repo = self.repodb[repoid]
            else:
                repo = self.other_repos[repoid]
            objpath = self.make_op(repo)
            self.assertCIMNameEqual(objpath.Dependent, ref)
            self.assertEqual(set(ref.key_properties()),
                    set(["CreationClassName", "Name",
                          "SystemCreationClassName", "SystemName"]))
            repos.remove(ref.Name)
        self.assertEqual(len(repos), 0)

    @swbase.test_with_repos(**{
        'stable' : True,
        'updates' : True,
        'updates-testing' : False,
        'misc' : False
    })
    def test_get_repository_computer_systems(self):
        """
        Try to get computer system associated with repository.
        """
        for repo in self.repodb.values():
            objpath = self.make_op(repo=repo)
            refs = objpath.Dependent.to_instance().associators(
                    AssocClass=self.CLASS_NAME,
                    Role="Dependent",
                    ResultRole="Antecedent",
                    ResultClass=self.system_cs_name)
            self.assertEqual(len(refs), 1)
            self.assertCIMNameEqual(refs[0].path, objpath.Antecedent)

    @swbase.test_with_repos(**{
        'stable' : True,
        'updates' : True,
        'updates-testing' : False,
        'misc' : False
    })
    def test_get_repository_computer_system_names(self):
        """
        Try to get computer system names associated with repository.
        """
        for repo in self.repodb.values():
            objpath = self.make_op(repo=repo)
            refs = objpath.Dependent.to_instance().associator_names(
                    AssocClass=self.CLASS_NAME,
                    Role="Dependent",
                    ResultRole="Antecedent",
                    ResultClass=self.system_cs_name)
            self.assertEqual(len(refs), 1)
            self.assertCIMNameEqual(refs[0], objpath.Antecedent)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestHostedSoftwareIdentityResource)

if __name__ == '__main__':
    unittest.main()
