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
# Authors: Tomas Bzatek <tbzatek@redhat.com>
#

from journald_common import JournalBase
import time
import syslog
import unittest

class TestIndications(JournalBase):
    """
    Class for testing LMI_JournalMessageLog indications
    """

    NEEDS_INDICATIONS = True

    def test_check_good_filter(self):
        """
        Journal: Test good indication filter
        """
        filter_name = "test_good_filter_%d" % (time.time() * 1000)
        sub = self.subscribe(filter_name, "select * from LMI_JournalLogRecordInstanceCreationIndication where SourceInstance isa LMI_JournalLogRecord")
        self.assertIsNotNone(sub)
        self.unsubscribe(filter_name);


    def test_message_send(self):
        """
        Journal: Test message logging and its retrieval from journal
        """
        filter_name = "test_message_send_%d" % (time.time() * 1000)
        syslog_msg = "== LMI_Journald test message =="
        sub = self.subscribe(filter_name, "select * from LMI_JournalLogRecordInstanceCreationIndication where SourceInstance isa LMI_JournalLogRecord")
        syslog.syslog(syslog_msg)
        indication = self.get_indication(10)
        self.assertEqual(indication.classname, "LMI_JournalLogRecordInstanceCreationIndication")
        self.assertIn("SourceInstance", indication.keys())
        self.assertTrue(indication["SourceInstance"] is not None)
        self.assertIn(syslog_msg, indication["SourceInstance"]["DataFormat"])
        self.unsubscribe(filter_name);

if __name__ == '__main__':
    unittest.main()
