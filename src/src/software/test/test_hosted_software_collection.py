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
Unit tests for ``LMI_MemberOfSoftwareCollection`` provider.
"""

import pywbem
import unittest

import swbase

class TestHostedSoftwareCollection(swbase.SwTestCase):
    """
    Basic cim operations test on ``LMI_HostedSoftwareCollection``.
    """

    CLASS_NAME = "LMI_HostedSoftwareCollection"
    KEYS = ("Antecedent", "Dependent")

    def make_op(self):
        """
        :returns Object path of ``LMI_HostedSoftwareCollection``.
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        return self.cim_class.new_instance_name({
            "Antecedent" : self.system_iname,
            "Dependent" : self.ns.LMI_SystemSoftwareCollection. \
                new_instance_name({
                    "InstanceID" : "LMI:LMI_SystemSoftwareCollection"
                })
            })

    def test_get_instance(self):
        """
        Test ``GetInstance()`` call on ``LMI_HostedSoftwareCollection``.
        """
        objpath = self.make_op()
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)
        self.assertCIMNameEqual(inst.path, objpath)
        self.assertEqual(sorted(inst.properties()), sorted(self.KEYS))
        self.assertCIMNameEqual(objpath, inst.path)
        for key in self.KEYS:
            self.assertCIMNameEqual(getattr(inst, key), getattr(inst.path, key))

        # try with CIM_ prefix
        objpath.Antecedent.wrapped_object.classname = "CIM_ComputerSystem"

        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)

        # try with CIM_ prefix also for CreationClassName
        objpath.Antecedent.wrapped_object["CreationClassName"] = \
                "CIM_ComputerSystem"

        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)

    def test_enum_instances(self):
        """
        Test ``EnumInstances()`` call on ``LMI_HostedSoftwareCollection``.
        """
        objpath = self.make_op()
        insts = self.cim_class.instances()
        self.assertEqual(1, len(insts))
        self.assertCIMNameEqual(objpath, insts[0].path)
        self.assertEqual(sorted(insts[0].properties()), sorted(self.KEYS))
        for key in self.KEYS:
            self.assertCIMNameEqual(
                    getattr(insts[0], key), getattr(insts[0].path, key))

    def test_enum_instance_names(self):
        """
        Test ``EnumInstanceNames`` call on ``LMI_HostedSoftwareCollection``.
        """
        objpath = self.make_op()
        inames = self.cim_class.instance_names()
        self.assertEqual(1, len(inames))
        self.assertCIMNameEqual(objpath, inames[0])

    def test_get_computer_system_collections(self):
        """
        Try to get collection associated to computer system.
        """
        objpath = self.make_op()
        refs = objpath.Antecedent.to_instance().associator_names(
                AssocClass=self.CLASS_NAME,
                Role="Antecedent",
                ResultRole="Dependent",
                ResultClass='LMI_SystemSoftwareCollection')
        self.assertEqual(len(refs), 1)
        ref = refs[0]
        self.assertCIMNameEqual(objpath.Dependent, ref)

    def test_get_collection_computer_systems(self):
        """
        Try to get computer system associated to software collection.
        """
        objpath = self.make_op()
        refs = objpath.Dependent.to_instance().associator_names(
                AssocClass=self.CLASS_NAME,
                Role="Dependent",
                ResultRole="Antecedent",
                ResultClass=self.system_cs_name)
        self.assertEqual(1, len(refs))
        ref = refs[0]
        self.assertCIMNameEqual(objpath.Antecedent, ref)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestHostedSoftwareCollection)

if __name__ == '__main__':
    unittest.main()
