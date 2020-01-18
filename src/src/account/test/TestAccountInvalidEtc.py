# -*- encoding: utf-8 -*-
# Copyright(C) 2012-2013 Red Hat, Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or(at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
# Authors: Alois Mahdal <amahdal@redhat.com>

import sys

import lmi.test.util
from lmi.test.util import mark_dangerous
from common import AccountBase
from lmi.shell import LMIInstance


# ......................................................................... #
# Helper functions
# ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''' #

def gen_cripple(*args):
    """
    Generator of test methods. Each call to this function adds
    one test method to TestAccountInvalidEtc.

    All arguments are passed to PasswdCrippler.cripple.
    """

    def _new_cripple_test(self):
        self.crippler.cripple(*args)
        self.assertSane()

    meth_name = "test%s_%s_%s" % (path.replace("/", "_"), op, case)
    _new_cripple_test.__name__ = meth_name
    setattr(TestAccountInvalidEtc, meth_name, _new_cripple_test)


# ......................................................................... #
# Test data (later "converted" to test methods)
# ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''' #

class PasswdCrippler(lmi.test.util.BaseCrippler):
    """
    Cripple files related to user management
    """

    def _define_cases(self):

        data = {
            'ep': '$6$S2M8XGNtfRj/7mnX$Sn8XPyzOFYKSqSYLRlII4wYQc2A'
                  '.R6gQxh19Oz9e6KHLcCsvLK7ePJWLbZInWktI8/ZaiT/fFA'
                  'Tjx6c3Q1A.Y1',
            'o15': 32769,               # over 15-bit uint
            'o32': 4294967297,          # over 32-bit uint
            'mi': sys.maxint,
            'omi': sys.maxint + 1,
        }

        cases = {}
        cases['/etc/passwd'] = {

            # syntax traps
            'empty_fields': "::::::",
            'one_field': "user1\n",
            'less_fields': "user1:x:2000:2000:U1:/home/user1",
            'more_fields': "user1:x:2000:2000:U1:/home/user1:/bin/bash:hm",

            # funny
            'pegasus': "pegasus:x:66:65"
                       ":tog-pegasus OpenPegasus WBEM/CIM services"
                       ":/var/lib/Pegasus:/sbin/nologin",
            'root': "root:x:0:0:root:/root:/bin/bash",

            # field: name
            'name_empty': ":x:2000:2000:U1:/home/user1:/bin/bash",
            'name_null': "\x00:x:2000:2000:U1:/home/user1:/bin/bash",
            'name_tab': "\x09:x:2000:2000:U1:/home/user1:/bin/bash",
            'name_space': " :x:2000:2000:U1:/home/user1:/bin/bash",
            'name_semicol': ";:x:2000:2000:U1:/home/user1:/bin/bash",
            'name_comma': ",:x:2000:2000:U1:/home/user1:/bin/bash",
            'name_duplicate': "user1:x:2000:2000:U1:/home/user1:/bin/bash\n"
                              "user1:x:2001:2001:User 2:/home/user2:/bin/bash",

            # field: password
            'password_empty': "user1::2000:2000:U1:/home/user1:/bin/bash",
            'password_null': "user1:\x00:2000:2000:U1:/home/user1:/bin/bash",
            'password_tab': "user1:\x09:2000:2000:U1:/home/user1:/bin/bash",
            'password_space': "user1: :2000:2000:U1:/home/user1:/bin/bash",
            'password_semicol': "user1:;:2000:2000:U1:/home/user1:/bin/bash",
            'password_comma': "user1:,:2000:2000:U1:/home/user1:/bin/bash",
            'password_2aster': "user1:**:2000:2000:U1:/home/user1:/bin/bash",
            'password_2x': "user1:xx:2000:2000:U1:/home/user1:/bin/bash",
            'password_asterx': "user1:*x:2000:2000:U1:/home/user1:/bin/bash",
            'password_xaster': "user1:x*:2000:2000:U1:/home/user1:/bin/bash",

            # field: uid
            'uid_empty': "user1:x::2000:U1:/home/user1:/bin/bash",
            'uid_null': "user1:x:\x00:2000:U1:/home/user1:/bin/bash",
            'uid_tab': "user1:x:\x09:2000:U1:/home/user1:/bin/bash",
            'uid_space': "user1:x: :2000:U1:/home/user1:/bin/bash",
            'uid_semicol': "user1:x:;:2000:U1:/home/user1:/bin/bash",
            'uid_comma': "user1:x:,:2000:U1:/home/user1:/bin/bash",
            'uid_string': "user1:x:hey:2000:U1:/home/user1:/bin/bash",
            'uid_negative': "user1:x:-2000:2000:U1:/home/user1:/bin/bash",
            'uid_over15': "user1:x:%(o15)d:2000:U1:/home/user1:/bin/bash",
            'uid_over32': "user1:x:%(o32)d:2000:U1:/home/user1:/bin/bash",
            'uid_over_maxint': "user1:x:%(omi)d:2000:U1:/home/user1:/bin/sh",
            'uid_maxint': "user1:x:%(mi)d:2000:U1:/home/user1:/bin/bash",
            'uid_duplicate': "user1:x:2000:2000:U1:/home/user1:/bin/bash\n"
                             "user2:x:2000:2001:User 2:/home/user2:/bin/bash",

            # field: gid
            'gid_empty': "user1:x:2000::U1:/home/user1:/bin/bash",
            'gid_null': "user1:x:2000:\x00:U1:/home/user1:/bin/bash",
            'gid_tab': "user1:x:2000:\x09:U1:/home/user1:/bin/bash",
            'gid_space': "user1:x:2000: :U1:/home/user1:/bin/bash",
            'gid_semicol': "user1:x:2000:;:U1:/home/user1:/bin/bash",
            'gid_comma': "user1:x:2000:,:U1:/home/user1:/bin/bash",
            'gid_string': "user1:x:2000:hey:U1:/home/user1:/bin/bash",
            'gid_negative': "user1:x:2000:-2000:U1:/home/user1:/bin/bash",
            'gid_over15': "user1:x:2000:%(o15)d:U1:/home/user1:/bin/bash",
            'gid_over32': "user1:x:2000:%(o32)d:U1:/home/user1:/bin/bash",
            'gid_over_maxint': "user1:x:2000:%(omi)d:U1:/home/user1:/bin/sh",
            'gid_maxint': "user1:x:2000:%(mi)d:U1:/home/user1:/bin/bash",
            'gid_duplicate': "user1:x:2000:2000:U1:/home/user1:/bin/bash\n"
                             "user2:x:2001:2000:User 2:/home/user2:/bin/bash",

            # field: directory
            'directory_empty': "user1:x:2000:2000:U1::/bin/bash",
            'directory_null': "user1:x:2000:2000:U1:\x00:/bin/bash",
            'directory_tab': "user1:x:2000:2000:U1:\x09:/bin/bash",

        }

        cases['/etc/group'] = {

            # a valid line as a template
            'valid': "group1:x:2000:user1,user2,user3",

            # syntax traps
            'empty_fields': ":::",
            'one_field': "group1\n",
            'less_fields': "group1:x:user1,user2,user3",
            'more_fields': "group1:x:2000:user1,user2,user3:hey",

            # funny
            'pegasus': "pegasus:x:65:",
            'root': "root:x:0:",

            # field: name
            'name_empty': ":x:2000:user1,user2,user3",
            'name_null': "\x00:x:2000:user1,user2,user3",
            'name_tab': "\x09:x:2000:user1,user2,user3",
            'name_space': " :x:2000:user1,user2,user3",
            'name_semicol': ";:x:2000:user1,user2,user3",
            'name_comma': ",:x:2000:user1,user2,user3",
            'name_duplicate': "group1:x:2000:user1,user2,user3\n"
                              "group1:x:2001:user1,user2,user3",

            # field: password
            'password_empty': "group1::2000:user1,user2,user3",
            'password_null': "group1:\x00:2000:user1,user2,user3",
            'password_tab': "group1:\x09:2000:user1,user2,user3",
            'password_space': "group1: :2000:user1,user2,user3",
            'password_semicol': "group1:;:2000:user1,user2,user3",
            'password_comma': "group1:,:2000:user1,user2,user3",
            'password_2aster': "group1:**:2000:user1,user2,user3",
            'password_2x': "group1:xx:2000:user1,user2,user3",
            'password_asterx': "group1:*x:2000:user1,user2,user3",
            'password_xaster': "group1:x*:2000:user1,user2,user3",

            # field: gid
            'gid_empty': "group1:x::user1,user2,user3",
            'gid_null': "group1:x:\x00:user1,user2,user3",
            'gid_tab': "group1:x:\x09:user1,user2,user3",
            'gid_space': "group1:x: :user1,user2,user3",
            'gid_semicol': "group1:x:;:user1,user2,user3",
            'gid_comma': "group1:x:,:user1,user2,user3",
            'gid_string': "group1:x:hey:user1,user2,user3",
            'gid_negative': "group1:x:-2000:user1,user2,user3",
            'gid_over15': "group1:x:%(o15)d:user1,user2,user3",
            'gid_over32': "group1:x:%(o32)d:user1,user2,user3",
            'gid_over_maxint': "group1:x:%(omi)d:user1.user2,user3",
            'gid_maxint': "group1:x:%(mi)d:user1,user2,user3",
            'gid_duplicate': "group1:x:2000:user1,user2,user3"
                             "user2:x:2000:user1,user2,user3",

            # field: ulist
            'ulist_null': "group1:x:2000:\x00",
            'ulist_tab': "group1:x:2000:\x09",
            'ulist_space': "group1:x:2000: ",
            'ulist_semicol': "group1:x:2000:;",
            'ulist_nulls': "group1:x:2000:\x00,\x00",
            'ulist_tabs': "group1:x:2000:\x09,\x09",
            'ulist_spaces': "group1:x:2000: , ",
            'ulist_semicols': "group1:x:2000:;,;",
            'ulist_comma': "group1:x:2000:,",

        }

        cases['/etc/shadow'] = {

            # a valid line as a template
            'valid': "user1:%(ep)s:16118:0:99999:7:::",

            # syntax traps
            'empty_fields': "::::::::",
            'one_field': "user1\n",
            'less_fields': "user1:%(ep)s:16118:0:99999:7::",
            'more_fields': "user1:%(ep)s:16118:0:99999:7::::hey",

            # funny
            'pegasus': "pegasus:$6$1.d1PKQWTeS.8eXI$qM5NXh0a9RRv7ajn"
                       "Sigd1wBqZp9l2.cVlJfPtybJUxbilM7HScfv.kW6EWcU"
                       "YKd3mU2bszTiQab0tKgmehzz5.:16146::::::",
            'root': "root:$6$s//AqmEc2gzQyFuL$9rUPnSPE6uYpDTBCiel"
                    "tHK5.5I0iJiiJva4bPnD5QH6/6iFKzJ32Lcrxxszz9In"
                    "BuorKT3ypIHeHeGvPkaPfO/:16118:0:99999:7:::",

            # field: name
            'name_empty': ":%(ep)s:16118:0:99999:7:::",
            'name_null': "\x00:%(ep)s:16118:0:99999:7:::",
            'name_tab': "\x09:%(ep)s:16118:0:99999:7:::",
            'name_space': " :%(ep)s:16118:0:99999:7:::",
            'name_semicol': ";:%(ep)s:16118:0:99999:7:::",
            'name_comma': ",:%(ep)s:16118:0:99999:7:::",
            'name_duplicate': "user1:%(ep)s:16118:0:99999:7:::\n"
                              "user1:%(ep)s:16118:0:99999:7:::",

            # field: password
            'password_empty': "user1:%(ep)s:16118:0:99999:7:::",
            'password_null': "user1:%(ep)s:16118:0:99999:7:::",
            'password_tab': "user1:%(ep)s:16118:0:99999:7:::",
            'password_space': "user1:%(ep)s:16118:0:99999:7:::",
            'password_semicol': "user1:%(ep)s:16118:0:99999:7:::",
            'password_comma': "user1:%(ep)s:16118:0:99999:7:::",
            'password_2aster': "user1:%(ep)s:16118:0:99999:7:::",
            'password_2x': "user1:%(ep)s:16118:0:99999:7:::",
            'password_asterx': "user1:%(ep)s:16118:0:99999:7:::",
            'password_xaster': "user1:%(ep)s:16118:0:99999:7:::",

            # field: last change
            'lastch_empty': "user1:%(ep)s::0:99999:7:::",
            'lastch_null': "user1:%(ep)s:\x00:0:99999:7:::",
            'lastch_tab': "user1:%(ep)s:\x09:0:99999:7:::",
            'lastch_space': "user1:%(ep)s: :0:99999:7:::",
            'lastch_semicol': "user1:%(ep)s:;:0:99999:7:::",
            'lastch_comma': "user1:%(ep)s:,:0:99999:7:::",
            'lastch_string': "user1:%(ep)s:hey:0:99999:7:::",
            'lastch_negative': "user1:%(ep)s:-42:0:99999:7:::",
            'lastch_over15': "user1:%(ep)s:%(o15)d:0:99999:7:::",
            'lastch_over32': "user1:%(ep)s:%(o32)d:0:99999:7:::",
            'lastch_over_maxint': "user1:%(ep)s:%(omi)d:0:99999:7:::",
            'lastch_maxint': "user1:%(ep)s:%(mi)d:0:99999:7:::",

            # field: minimum age
            'minage_empty': "user1:%(ep)s:16118::99999:7:::",
            'minage_null': "user1:%(ep)s:16118:\x00:99999:7:::",
            'minage_tab': "user1:%(ep)s:16118:\x09:99999:7:::",
            'minage_space': "user1:%(ep)s:16118: :99999:7:::",
            'minage_semicol': "user1:%(ep)s:16118:;:99999:7:::",
            'minage_comma': "user1:%(ep)s:16118:,:99999:7:::",
            'minage_string': "user1:%(ep)s:16118:hey:99999:7:::",
            'minage_negative': "user1:%(ep)s:16118:-42:99999:7:::",
            'minage_over15': "user1:%(ep)s:16118:%(o15)d:99999:7:::",
            'minage_over32': "user1:%(ep)s:16118:%(o32)d:99999:7:::",
            'minage_over_maxint': "user1:%(ep)s:16118:%(omi)d:99999:7:::",
            'minage_maxint': "user1:%(ep)s:16118:%(mi)d:99999:7:::",

            # field: maximum age
            'maxage_empty': "user1:%(ep)s:16118:0::7:::",
            'maxage_null': "user1:%(ep)s:16118:0:\x00:7:::",
            'maxage_tab': "user1:%(ep)s:16118:0:\x09:7:::",
            'maxage_space': "user1:%(ep)s:16118:0: :7:::",
            'maxage_semicol': "user1:%(ep)s:16118:0:;:7:::",
            'maxage_comma': "user1:%(ep)s:16118:0:,:7:::",
            'maxage_string': "user1:%(ep)s:16118:0:hey:7:::",
            'maxage_negative': "user1:%(ep)s:16118:0:-42:7:::",
            'maxage_over15': "user1:%(ep)s:16118:0:%(o15)d:7:::",
            'maxage_over32': "user1:%(ep)s:16118:0:%(o32)d:7:::",
            'maxage_over_maxint': "user1:%(ep)s:16118:0:%(omi)d:7:::",
            'maxage_maxint': "user1:%(ep)s:16118:0:%(mi)d:7:::",

            # field: warning period
            'wperiod_empty': "user1:%(ep)s:16118:0:99999::::",
            'wperiod_null': "user1:%(ep)s:16118:0:99999:\x00:::",
            'wperiod_tab': "user1:%(ep)s:16118:0:99999:\x09:::",
            'wperiod_space': "user1:%(ep)s:16118:0:99999: :::",
            'wperiod_semicol': "user1:%(ep)s:16118:0:99999:;:::",
            'wperiod_comma': "user1:%(ep)s:16118:0:99999:,:::",
            'wperiod_string': "user1:%(ep)s:16118:0:99999:hey:::",
            'wperiod_negative': "user1:%(ep)s:16118:0:99999:-42:::",
            'wperiod_over15': "user1:%(ep)s:16118:0:99999:%(o15)d:::",
            'wperiod_over32': "user1:%(ep)s:16118:0:99999:%(o32)d:::",
            'wperiod_over_maxint': "user1:%(ep)s:16118:0:99999:%(omi)d:::",
            'wperiod_maxint': "user1:%(ep)s:16118:0:99999:%(mi)d:::",

            # field: inactivity period
            'iperiod_empty': "user1:%(ep)s:16118:0:99999:7:::",
            'iperiod_null': "user1:%(ep)s:16118:0:99999:7:\x00::",
            'iperiod_tab': "user1:%(ep)s:16118:0:99999:7:\x09::",
            'iperiod_space': "user1:%(ep)s:16118:0:99999:7: ::",
            'iperiod_semicol': "user1:%(ep)s:16118:0:99999:7:;::",
            'iperiod_comma': "user1:%(ep)s:16118:0:99999:7:,::",
            'iperiod_string': "user1:%(ep)s:16118:0:99999:7:hey::",
            'iperiod_negative': "user1:%(ep)s:16118:0:99999:7:-42::",
            'iperiod_over15': "user1:%(ep)s:16118:0:99999:7:%(o15)d::",
            'iperiod_over32': "user1:%(ep)s:16118:0:99999:7:%(o32)d::",
            'iperiod_over_maxint': "user1:%(ep)s:16118:0:99999:7:%(omi)d::",
            'iperiod_maxint': "user1:%(ep)s:16118:0:99999:7:%(mi)d::",

            # field: account expiration date
            'expdate_empty': "user1:%(ep)s:16118:0:99999:7:::",
            'expdate_null': "user1:%(ep)s:16118:0:99999:7::\x00:",
            'expdate_tab': "user1:%(ep)s:16118:0:99999:7::\x09:",
            'expdate_space': "user1:%(ep)s:16118:0:99999:7:: :",
            'expdate_semicol': "user1:%(ep)s:16118:0:99999:7::;:",
            'expdate_comma': "user1:%(ep)s:16118:0:99999:7::,:",
            'expdate_string': "user1:%(ep)s:16118:0:99999:7::hey:",
            'expdate_negative': "user1:%(ep)s:16118:0:99999:7::-42:",
            'expdate_over15': "user1:%(ep)s:16118:0:99999:7::%(o15)d:",
            'expdate_over32': "user1:%(ep)s:16118:0:99999:7::%(o32)d:",
            'expdate_over_maxint': "user1:%(ep)s:16118:0:99999:7::%(omi)d:",
            'expdate_maxint': "user1:%(ep)s:16118:0:99999:7::%(mi)d:",

        }

        cases['/etc/gshadow'] = {

            # a valid line as a template
            'valid': "group1:%(ep)s:user1,user2:user3,user4",

            # syntax traps
            'empty_fields': ":::",
            'one_field': "user1\n",
            'less_fields': "group1:%(ep)s:user1,user2",
            'more_fields': "group1:%(ep)s:user1,user2:user3,user4:hey",

            # funny
            'pegasus': "pegasus:!::",
            'root': "root:::",

            # field: name
            'name_empty': ":%(ep)s:user1,user2:user3,user4",
            'name_null': "\x00:%(ep)s:user1,user2:user3,user4",
            'name_tab': "group1:%(ep)s:user1,user2:user3,user4",
            'name_space': "group1:%(ep)s:user1,user2:user3,user4",
            'name_semicol': "group1:%(ep)s:user1,user2:user3,user4",
            'name_comma': "group1:%(ep)s:user1,user2:user3,user4",
            'name_duplicate': "group1:%(ep)s:user1,user2:user3,user4\n"
                              "group1:%(ep)s:user1,user2:user3,user4",

            # field: password
            'password_empty': "group1::user1,user2:user3,user4",
            'password_null': "group1:\x00:user1,user2:user3,user4",
            'password_tab': "group1:\x09:user1,user2:user3,user4",
            'password_space': "group1: :user1,user2:user3,user4",
            'password_semicol': "group1:;:user1,user2:user3,user4",
            'password_comma': "group1:,:user1,user2:user3,user4",
            'password_2aster': "group1:**:user1,user2:user3,user4",
            'password_2x': "group1:xx:user1,user2:user3,user4",
            'password_asterx': "group1:*x:user1,user2:user3,user4",
            'password_xaster': "group1:x*:user1,user2:user3,user4",

            # field: alist
            'alist_null': "group1:%(ep)s:\x00:user3,user4",
            'alist_tab': "group1:%(ep)s:\x00:user3,user4",
            'alist_space': "group1:%(ep)s: :user3,user4",
            'alist_semicol': "group1:%(ep)s:;:user3,user4",
            'alist_nulls': "group1:%(ep)s:\x00,\x00:user3,user4",
            'alist_tabs': "group1:%(ep)s:\x09,\x09:user3,user4",
            'alist_spaces': "group1:%(ep)s: , :user3,user4",
            'alist_semicols': "group1:%(ep)s:;,;:user3,user4",
            'alist_comma': "group1:%(ep)s:,:user3,user4",

            # field: ulist
            'ulist_null': "group1:%(ep)s:user1,user2:\x00",
            'ulist_tab': "group1:%(ep)s:user1,user2:\x00",
            'ulist_space': "group1:%(ep)s:user1,user2: ",
            'ulist_semicol': "group1:%(ep)s:user1,user2:;",
            'ulist_nulls': "group1:%(ep)s:user1,user2:\x00,\x00",
            'ulist_tabs': "group1:%(ep)s:user1,user2:\x09,\x09",
            'ulist_spaces': "group1:%(ep)s:user1,user2: , ",
            'ulist_semicols': "group1:%(ep)s:user1,user2:;,;",
            'ulist_comma': "group1:%(ep)s:user1,user2:,",

        }

        # add common cases
        #
        for key in cases:
            cases[key]['many_fields'] = ":".join(["field"] * 5000)

        # expand data
        for path in cases:
            for case in cases[path]:
                cases[path][case] = cases[path][case] % data

        return cases


# ......................................................................... #
# Actual test
# ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''' #

@mark_dangerous
class TestAccountInvalidEtc(AccountBase):

    CLASS_NAME = "LMI_Account"

    def setUp(self):
        self.bs = lmi.test.util.BackupStorage()
        self.bs.add_file("/etc/passwd")
        self.bs.add_file("/etc/shadow")
        self.bs.add_file("/etc/group")
        self.bs.add_file("/etc/gshadow")
        self.inst = self.cim_class.first_instance()
        self.addCleanup(self.bs.restore_all)
        self.crippler = PasswdCrippler()

    def assertSane(self):
        """
        Assert that main properties can be read
        """
        self.assertIsInstance(self.inst, LMIInstance)
        # check if it provides key properties
        for attr in ['CreationClassName', 'Name', 'SystemCreationClassName',
                     'SystemName']:
            self.assertIsNotNone(self.inst.properties_dict()[attr])

        # check if it provides other properties, which should be set
        for attr in ['host', 'UserID', 'UserPassword', 'UserPasswordEncoding']:
            self.assertIsNotNone(self.inst.properties_dict()[attr])

##
#  Here test methods are generated from data in PasswdCrippler instance,
#

crippler = PasswdCrippler()

for path in crippler.all_paths():
    for case in crippler.all_cases_for(path):
        for op in ['append', 'replace']:
            gen_cripple(path, case, op)
