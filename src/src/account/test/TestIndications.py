# Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
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
import time
from methods import *

class TestIndications(AccountBase):
    """
    Class for testing LMI_Account indications
    """
    NEEDS_INDICATIONS = True

    def test_check_good_filter(self):
        """
        Account: Test good indication filter
        """
        filter_name = "test_good_filter_%d" % time.time() * 1000
        sub = self.subscribe(filter_name, "select * from LMI_AccountInstanceCreationIndication where SourceInstance isa LMI_Account")
        self.assertIsNotNone(sub)
        self.unsubscribe(filter_name);

    def test_check_bad_filter(self):
        """
        Account: Test bad indication filter
        """
        pass

    def test_group_deletion_indication(self):
        """
        Account: Test indication of group deletion
        """
        create_group(self.group_name)
        filter_name = "test_delete_group_%d" % time.time() * 1000
        sub = self.subscribe(filter_name, "select * from LMI_AccountInstanceDeletionIndication where SourceInstance isa LMI_Group")
        clean_group(self.group_name)
        indication = self.get_indication(10)
        self.assertEqual(indication.classname, "LMI_AccountInstanceDeletionIndication")
        self.assertIn("SourceInstance", indication.keys())
        self.assertTrue(indication["SourceInstance"] is not None)
        self.assertEqual(indication["SourceInstance"]["Name"], self.group_name)

    def test_group_creation_indication(self):
        """
        Account: Test indication of group creation
        """
        clean_group(self.group_name)
        filter_name = "test_create_group_%d" % time.time() * 1000
        sub = self.subscribe(filter_name, "select * from LMI_AccountInstanceCreationIndication where SourceInstance isa LMI_Group")
        create_group(self.group_name)
        indication = self.get_indication(10)
        self.assertEqual(indication.classname, "LMI_AccountInstanceCreationIndication")
        self.assertIn("SourceInstance", indication.keys())
        self.assertTrue(indication["SourceInstance"] is not None)
        self.assertEqual(indication["SourceInstance"]["Name"], self.group_name)
        clean_group(self.group_name)

    def test_account_deletion_indication(self):
        """
        Account: Test indication of account deletion
        """
        create_account(self.user_name)
        filter_name = "test_delete_account_%d" % time.time() * 1000
        sub = self.subscribe(filter_name, "select * from LMI_AccountInstanceDeletionIndication where SourceInstance isa LMI_Account")
        clean_account(self.user_name)
        indication = self.get_indication(10)
        self.assertEqual(indication.classname, "LMI_AccountInstanceDeletionIndication")
        self.assertIn("SourceInstance", indication.keys())
        self.assertTrue(indication["SourceInstance"] is not None)
        self.assertEqual(indication["SourceInstance"]["Name"], self.user_name)

    def test_account_creation_indication(self):
        """
        Account: Test indication of account creation
        """
        clean_account(self.user_name)
        filter_name = "test_create_account_%d" % time.time() * 1000
        sub = self.subscribe(filter_name, "select * from LMI_AccountInstanceCreationIndication where SourceInstance isa LMI_Account")
        create_account(self.user_name)
        indication = self.get_indication(10)
        self.assertEqual(indication.classname, "LMI_AccountInstanceCreationIndication")
        self.assertIn("SourceInstance", indication.keys())
        self.assertTrue(indication["SourceInstance"] is not None)
        self.assertEqual(indication["SourceInstance"]["Name"], self.user_name)
        clean_account(self.user_name)

