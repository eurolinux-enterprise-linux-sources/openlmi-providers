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
Unit tests for LMI_MemberOfSoftwareCollection provider.
"""

import pywbem
import unittest

import base

class TestMemberOfSoftwareCollection(base.SoftwareBaseTestCase):
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_MemberOfSoftwareCollection"
    KEYS = ("Collection", "Member")

    def make_op(self, pkg, newer=True):
        """
        @return object path of MembeOfSoftwareCollection association
        """
        return self.cim_class.new_instance_name({
            "Collection" : self.ns.LMI_SystemSoftwareCollection. \
                new_instance_name({
                    "InstanceID" : "LMI:LMI_SystemSoftwareCollection"
                }),
            "Member" : self.ns.LMI_SoftwareIdentity.new_instance_name({
                "InstanceID" : 'LMI:LMI_SoftwareIdentity:' \
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
            self.assertNotEqual(inst, None,
                    'Package "%s" not available in SystemSoftwareCollection'
                    % str(pkg))
            self.assertCIMNameEqual(inst.path, objpath,
                    "Object paths should match for package %s"%pkg)
            for key in self.KEYS:
                self.assertIn(key, inst.properties(),
                        "OP is missing \"%s\" key for package %s"%(key, pkg))

    def test_get_member_referents(self):
        """
        Test ReferenceNames for SoftwareIdentity.
        """
        for pkg in self.safe_pkgs:
            objpath = self.make_op(pkg)
            member = objpath.Member.to_instance() 
            self.assertNotEqual(member, None,
                    'Package "%s" not found' % str(pkg))
            refs = member.associator_names(
                    AssocClass=self.CLASS_NAME,
                    Role="Member",
                    ResultRole="Collection",
                    ResultClass="LMI_SystemSoftwareCollection")
            self.assertEqual(len(refs), 1,
                    'Package "%s" not available in SystemSoftwareCollection'
                    % str(pkg))
            ref = refs[0]
            self.assertCIMNameEqual(objpath.Collection, ref)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestMemberOfSoftwareCollection)

if __name__ == '__main__':
    unittest.main()
