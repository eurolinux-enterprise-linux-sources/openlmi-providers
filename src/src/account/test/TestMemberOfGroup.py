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
from lmi.shell import LMIInstance
from lmi.shell import LMIInstanceName


class TestMemberOfGroup(common.AccountBase):
    """
    Class for testing LMI_MemberOfGroup class
    """

    def tearDown(self):
        common.UserOps.clean_account(self.user_name)
        common.UserOps.clean_group(self.group_name)

    def test_add_user_to_group(self):
        """
        Account: Test to add user to group
        """
        common.UserOps.create_account(self.user_name)
        common.UserOps.create_group(self.group_name)
        user = self.ns.LMI_Account.first_instance({"Name": self.user_name})
        group = self.ns.LMI_Group.first_instance_name({
            "Name": self.group_name})
        self.assertIsInstance(user, LMIInstance)
        self.assertIsInstance(group, LMIInstanceName)
        identity = user.first_associator_name(ResultClass='LMI_Identity')
        self.assertIsInstance(identity, LMIInstanceName)
        tocreate = self.ns.LMI_MemberOfGroup.create_instance({
            'Collection': group,
            'Member': identity
        })
        self.assertIsInstance(tocreate, LMIInstance)

    def test_remove_user_from_group(self):
        """
        Account: Test remove user from group
        """
        # make sure the account will exist
        common.UserOps.create_account(self.user_name)
        inst = self.ns.LMI_Account.first_instance({"Name": self.user_name})
        self.assertIsInstance(inst, LMIInstance)
        r = inst.delete()
        self.assertTrue(r)
        # check if it was really deleted
        self.assertFalse(common.UserOps.is_user(self.user_name))

    def test_user_in_groups(self):
        """
        Account: Test correct list of groups for user
        """
        common.UserOps.create_account(self.user_name)
        common.UserOps.create_group(self.group_name)
        common.UserOps.add_user_to_group(self.user_name, self.group_name)
        user = self.ns.LMI_Account.first_instance({"Name": self.user_name})
        self.assertIsInstance(user, LMIInstance)
        ident = self.ns.LMI_Identity.first_instance({
            "InstanceID": "LMI:UID:%s" % user.UserID
        })
        self.assertIsInstance(ident, LMIInstance)
        insts = ident.associators(AssocClass='LMI_MemberOfGroup')
        self.assertEqual(len(insts), 2)
        found_user = False
        found_group = False
        for inst in insts:
            if inst.Name == self.user_name:
                found_user = True
            elif inst.Name == self.group_name:
                found_group = True
        self.assertTrue(found_user)
        self.assertTrue(found_group)
