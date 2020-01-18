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


class TestAccount(common.AccountBase):
    """
    Class for testing LMI_Account class
    """

    CLASS_NAME = "LMI_Account"

    def tearDown(self):
        common.UserOps.clean_account(self.user_name)

    def test_account_properties(self):
        """
        Account: Test if there are key and main properties in LMI_Account
        """
        inst = self.cim_class.first_instance()
        self.assertIsInstance(inst, LMIInstance)
        # check if it provides key properties
        for attr in ['CreationClassName', 'Name', 'SystemCreationClassName',
                     'SystemName']:
            self.assertIsNotNone(inst.properties_dict()[attr])

        # check if it provides other properties, which should be set
        for attr in ['host', 'UserID', 'UserPassword', 'UserPasswordEncoding']:
            self.assertIsNotNone(inst.properties_dict()[attr])

    def test_create_account(self):
        """
        Account: Test to create user account
        """
        # make sure the account will not exist
        common.UserOps.clean_account(self.user_name)
        lams = self.ns.LMI_AccountManagementService.first_instance()
        self.assertIsInstance(lams, LMIInstance)
        (retval, rparam, errorstr) = lams.CreateAccount({
            'Name': self.user_name,
            'System': self.system_iname
        })
        self.assertEqual(retval, 0)
        # The user now should be created, check it
        self.assertTrue(common.UserOps.is_user(self.user_name))

    def test_delete_account(self):
        """
        Account: Test to delete account
        """
        # make sure the account will exist
        common.UserOps.create_account(self.user_name)
        inst = self.cim_class.first_instance({"Name": self.user_name})
        self.assertIsInstance(inst, LMIInstance)
        r = inst.delete()
        self.assertTrue(r)
        # check if it was really deleted
        self.assertFalse(common.UserOps.is_user(self.user_name))

    def test_modify_account(self):
        """
        Account: Test several modifications
        """
        common.UserOps.create_account(self.user_name)
        inst = self.cim_class.first_instance({"Name": self.user_name})
        self.assertIsInstance(inst, LMIInstance)
        # gecos
        inst.ElementName = "GECOS"
        (retval, rparam, errorstr) = inst.push()
        self.assertEqual(retval, 0)
        self.assertEqual(methods.field_in_passwd(self.user_name, 4),
                         "GECOS")
        # login shell
        inst.LoginShell = "/modified"
        (retval, rparam, errorstr) = inst.push()
        self.assertEqual(retval, 0)
        self.assertEqual(methods.field_in_passwd(self.user_name, 6),
                         "/modified")
        # password
        inst.UserPassword = [
            '$6$9Ky8vI6f$ipRcdc7rgMrtDh.sWOaRSoBck2cLz4eUom8Ez'
            'e.NaY2DoMmNimuFBrXpJjlPCjMoeFTYC.FdZwj488JZcohyw1'
        ]
        (retval, rparam, errorstr) = inst.push()
        self.assertEqual(retval, 0)
        shad = (
            '$6$9Ky8vI6f$ipRcdc7rgMrtDh.sWOaRSoBck2cLz4eUom8Ez'
            'e.NaY2DoMmNimuFBrXpJjlPCjMoeFTYC.FdZwj488JZcohyw1'
        )
        self.assertEqual(methods.field_in_shadow(self.user_name, 1), shad)
