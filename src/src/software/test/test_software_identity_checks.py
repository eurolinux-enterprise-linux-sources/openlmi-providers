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
Unit tests for LMI_SoftwareIdentityChecks provider.
"""

import pywbem
import unittest

from lmi.test.lmibase import enable_lmi_exceptions
import base
import util

class TestSoftwareIdentityChecks(base.SoftwareBaseTestCase):
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_SoftwareIdentityChecks"
    KEYS = ("Check", "Element")

    @classmethod
    def needs_pkgdb_files(cls):
        return True

    def make_op(self, pkg, filepath, newer=True):
        """
        @return object path of SoftwareIdentityChecks association
        """
        return self.cim_class.new_instance_name({
            "Check" : self.ns.LMI_SoftwareIdentityFileCheck.new_instance_name({
                "CheckID" : 'LMI:LMI_SoftwareIdentityFileCheck',
                "Name" : filepath,
                "SoftwareElementID" : pkg.get_nevra(
                        newer=newer, with_epoch="ALWAYS"),
                "SoftwareElementState" : pywbem.Uint16(2),     #Executable
                "TargetOperatingSystem" : util.get_target_operating_system(),
                "Version" : getattr(pkg, 'up_evra' if newer else 'evra')
                }),
            "Element" : self.ns.LMI_SoftwareIdentity.new_instance_name({
                "InstanceID" : 'LMI:LMI_SoftwareIdentity:'
                        + pkg.get_nevra(newer=newer, with_epoch="ALWAYS")
                })
            })

    @unittest.skip("unrealiable test on random packages")
    def test_get_instance(self):
        """
        Tests GetInstance call on packages from our rpm cache.
        """
        for pkg in self.safe_pkgs:
            for filepath in self.pkgdb_files[pkg.name]:
                objpath = self.make_op(pkg, filepath)
                inst = objpath.to_instance()
                self.assertCIMNameEqual(inst.path, objpath,
                        "Object paths should match for package %s"%pkg)
                for key in self.KEYS:
                    self.assertIn(key, inst.properties(),
                        "OP is missing \"%s\" key for package %s"%(key, pkg))
                    if key == "SoftwareElementID":
                        setattr(inst, key, getattr(inst, key)[:-1])
                    self.assertCIMNameEqual(
                            getattr(inst, key), getattr(inst.path, key))
                self.assertCIMNameEqual(objpath, inst.path,
                        "Object paths should match for package %s"%pkg)

    @enable_lmi_exceptions
    def test_enum_instance_names(self):
        """
        Tests EnumInstances call on identity checks - should not be supported.
        """
        self.assertRaisesCIM(pywbem.CIM_ERR_NOT_SUPPORTED,
                self.cim_class.instance_names)

    def test_get_identity_referents(self):
        """
        Test AssociatorNames for Element(SoftwareIdentity).
        """
        for pkg in self.safe_pkgs:
            files = self.pkgdb_files[pkg.name]
            if not files:
                # package may not have any file
                continue
            objpath = self.make_op(pkg, files[0])
            refs = objpath.Element.to_instance().associator_names(
                    AssocClass=self.CLASS_NAME,
                    Role="Element",
                    ResultRole="Check",
                    ResultClass="LMI_SoftwareIdentityFileCheck")
            for ref in refs:
                self.assertEqual(ref.namespace, 'root/cimv2')
                self.assertEqual(ref.classname,
                    "LMI_SoftwareIdentityFileCheck")
                self.assertIn(ref.Name, set(r.Name for r in refs))
                self.assertEqual(sorted(ref.key_properties()),
                    ["CheckID", "Name", "SoftwareElementID",
                        "SoftwareElementState",
                        "TargetOperatingSystem", "Version"])
                self.assertEqual(
                    pkg.get_nevra(newer=True, with_epoch='ALWAYS'),
                    ref.SoftwareElementID)

    @unittest.skip("unrealiable test on random packages")
    def test_get_check_referents(self):
        """
        Test AssociatorNames for Check(SoftwareIdentityFileCheck).
        """
        for pkg in self.safe_pkgs:
            files = self.pkgdb_files[pkg.name]
            for filepath in files:
                objpath = self.make_op(pkg, filepath)
                refs = objpath.Check.to_instance().associator_names(
                        AssocClass=self.CLASS_NAME,
                        Role="Check",
                        ResultRole="Element",
                        ResultClass="LMI_SoftwareIdentity")
                self.assertEqual(len(refs), 1)
                ref = refs[0]
                self.assertCIMNameEqual(objpath.Element, ref)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareIdentityChecks)

if __name__ == '__main__':
    unittest.main()
