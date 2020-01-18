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
# Authors: Alois Mahdal <amahdal@redhat.com>
#

import common
import lmi.test.util
from lmi.test.ind import IndicationStreamTestCase


class TestIndicationEventStream(IndicationStreamTestCase):
    """
    Test streams of events and indications
    """

    def setUp(self):
        super(TestIndicationEventStream, self).setUp()
        self.driver_class = common.AccountIndicationTestDriver

    def check(self, case):
        """
        Execute case and perform all assertions.
        """
        probe = self.execute(case)
        self.assertExpectedIndStream(probe, case)
        self.assertExpectedSIStream(probe, case)

    def check_numbased(self, variations, items):
        """
        Construct+execute number-based case and perform all assertions.

        This is aimed at specific cases when we only want certain
        number of actions/handlers/subscription and we want to check
        that certain number of indications/instances was delivered.

        Typically this is useful for corner cases with zeroes, e.g.
        when there are zero handlers, zero indications must be
        delivered, no matter number of subscriptions or actions.
        """
        for counts in variations:
            case = self.mknumcase(counts, items)
            probe = self.execute(case)
            self.assertExpectedIndStream(probe, case)
            self.assertExpectedSIStream(probe, case)

    # ............
    # the simplest

    def test_account_create(self):
        case = {
            'subscriptions': 'ac',
            'handlers': 'pr',
            'actions': 'ac',
            'expected_ind_stream': 'ac',
            'expected_si_stream': 'a',
        }
        self.check(case)

    def test_account_delete(self):
        case = {
            'subscriptions': 'ad',
            'handlers': 'pr',
            'actions': 'ac,ad',
            'expected_ind_stream': 'ad',
            'expected_si_stream': 'a',
        }
        self.check(case)

    def test_account_round(self):
        case = {
            'subscriptions': 'ac,ad',
            'handlers': '2pr',
            'actions': 'ac,ad',
            'expected_ind_stream': 'ac,ad',
            'expected_si_stream': '2a',
        }
        self.check(case)

    def test_group_create(self):
        case = {
            'subscriptions': 'gc',
            'handlers': 'pr',
            'actions': 'gc',
            'expected_ind_stream': 'ac',
            'expected_si_stream': 'g',
        }
        self.check(case)

    def test_group_delete(self):
        case = {
            'subscriptions': 'gd',
            'handlers': 'pr',
            'actions': 'gc,gd',
            'expected_ind_stream': 'ad',
            'expected_si_stream': 'g',
        }
        self.check(case)

    def test_group_round(self):
        case = {
            'subscriptions': 'gc,gd',
            'handlers': '2pr',
            'actions': 'gc,gd',
            'expected_ind_stream': 'ac,ad',
            'expected_si_stream': '2g',
        }
        self.check(case)

    # ................
    # longer sequences

    @lmi.test.util.mark_tedious
    def test_account_create_50(self):
        case = {
            'subscriptions': 'ac',
            'handlers': 'pr',
            'actions': '50ac,50ad',
            'expected_ind_stream': '50ac',
            'expected_si_stream': '50a',
        }
        self.driver_options['delay_chillout'] = 20
        self.check(case)

    @lmi.test.util.mark_tedious
    def test_account_delete_50(self):
        case = {
            'subscriptions': 'ad',
            'handlers': 'pr',
            'actions': '50ac,50ad',
            'expected_ind_stream': '50ad',
            'expected_si_stream': '50a',
        }
        self.driver_options['delay_chillout'] = 20
        self.check(case)

    @lmi.test.util.mark_tedious
    def test_account_round_50(self):
        case = {
            'subscriptions': 'ac,ad',
            'handlers': '2pr',
            'actions': '50ac,50ad',
            'expected_ind_stream': '50ac,50ad',
            'expected_si_stream': '100a',
        }
        self.driver_options['delay_chillout'] = 20
        self.check(case)

    @lmi.test.util.mark_tedious
    def test_group_create_50(self):
        case = {
            'subscriptions': 'gc',
            'handlers': 'pr',
            'actions': '50gc,50gd',
            'expected_ind_stream': '50ac',
            'expected_si_stream': '50g',
        }
        self.driver_options['delay_chillout'] = 20
        self.check(case)

    @lmi.test.util.mark_tedious
    def test_group_delete_50(self):
        case = {
            'subscriptions': 'gd',
            'handlers': 'pr',
            'actions': '50gc,50gd',
            'expected_ind_stream': '50ad',
            'expected_si_stream': '50g',
        }
        self.driver_options['delay_chillout'] = 20
        self.check(case)

    @lmi.test.util.mark_tedious
    def test_group_round_50(self):
        case = {
            'subscriptions': 'gc,gd',
            'handlers': '2pr',
            'actions': '50gc,50gd',
            'expected_ind_stream': '50ac,50ad',
            'expected_si_stream': '100g',
        }
        self.driver_options['delay_chillout'] = 20
        self.check(case)

    # .................................
    # uneven handlers vs. subscriptions

    def test_account_numbased(self):
        items = "ac", "ac", "a"     # action, indication, source instance
        variations = [
            # (handlers, subs, acts, expect)
            #                        ^ count of indications, assumed
            #                          equal to count of source instances
            (0, 0, 0, 0),
            (0, 0, 1, 0),
            (0, 0, 2, 0),
            (0, 1, 0, 0),
            (0, 1, 1, 0),
            (0, 1, 2, 0),
            (0, 2, 0, 0),
            (0, 2, 1, 0),
            (0, 2, 2, 0),
            (1, 0, 0, 0),
            (1, 0, 1, 0),
            (1, 0, 2, 0),
            (1, 1, 0, 0),
            (1, 1, 1, 1),
            (1, 1, 2, 2),
            (1, 2, 0, 0),
            (1, 2, 1, 1),
            (1, 2, 2, 2),
            (2, 0, 0, 0),
            (2, 0, 1, 0),
            (2, 0, 2, 0),
            (2, 1, 0, 0),
            (2, 1, 1, 1),
            (2, 1, 2, 2),
            (2, 2, 0, 0),
            (2, 2, 1, 2),
            (2, 2, 2, 4),
        ]
        self.check_numbased(variations, items)

    def test_group_numbased(self):
        items = "gc", "ac", "g"     # action, indication, source instance
        variations = [
            # (handlers, subs, acts, expect)
            #                        ^ count of indications, assumed
            #                          equal to count of source instances
            (0, 0, 0, 0),
            (0, 0, 1, 0),
            (0, 0, 2, 0),
            (0, 1, 0, 0),
            (0, 1, 1, 0),
            (0, 1, 2, 0),
            (0, 2, 0, 0),
            (0, 2, 1, 0),
            (0, 2, 2, 0),
            (1, 0, 0, 0),
            (1, 0, 1, 0),
            (1, 0, 2, 0),
            (1, 1, 0, 0),
            (1, 1, 1, 1),
            (1, 1, 2, 2),
            (1, 2, 0, 0),
            (1, 2, 1, 1),
            (1, 2, 2, 2),
            (2, 0, 0, 0),
            (2, 0, 1, 0),
            (2, 0, 2, 0),
            (2, 1, 0, 0),
            (2, 1, 1, 1),
            (2, 1, 2, 2),
            (2, 2, 0, 0),
            (2, 2, 1, 2),
            (2, 2, 2, 4),
        ]
        self.check_numbased(variations, items)
