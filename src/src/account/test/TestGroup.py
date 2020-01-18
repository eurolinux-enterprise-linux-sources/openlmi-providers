# -*- encoding: utf-8 -*-
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
#
# Authors: Roman Rakus <rrakus@redhat.com>
#

import common
import methods
from lmi.shell import LMIInstance


class TestGroup(common.AccountBase):
    """
    Class for testing LMI_Group class
    """

    CLASS_NAME = "LMI_Group"

    def tearDown(self):
        common.UserOps.clean_group(self.group_name)

    def test_group_properties(self):
        """
        Account: Test if there are key and main properties in LMI_Group
        """
        inst = self.cim_class.first_instance()
        self.assertIsInstance(inst, LMIInstance)
        # check if it provides key properties
        for attr in ['CreationClassName', 'Name']:
            self.assertIsNotNone(inst.properties_dict()[attr])

        # check if it provides other properties, which should be set
        for attr in ['ElementName', 'InstanceID']:
            self.assertIsNotNone(inst.properties_dict()[attr])

    def test_create_group(self):
        """
        Account: Test to create group
        """
        # make sure the group will not exist
        common.UserOps.clean_group(self.group_name)
        lams = self.ns.LMI_AccountManagementService.first_instance()
        self.assertIsInstance(lams, LMIInstance)
        (retval, rparam, errorstr) = lams.CreateGroup({
            'Name': self.group_name,
            'System': self.system_iname
        })
        self.assertEqual(retval, 0)
        # The group now should be created, check it
        self.assertEqual(methods.field_in_group(self.group_name, 0),
                         self.group_name)

    def test_delete_group(self):
        """
        Account: Test to delete group
        """
        # make sure the group will exist
        common.UserOps.create_group(self.group_name)
        inst = self.cim_class.first_instance({"Name": self.group_name})
        self.assertIsInstance(inst, LMIInstance)
        r = inst.delete()
        self.assertTrue(r)
        # check if it was really deleted
        self.assertIsNone(methods.field_in_group(self.group_name, 0))
