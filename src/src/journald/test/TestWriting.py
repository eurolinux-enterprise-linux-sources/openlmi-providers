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
from lmi.shell import LMIInstance
from lmi.test import unittest
import time


class TestWriting(JournalBase):
    """
    Class for testing LMI_JournalLogRecord record writing
    """

    CLASS_NAME = "LMI_JournalLogRecord"

    def test_create_instance(self):
        """
        Journal: Test new record writeout using CreateInstance()
        """
        new_msg = "== LMI_Journald test message for CreateInstance() mark %d ==" % (time.time() * 1000)
        inst = self.cim_class.create_instance(
            { "CreationClassName": "LMI_JournalLogRecord",
              "LogCreationClassName": "LMI_JournalMessageLog",
              "LogName": "Journal",
              "DataFormat": new_msg })
        self.assertIsInstance(inst, LMIInstance)
        self.assertIsNotNone(inst.RecordID)
        self.assertIsNotNone(inst.MessageTimestamp)
        self.assertIn(new_msg, inst.DataFormat)

if __name__ == '__main__':
    unittest.main()
