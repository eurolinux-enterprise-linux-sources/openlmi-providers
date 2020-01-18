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
import pywbem

class TestMemberOfGroup(AccountBase):
    """
    Class for testing LMI_MemberOfGroup class
    """
    def test_add_user_to_group(self):
        """
        Account: Test to add user to group
        """
        create_account(self.user_name)
        create_group(self.group_name)
        user = self.wbemconnection.ExecQuery('WQL',
            'select * from LMI_Account where Name = "%s"' %self.user_name)[0]
        group = self.wbemconnection.ExecQuery('WQL',
            'select * from LMI_Group where Name = "%s"' %self.group_name)[0]
        tocreate = pywbem.CIMInstance('LMI_MemberOfGroup',
            {'Collection' : group.path,
            'Member' : user.path})
        self.wbemconnection.CreateInstance(tocreate)
        clean_account(self.user_name)
        clean_group(self.group_name)

    def test_remove_user_from_group(self):
        """
        Account: Test remove user from group
        """
        # make sure the account will exist
        create_account(self.user_name)
        i = self.wbemconnection.ExecQuery('WQL',
            'select * from LMI_Account where Name = "%s"' % self.user_name)[0]
        self.wbemconnection.DeleteInstance(i.path)
        # check if it was really deleted
        self.assertFalse(user_exists(self.user_name))
        clean_account(self.user_name)

    def test_user_in_groups(self):
        """
        Account: Test correct list of groups for user
        """
        create_account(self.user_name)
        create_group(self.group_name)
        add_user_to_group(self.user_name, self.group_name)
        user = self.wbemconnection.ExecQuery('WQL',
            'select * from LMI_Account where Name = "%s"' %self.user_name)[0]
        ident = self.wbemconnection.ExecQuery('WQL',
            'select * from LMI_Identity where InstanceID = "LMI:UID:%s"' % user["UserID"])[0]
        insts = self.wbemconnection.Associators(ident.path, AssocClass = 'LMI_MemberOfGroup')
        self.assertEqual(len(insts), 2)
        found_user = False
        found_group = False
        for inst in insts:
            if inst["Name"] == self.user_name:
                found_user = True
            elif inst["Name"] == self.group_name:
                found_group = True
        self.assertTrue(found_user)
        self.assertTrue(found_group)
        clean_account(self.user_name)
        clean_group(self.group_name)
