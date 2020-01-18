# -*- encoding: utf-8 -*-
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
# Authors: Roman Rakus <rrakus@redhat.com>
#

from common import AccountBase
from methods import *

class TestGroup(AccountBase):
    """
    Class for testing LMI_Group class
    """
    def test_group_properties(self):
        """
        Account: Test if there are key and main properties in LMI_Group
        """
        slct = "select * from LMI_Group"
        instances = self.wbemconnection.ExecQuery('WQL', slct)
        self.assertTrue(len(instances) > 0)
        # check if it provides key properties
        for attr in ["CreationClassName", "Name"]:
            self.assertIsNotNone(instances[0][attr])

        # check if it provides other properties, which should be set
        for attr in ["ElementName", "InstanceID"]:
            self.assertIsNotNone(instances[0][attr])

    def test_create_group(self):
        """
        Account: Test to create group
        """
        # make sure the group will not exist
        clean_group(self.group_name)
        computer_system = self.wbemconnection.ExecQuery('WQL',
            'select * from %s' % self.system_cs_name)[0]
        lams = self.wbemconnection.ExecQuery('WQL',
            'select * from LMI_AccountManagementService')[0]
        self.wbemconnection.InvokeMethod("CreateGroup", lams.path,
            Name=self.group_name, System=computer_system.path)
        # The group now should be created, check it
        self.assertEqual(field_in_group(self.group_name, 0), self.group_name)
        # now delete that group
        clean_group(self.group_name)

    def test_delete_group(self):
        """
        Account: Test to delete group
        """
        # make sure the group will exist
        create_group(self.group_name)
        i = self.wbemconnection.ExecQuery('WQL',
            'select * from LMI_Group where Name = "%s"' % self.group_name)[0]
        self.wbemconnection.DeleteInstance(i.path)
        # check if it was really deleted
        self.assertIsNone(field_in_group(self.group_name, 0))
        clean_group(self.group_name)

