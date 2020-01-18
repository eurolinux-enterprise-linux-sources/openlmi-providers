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
import pywbem
import unittest

class TestWriting(JournalBase):
    """
    Class for testing LMI_JournalLogRecord record writing
    """

    def test_create_instance(self):
        """
        Journal: Test new record writeout using CreateInstance()
        """
        new_msg = "== LMI_Journald test message for CreateInstance() mark %d ==" % (time.time() * 1000)
        inst = pywbem.CIMInstance('LMI_JournalLogRecord',
                                  properties = { "CreationClassName": "LMI_JournalLogRecord",
                                                 "LogCreationClassName": "LMI_JournalMessageLog",
                                                 "LogName": "Journal",
                                                 "DataFormat": new_msg })
        iname = self.wbemconnection.CreateInstance(NewInstance = inst)
        new_inst = self.wbemconnection.GetInstance(iname)
        self.assertIsNotNone(iname.keybindings['RecordID'])
        self.assertIsNotNone(iname.keybindings['MessageTimestamp'])
        self.assertIn(new_msg, new_inst['DataFormat'])

if __name__ == '__main__':
    unittest.main()
