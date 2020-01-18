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
Unit tests for LMI_HostedSoftwareIdentityResource provider.
"""

import pywbem
import unittest

import base

class TestHostedSoftwareIdentityResource(base.SoftwareBaseTestCase):
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_HostedSoftwareIdentityResource"
    KEYS = ("Antecedent", "Dependent")

    @classmethod
    def needs_pkgdb(cls):
        return False

    @classmethod
    def needs_repodb(cls):
        return True

    def make_op(self, repo):
        """
        @return object path of HostedSoftwareIdentityResource association
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

    def test_get_instance(self):
        """
        Tests GetInstance call on repositories from our rpm cache.
        """
        for repo in self.repodb:
            objpath = self.make_op(repo)
            inst = objpath.to_instance()
            self.assertCIMNameEqual(objpath, inst.path,
                    "Object paths should match for repo %s" % repo.repoid)
            for key in self.KEYS:
                self.assertIn(key, inst.properties(),
                    'OP is missing \"%s\" key for repo "%s".' %
                    (key, repo.repoid))
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key),
                        'Key property "%s" does not match for repo "%s"!' %
                        (key, repo.repoid))

    def test_enum_instance_names(self):
        """
        Tests EnumInstanceNames call on repositories from our cache.
        """
        inames = self.cim_class.instance_names()
        repos = { r.repoid: r for r in self.repodb }
        self.assertEqual(len(repos), len(inames))
        for iname in inames:
            self.assertIn(iname.Dependent.Name, repos)
            objpath = self.make_op(repos[iname.Dependent.Name])
            self.assertCIMNameEqual(objpath, iname)

    def test_enum_instances(self):
        """
        Tests EnumInstanceNames call on repositories from our cache.
        """
        inames = self.cim_class.instances()
        repos = { r.repoid: r for r in self.repodb }
        self.assertEqual(len(repos), len(inames))
        for inst in inames:
            repoid = inst.Dependent.Name
            self.assertIn(repoid, repos)
            objpath = self.make_op(repos[repoid])
            self.assertCIMNameEqual(objpath, inst.path)
            for key in self.KEYS:
                self.assertIn(key, inst.properties(),
                    'OP is missing \"%s\" key for repo "%s".' % (key, repoid))
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key),
                        'Key property "%s" does not match for repo "%s"!' %
                        (key, repoid))

    def test_get_antecedent_referents(self):
        """
        Test ReferenceNames for ComputerSystem.
        """
        if not self.repodb:
            return
        repo = self.repodb[0]
        objpath = self.make_op(repo)
        refs = objpath.Antecedent.to_instance().associator_names(
                AssocClass=self.CLASS_NAME,
                Role="Antecedent",
                ResultRole="Dependent",
                ResultClass="LMI_SoftwareIdentityResource")
        repos = {r.repoid: r for r in self.repodb}
        for ref in refs:
            self.assertEqual(ref.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentityResource")
            objpath = self.make_op(repos[ref.Name])
            self.assertCIMNameEqual(objpath.Dependent, ref)
            del repos[ref.Name]
        self.assertEqual(0, len(repos))

    def test_get_dependent_referents(self):
        """
        Test ReferenceNames for repository.
        """
        for repo in self.repodb:
            objpath = self.make_op(repo=repo)
            refs = objpath.Dependent.to_instance().associator_names(
                    AssocClass=self.CLASS_NAME,
                    Role="Dependent",
                    ResultRole="Antecedent",
                    ResultClass=self.system_cs_name)
            self.assertEqual(len(refs), 1)
            self.assertCIMNameEqual(objpath.Antecedent, refs[0])

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestHostedSoftwareIdentityResource)

if __name__ == '__main__':
    unittest.main()
