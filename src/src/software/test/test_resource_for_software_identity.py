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
Unit tests for LMI_ResourceForSoftwareIdentity provider.
"""

import pywbem
import unittest

from lmi.test.util import mark_dangerous
from lmi.test.util import mark_tedious
import base

class TestResourceForSoftwareIdentity(base.SoftwareBaseTestCase):
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_ResourceForSoftwareIdentity"
    KEYS = ("ManagedElement", "AvailableSAP")

    @classmethod
    def needs_repodb(cls):
        return True

    def make_op(self, pkg=None, newer=True, repo=None):
        """
        @return object path of ResourceForSoftwareIdentity association
        """
        objpath = self.cim_class.new_instance_name({
            "AvailableSAP" : self.ns.LMI_SoftwareIdentityResource. \
                new_instance_name({
                    "CreationClassName" : "LMI_SoftwareIdentityResource",
                    "SystemCreationClassName" : self.system_cs_name,
                    "SystemName" : self.SYSTEM_NAME
                }),
            "ManagedElement" : self.ns.LMI_SoftwareIdentity.new_instance_name({})
            })
        if repo is not None:
            objpath.AvailableSAP.wrapped_object["Name"] = repo.repoid
        elif pkg is not None:
            objpath.AvailableSAP.wrapped_object["Name"] = getattr(pkg,
                    'up_repo' if newer else 'repo')
        if pkg is not None:
            objpath.ManagedElement.wrappe_object["InstanceID"] = (
                      'LMI:LMI_SoftwareIdentity:'
                    + pkg.get_nevra(newer=newer, with_epoch="ALWAYS"))
        return objpath

    @mark_dangerous
    def test_get_instance(self):
        """
        Tests GetInstance call on packages from our rpm cache.
        """
        for pkg in self.dangerous_pkgs:
            objpath = self.make_op(pkg)
            inst = objpath.to_instance()
            self.assertCIMNameEqual(objpath, inst.path,
                    "Object paths should match for package %s"%pkg)
            for key in self.KEYS:
                self.assertIn(key, inst.properties(),
                    'OP is missing \"%s\" key for package "%s".' % (key, pkg))
                self.assertCIMNameEqual(
                        getattr(inst, key), getattr(inst.path, key),
                        'Key property "%s" does not match for package "%s"!' %
                        (key, pkg))

    @mark_tedious
    def test_get_resource_referents(self):
        """
        Test ReferenceNames for AvailableSAP.
        """
        for repo in self.repodb:
            objpath = self.make_op(repo=repo)
            if not repo.pkg_count or repo.pkg_count > 10000:
                # do not test too big repositories
                continue
            refs = objpath.AvailableSAP.to_instance().associator_names(
                    AssocClass=self.CLASS_NAME,
                    Role="AvailableSAP",
                    ResultRole="ManagedElement",
                    ResultClass="LMI_SoftwareIdentity")
            if repo.pkg_count > 0:
                self.assertGreater(len(refs), 0,
                        'repository "%s" is missing software identities'
                        % repo.name)
            for ref in refs:
                self.assertEqual(ref.namespace, 'root/cimv2')
                self.assertEqual(ref.classname, "LMI_SoftwareIdentity")
                self.assertEqual(ref.key_properties(), ["InstanceID"])
                self.assertTrue(
                        ref.InstanceID.startswith("LMI:LMI_SoftwareIdentity:"))

            nevra_set = set(i.InstanceID for i in refs)
            # NOTE: installed packages might not be available
            for pkg, up in ((pkg, up) for pkg in self.dangerous_pkgs
                    for up in (True, False)):
                nevra = 'LMI:LMI_SoftwareIdentity:'+pkg.get_nevra(
                        newer=up, with_epoch="ALWAYS")
                reponame = getattr(pkg, 'up_repo' if up else 'repo')
                if reponame == repo.repoid:
                    self.assertTrue(nevra in nevra_set,
                            'Missing nevra "%s" for repo "%s".' % (nevra,
                                reponame))

    @mark_tedious
    def test_get_resource_referents_for_disabled_repo(self):
        """
        Test ReferenceNames for AvailableSAP, which is disabled.
        """
        for repo in self.repodb:
            if repo.status:
                continue    # test only disabled repositories
            objpath = self.make_op(repo=repo)
            refs = objpath.AvailableSAP.to_instance().associator_names(
                    AssocClass=self.CLASS_NAME,
                    Role="AvailableSAP",
                    ResultRole="ManagedElement",
                    ResultClass="LMI_SoftwareIdentity")
            if repo.pkg_count:
                self.assertGreater(len(refs), 0,
                    'no software identities associated to repo "%s"' % repo.name)
            for ref in refs:
                self.assertEqual(ref.namespace, 'root/cimv2')
                self.assertEqual(ref.classname, "LMI_SoftwareIdentity")
                self.assertEqual(ref.key_properties(), ["InstanceID"])
                self.assertTrue(
                        ref.InstanceID.startswith("LMI:LMI_SoftwareIdentity:"))

    @mark_dangerous
    def test_get_managed_element_referents(self):
        """
        Test ReferenceNames for SoftwareIdentity.
        """
        for pkg, up in ((pkg, up) for pkg in self.dangerous_pkgs
                for up in (True, False)):
            objpath = self.make_op(pkg, newer=up)
            refs = objpath.ManagedElement.to_instance().associator_names(
                    AssocClass=self.CLASS_NAME,
                    Role="ManagedElement",
                    ResultRole="AvailableSAP",
                    ResultClass="LMI_SoftwareIdentityResource")
            self.assertEqual(1, len(refs),
                    'No repo found for pkg "%s".' % pkg.get_nevra(newer=up,
                        with_epoch="ALWAYS"))
            ref = refs[0]
            self.assertEqual(ref.namespace, 'root/cimv2')
            self.assertEqual(ref.classname, "LMI_SoftwareIdentityResource")
            self.assertEqual(sorted(ref.key_properties()),
                    sorted(["SystemCreationClassName", "SystemName",
                        "Name", "CreationClassName"]))
            self.assertEqual(
                    getattr(pkg, 'up_repo' if up else 'repo'),
                    ref.Name, 'Repository name does not match for pkg "%s"'%
                    pkg.get_nevra(newer=up, with_epoch="ALWAYS"))

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestResourceForSoftwareIdentity)

if __name__ == '__main__':
    unittest.main()
