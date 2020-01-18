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
Unit tests for ``LMI_SystemSoftwareCollection`` provider.
"""

import unittest

import swbase

class TestSystemSoftwareCollection(swbase.SwTestCase):
    """
    Basic cim operations test on ``LMI_SystemSoftwareCollection``.
    """

    CLASS_NAME = "LMI_SystemSoftwareCollection"
    KEYS = ("InstanceID", )

    def make_op(self):
        """
        :returns Object path of ``LMI_SystemSoftwareCollection``.
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        return self.cim_class.new_instance_name({
            "InstanceID" : "LMI:LMI_SystemSoftwareCollection"
        })

    def test_get_instance(self):
        """
        Test ``GetInstance()`` call on ``LMI_SystemSoftwareCollection``.
        """
        objpath = self.make_op()
        inst = objpath.to_instance()

        self.assertCIMNameEqual(objpath, inst.path)
        for key in self.KEYS:
            self.assertEqual(getattr(inst, key), getattr(objpath, key))
        self.assertIsInstance(inst.Caption, basestring)

    def test_enum_instance_names(self):
        """
        Test ``EnumInstanceNames()`` call on ``LMI_SystemSoftwareCollection``.
        """
        inames = self.cim_class.instance_names()
        self.assertEqual(len(inames), 1)
        iname = inames[0]
        self.assertCIMNameEqual(iname, self.make_op())

    def test_enum_instances(self):
        """
        Test ``EnumInstances()`` call on ``LMI_SystemSoftwareCollection``.
        """
        insts = self.cim_class.instances()
        self.assertEqual(len(insts), 1)
        inst = insts[0]
        self.assertCIMNameEqual(inst.path, self.make_op())
        self.assertEqual(inst.InstanceID, inst.InstanceID)
        self.assertIsInstance(inst.Caption, basestring)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSystemSoftwareCollection)

if __name__ == '__main__':
    unittest.main()
