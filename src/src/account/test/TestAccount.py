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
import subprocess

class TestAccount(AccountBase):
    """
    Class for testing LMI_Account class
    """
    def test_account_properties(self):
        """
        Account: Test if there are key and main properties in LMI_Account
        """
        slct = "select * from LMI_Account"
        instances = self.wbemconnection.ExecQuery('WQL', slct)
        self.assertTrue(len(instances) > 0)
        # check if it provides key properties
        for attr in ["CreationClassName", "Name", "SystemCreationClassName",
                     "SystemName"]:
            self.assertIsNotNone(instances[0][attr])

        # check if it provides other properties, which should be set
        for attr in ["host", "UserID", "UserPassword", "UserPasswordEncoding"]:
            self.assertIsNotNone(instances[0][attr])

    def test_create_account(self):
        """
        Account: Test to create user account
        """
        # make sure the account will not exist
        clean_account(self.user_name)
        computer_system = self.wbemconnection.ExecQuery('WQL',
            'select * from %s' % self.system_cs_name)[0]
        lams = self.wbemconnection.ExecQuery('WQL',
            'select * from LMI_AccountManagementService')[0]
        self.wbemconnection.InvokeMethod("CreateAccount", lams.path,
            Name=self.user_name, System=computer_system.path)
        # The user now should be created, check it
        self.assertTrue(user_exists(self.user_name))
        # now delete that user
        clean_account(self.user_name)

    def test_delete_account(self):
        """
        Account: Test to delete account
        """
        # make sure the account will exist
        create_account(self.user_name)
        i = self.wbemconnection.ExecQuery('WQL',
            'select * from LMI_Account where Name = "%s"' % self.user_name)[0]
        self.wbemconnection.DeleteInstance(i.path)
        # check if it was really deleted
        self.assertFalse(user_exists(self.user_name))
        clean_account(self.user_name)

    def test_modify_account(self):
        """
        Account: Test several modifications
        """
        create_account(self.user_name)
        i = self.wbemconnection.ExecQuery('WQL',
            'select * from LMI_Account where Name = "%s"' % self.user_name)[0]
        # gecos
        i["ElementName"] = "GECOS"
        self.wbemconnection.ModifyInstance(i)
        self.assertEqual(field_in_passwd(self.user_name, 4), "GECOS")
        # login shell
        i["LoginShell"] = "/modified"
        self.wbemconnection.ModifyInstance(i)
        self.assertEqual(field_in_passwd(self.user_name, 6), "/modified")
        # password
        i["UserPassword"][0] = '$6$9Ky8vI6f$ipRcdc7rgMrtDh.sWOaRSoBck2cLz4eUom8Eze.NaY2DoMmNimuFBrXpJjlPCjMoeFTYC.FdZwj488JZcohyw1'
        self.wbemconnection.ModifyInstance(i)
        self.assertEqual(field_in_shadow(self.user_name, 1),
            '$6$9Ky8vI6f$ipRcdc7rgMrtDh.sWOaRSoBck2cLz4eUom8Eze.NaY2DoMmNimuFBrXpJjlPCjMoeFTYC.FdZwj488JZcohyw1')
        clean_account(self.user_name)

