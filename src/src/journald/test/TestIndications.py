# Copyright (C) 2013-2014 Red Hat, Inc.  All rights reserved.
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
import os
import time
import syslog
from lmi.test import unittest
from lmi.test import wbem

class TestIndications(JournalBase):
    """
    Class for testing LMI_JournalMessageLog indications
    """

    NEEDS_INDICATIONS = True

    def test_check_good_filter_simple(self):
        """
        Journal: Test good simple indication filter
        """
        filter_name = "test_good_simple_filter_%d" % (time.time() * 1000)
        sub = self.subscribe(filter_name, "select * from LMI_JournalLogRecordInstanceCreationIndication where SourceInstance isa LMI_JournalLogRecord")
        self.assertIsNotNone(sub)
        self.unsubscribe(sub);


    def test_check_good_filter_complex(self):
        """
        Journal: Test good complex indication filter
        """
        filter_name = "test_good_complex_filter_%d" % (time.time() * 1000)
        sub = self.subscribe(filter_name, "select * from LMI_JournalLogRecordInstanceCreationIndication where SourceInstance isa LMI_JournalLogRecord and SourceInstance.LMI_JournalLogRecord::LogName = 'Journal'")
        self.assertIsNotNone(sub)
        self.unsubscribe(sub);


    def test_check_bad_filter(self):
        """
        Journal: Test bad indication filter
        """
        filter_name = "test_bad_filter_%d" % (time.time() * 1000)
        self.assertRaisesCIM(wbem.CIM_ERR_NOT_SUPPORTED,
                             self.subscribe,
                             filter_name, "select * from LMI_JournalLogRecordInstanceCreationIndication where SourceInstance.LMI_JournalLogRecord::LogName = 'Journal'")


    def test_message_send(self):
        """
        Journal: Test message logging and its retrieval from journal
        """
        process_name = "nosetests"
        filter_name = "test_message_send_%d" % (time.time() * 1000)
        syslog_msg = "== LMI_Journald indication test message mark %d ==" % (time.time() * 1000)
        data_format = "{0}[{1}]: {2}".format(process_name, os.getpid(), syslog_msg)
        sub = self.subscribe(filter_name, "select * from LMI_JournalLogRecordInstanceCreationIndication where SourceInstance isa LMI_JournalLogRecord and SourceInstance.LMI_JournalLogRecord::DataFormat = '%s'" % data_format)
        syslog.syslog(syslog.LOG_DEBUG, syslog_msg)
        indication = self.get_indication(10)
        self.assertEqual(indication.classname, "LMI_JournalLogRecordInstanceCreationIndication")
        self.assertIn("SourceInstance", indication.keys())
        self.assertTrue(indication["SourceInstance"] is not None)
        self.assertIn(syslog_msg, indication["SourceInstance"]["DataFormat"])
        self.assertEqual(os.getpid(), indication["SourceInstance"]["ProcessID"])
        self.assertEqual(os.geteuid(), indication["SourceInstance"]["UserID"])
        self.assertEqual(os.getegid(), indication["SourceInstance"]["GroupID"])
        self.assertEqual(syslog.LOG_DEBUG, indication["SourceInstance"]["SyslogSeverity"])
        self.assertEqual(2, indication["SourceInstance"]["PerceivedSeverity"])
        self.assertEqual(1, indication["SourceInstance"]["SyslogFacility"])
        self.assertEqual(process_name, indication["SourceInstance"]["SyslogIdentifier"])
        self.unsubscribe(sub);

if __name__ == '__main__':
    unittest.main()
