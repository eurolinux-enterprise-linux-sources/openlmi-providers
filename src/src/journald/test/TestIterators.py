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
from lmi.test import wbem

CIM_MESSAGELOG_ITERATOR_RESULT_SUCCESS = 0
CIM_MESSAGELOG_ITERATOR_RESULT_NOT_SUPPORTED = 1
CIM_MESSAGELOG_ITERATOR_RESULT_FAILED = 2

class TestIterators(JournalBase):
    """
    Class for testing LMI_JournalMessageLog iterators
    """

    CLASS_NAME = "LMI_JournalMessageLog"

    def test_simple_iteration(self):
        """
        Journal: Test simple iteration and iterator ID stability
        """
        inst = self.cim_class.first_instance()
        self.assertIsInstance(inst, LMIInstance)
        (retval, rfirst, errorstr) = inst.PositionToFirstRecord()
        self.assertEqual(retval, 0)
        self.assertIsNotNone(rfirst)
        self.assertIn('IterationIdentifier', rfirst)
        self.assertIsNotNone(rfirst['IterationIdentifier'])
        (retval, rget, errorstr) = inst.GetRecord(PositionToNext = False,
                                                  IterationIdentifier = rfirst['IterationIdentifier'])
        self.assertEqual(retval, 0)
        self.assertEqual(rfirst['IterationIdentifier'], rget['IterationIdentifier'])
        (retval, rget, errorstr) = inst.GetRecord(PositionToNext = True,
                                                  IterationIdentifier = rget['IterationIdentifier'])
        self.assertEqual(retval, 0)
        self.assertNotEqual(rfirst['IterationIdentifier'], rget['IterationIdentifier'])
        (retval, rclose, errorstr) = inst.CancelIteration(IterationIdentifier = rget['IterationIdentifier'])
        self.assertEqual(retval, 0)


    def test_invalid_cancellation(self):
        """
        Journal: Test invalid IDs cancellation
        """
        inst = self.cim_class.first_instance()
        self.assertIsInstance(inst, LMIInstance)
        # Invalid iterator format
        iter_id = "sdfsdfsdgfa"
        self.assertRaisesCIM(wbem.CIM_ERR_INVALID_PARAMETER,
                             inst.CancelIteration,
                             IterationIdentifier = iter_id)
        # Correct format, inactive iterator
        iter_id = "LMI_JournalMessageLog_CMPI_Iter_9878972#0x7f8504021380#" \
                  "s=a13c10bf472048e69dceeee118cd7f84;i=3;b=5b96431d996a416ebf11dbb6543a5084;" \
                  "m=221f7b;t=4dcc1b6e70a7a;x=b1b572ade816b92e"
        (retval, rclose, errorstr) = inst.CancelIteration(IterationIdentifier = iter_id)
        self.assertEqual(retval, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED)


    def test_double_cancellation(self):
        """
        Journal: Test double iterator cancellation
        """
        inst = self.cim_class.first_instance()
        self.assertIsInstance(inst, LMIInstance)
        (retval, rfirst, errorstr) = inst.PositionToFirstRecord()
        self.assertEqual(retval, 0)
        self.assertIsNotNone(rfirst)
        (retval, _, errorstr) = inst.CancelIteration(IterationIdentifier = rfirst['IterationIdentifier'])
        self.assertEqual(retval, 0)
        (retval, _, errorstr) = inst.CancelIteration(IterationIdentifier = rfirst['IterationIdentifier'])
        self.assertEqual(retval, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED)


    def test_seeking(self):
        """
        Journal: Test seeking back and forth
        """
        inst = self.cim_class.first_instance()
        self.assertIsInstance(inst, LMIInstance)
        (retval, rfirst, errorstr) = inst.PositionToFirstRecord()
        self.assertEqual(retval, 0)
        self.assertIsNotNone(rfirst)
        (retval, rget1, errorstr) = inst.GetRecord(PositionToNext = True,
                                                   IterationIdentifier = rfirst['IterationIdentifier'])
        self.assertEqual(retval, 0)
        # Seeking to absolute record index should not be supported
        self.assertRaisesCIM(wbem.CIM_ERR_INVALID_PARAMETER,
                             inst.PositionAtRecord,
                             MoveAbsolute = True,
                             IterationIdentifier = rget1['IterationIdentifier'],
                             RecordNumber = 10)
        (retval, rseek, errorstr) = inst.PositionAtRecord(MoveAbsolute = False,
                                                          IterationIdentifier = rget1['IterationIdentifier'],
                                                          RecordNumber = 5)
        self.assertEqual(retval, 0)
        (retval, rseek, errorstr) = inst.PositionAtRecord(MoveAbsolute = False,
                                                          IterationIdentifier = rseek['IterationIdentifier'],
                                                          RecordNumber = 4)
        self.assertEqual(retval, 0)
        (retval, rseek, errorstr) = inst.PositionAtRecord(MoveAbsolute = False,
                                                          IterationIdentifier = rseek['IterationIdentifier'],
                                                          RecordNumber = -10)
        self.assertEqual(retval, 0)
        self.assertEqual(rfirst['IterationIdentifier'], rseek['IterationIdentifier'])
        (retval, rget2, errorstr) = inst.GetRecord(PositionToNext = False,
                                                   IterationIdentifier = rseek['IterationIdentifier'])
        self.assertEqual(retval, 0)
        # We should be at the starting position now
        self.assertEqual(rfirst['IterationIdentifier'], rget2['IterationIdentifier'])
        self.assertEqual(rget1['RecordData'], rget2['RecordData'])
        (retval, rclose, errorstr) = inst.CancelIteration(IterationIdentifier = rget2['IterationIdentifier'])
        self.assertEqual(retval, 0)


    def test_stale_iter(self):
        """
        Journal: Test stale iterators
        """
        # Note this test is closely tied to iterator format, keep in sync with the provider code
        inst = self.cim_class.first_instance()
        self.assertIsInstance(inst, LMIInstance)
        (retval, rfirst, errorstr) = inst.PositionToFirstRecord()
        self.assertEqual(retval, 0)
        self.assertIsNotNone(rfirst)
        iter_pointer = "0x7f8504021380"
        s1 = rfirst['IterationIdentifier'].split('#')
        iter_id = s1[0] + "#" + iter_pointer + "#" + s1[2]
        (retval, rget1, errorstr) = inst.GetRecord(PositionToNext = False,
                                                   IterationIdentifier = iter_id)
        self.assertEqual(retval, 0)
        self.assertNotEqual(rget1['IterationIdentifier'], iter_id)
        s2 = rget1['IterationIdentifier'].split('#')
        self.assertNotEqual(s1[0], s2[0])
        self.assertNotEqual(s1[1], s2[1])
        self.assertNotEqual(s2[1], iter_pointer)
        self.assertEqual(s1[2], s2[2])
        (retval, rget2, errorstr) = inst.GetRecord(PositionToNext = False,
                                                   IterationIdentifier = rget1['IterationIdentifier'])
        self.assertEqual(retval, 0)
        self.assertEqual(rget1['IterationIdentifier'], rget2['IterationIdentifier'])
        (retval, rget3, errorstr) = inst.GetRecord(PositionToNext = True,
                                                   IterationIdentifier = rget2['IterationIdentifier'])
        self.assertEqual(retval, 0)
        self.assertNotEqual(rget2['IterationIdentifier'], rget3['IterationIdentifier'])
        (retval, _, errorstr) = inst.CancelIteration(IterationIdentifier = rget2['IterationIdentifier'])
        self.assertEqual(retval, 0)
        (retval, _, errorstr) = inst.CancelIteration(IterationIdentifier = rfirst['IterationIdentifier'])
        self.assertEqual(retval, 0)

if __name__ == '__main__':
    unittest.main()
