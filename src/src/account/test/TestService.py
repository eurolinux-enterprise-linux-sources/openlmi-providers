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
from methods import field_in_group
from methods import field_in_passwd
from methods import field_in_shadow
from lmi.shell import LMIInstance


class TestService(common.AccountBase):
    """
    Class for testing LMI_AccountManagementService
    """

    CLASS_NAME = "LMI_AccountManagementService"

    def test_create_account(self):
        """
        Account: Test create account parameters
        """
        # make sure the account will not exist
        common.UserOps.clean_account(self.user_name)
        lams = self.cim_class.first_instance()
        self.assertIsInstance(lams, LMIInstance)

        # create account and test all parameters
        shell = "testshell"
        system_account = True
        dont_create_home = True
        dont_create_group = True
        gid = 0
        gecos = "test create account gecos"
        home_dir = "/test/home"
        password = ('$6$9Ky8vI6f$ipRcdc7rgMrtDh.sWOaRSoBck2cLz4eUom8Ez'
                    'e.NaY2DoMmNimuFBrXpJjlPCjMoeFTYC.FdZwj488JZcohyw1')
        uid = 777
        (rc, out, errorstr) = lams.CreateAccount({
            'Name': self.user_name,
            'System': self.system_iname,
            'Shell': shell,
            'SystemAccount': system_account,
            'DontCreateHome': dont_create_home,
            'DontCreateGroup': dont_create_group,
            'GID': gid,
            'GECOS': gecos,
            'HomeDirectory': home_dir,
            'Password': password,
            'UID': uid})
        acc = out["Account"]
        idents = out["Identities"]

        # check return values
        # account
        self.assertEqual(rc, 0)
        self.assertEqual(acc.Name, self.user_name)
        # identities
        for identity in idents:
            if identity.InstanceID.find("UID") != -1:
                # user identity
                self.assertEqual(identity.InstanceID, "LMI:UID:%d" % uid)
            else:
                # group identity
                self.assertEqual(identity.InstanceID, "LMI:GID:%d" % gid)

        # check with system info
        self.assertEqual(field_in_passwd(self.user_name, 2), str(uid))
        self.assertEqual(field_in_passwd(self.user_name, 3), str(gid))
        self.assertEqual(field_in_passwd(self.user_name, 4), gecos)
        self.assertEqual(field_in_passwd(self.user_name, 5), home_dir)
        self.assertEqual(field_in_passwd(self.user_name, 6), shell)
        self.assertEqual(field_in_shadow(self.user_name, 1), password)
        common.UserOps.clean_account(self.user_name)

    def test_create_group(self):
        """
        Account: Test create group parameters
        """
        common.UserOps.clean_group(self.group_name)
        lams = self.cim_class.first_instance()
        self.assertIsInstance(lams, LMIInstance)
        system_account = True
        gid = 666
        (rc, out, errorstr) = lams.CreateGroup({
            'Name': self.group_name,
            'System': self.system_iname,
            'SystemAccount': system_account,
            'GID': gid})

        group = out["Group"]
        idents = out["Identities"]
        self.assertEqual(rc, 0)
        self.assertEqual(group.Name, self.group_name)
        for identity in idents:
            self.assertEqual(identity.InstanceID, "LMI:GID:%d" % gid)

        self.assertEqual(field_in_group(self.group_name, 2), str(gid))
        common.UserOps.clean_group(self.group_name)
