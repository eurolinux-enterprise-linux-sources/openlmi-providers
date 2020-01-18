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

import common
import lmi.test.util
import methods

import threading

from lmi.test import CIMError

# ......................................................................... #
# Helper methods
# ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''' #

def perform_attempts(tcls, names, args):
    """
    Spawn and wait for one thread per username, passing name and args.

    tcls can be threading.Thread subclass or a callable that returns
    instance of such. Must accept two arguments: username and connection
    to the CIM.  It also must implement attribute "result" to communicate
    result to parent threads.

    args is an arbitrary dict that should contain at least 'ns' for LMI
    Namespace object (usually conn.root.cimv2) plus any other objects that
    particular subclass needs.

    Returns list of results.
    """
    # create, start threads and wait for them to finish
    threads = []
    for name in names:
        threads.append(tcls(name, args))
    [t.start() for t in threads]
    [t.join() for t in threads]
    return [t.result for t in threads]


# ......................................................................... #
# Threading helper classes
# ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''' #

class AccountActionAttempt(threading.Thread):
    """
    Base class to perform an action with account in a thread.
    """

    def __init__(self, username, args):
        threading.Thread.__init__(self)
        self.args = args
        self.username = username
        self.ns = self.args['ns']
        self.result = None

    def _chkargs(self, keys):
        """Check keys in args; raise sensible message"""
        for key in keys:
            if key not in self.args:
                raise ValueError("%s needs %s passed in args"
                                 % (self.__class__.__name__, key))


class CreationAttempt(AccountActionAttempt):
    """
    Try to create user, mute normal errors (error is OK)
    """

    def run(self):
        self._chkargs(['system_iname'])
        system_iname = self.args['system_iname']
        lams = self.ns.LMI_AccountManagementService.first_instance()
        try:
            lams.CreateAccount(Name=self.username, System=system_iname)
            self.result = self.username
        except CIMError:
            # OK, error reported to user
            self.result = False


class ModificationAttempt(AccountActionAttempt):
    """
    Try to modify user
    """

    def run(self):
        account = self.ns.LMI_Account.first_instance({"Name": self.username})
        account.LoginShell = methods.random_shell()
        account.push()


class DeletionAttempt(AccountActionAttempt):
    """
    Try to delete user
    """

    def run(self):
        account = self.ns.LMI_Account.first_instance({"Name": self.username})
        account.DeleteUser()


# ......................................................................... #
# Actual test
# ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''' #

class TestAccountRaceConditions(common.AccountBase):

    def setUp(self):
        self.user_count = 20        # = thread count
        self.prefix = "user"
        self.names = [lmi.test.util.random_string(strength=8,
                                                  prefix=self.prefix)
                      for i in xrange(self.user_count)]

        self.bs = lmi.test.util.BackupStorage()
        self.bs.add_file("/etc/passwd")
        self.bs.add_file("/etc/shadow")
        self.bs.add_file("/etc/group")
        self.bs.add_file("/etc/gshadow")
        self.args = {'ns': self.ns}

    def tearDown(self):
        self.bs.restore_all()

    def assertPasswdNotCorrupt(self):
        """
        Assert /etc/passwd is not corrupt
        """
        pf = common.PasswdFile()
        errors = pf.get_errors()
        msg = ("/etc/passwd corrupt: %s\n\nFull text follows:\n%s\n"
               % (errors, pf.fulltext))
        self.assertFalse(errors, msg)

    def assertUserCount(self, count=None):
        """
        Assert particular user count in /etc/passwd (filter on prefix)
        """
        if count is None:
            count = self.user_count
        pf = common.PasswdFile({'username_prefix': self.prefix})
        oc = count
        rc = len(pf.users)
        self.assertEqual(rc, oc,
                         "wrong user count: %s, expected %s" % (rc, oc))

    def assertUserNameSet(self, nameset=None):
        """
        Assert particular username set in /etc/passwd (filter on prefix)
        """
        if nameset is None:
            nameset = self.names
        pf = common.PasswdFile({'username_prefix': self.prefix})
        on = sorted(nameset)
        rn = sorted(pf.get_names())
        self.assertEqual(rn, on,
                         "wrong user set: %s, expected %s" % (rn, on))

    def test_create(self):
        """
        Account: Test creations from many threads.
        """
        self.args['system_iname'] = self.system_iname
        created_names = filter(
            lambda r: isinstance(r, basestring),
            perform_attempts(CreationAttempt, self.names, self.args)
        )
        self.assertUserNameSet(created_names)
        self.assertPasswdNotCorrupt()

    def test_modify(self):
        """
        Account: Test modifications from many threads.
        """
        [common.UserOps.create_account(n) for n in self.names]
        perform_attempts(ModificationAttempt, self.names, self.args)
        self.assertUserCount()
        self.assertPasswdNotCorrupt()

    def test_delete(self):
        """
        Account: Test deletions from many threads.
        """
        [common.UserOps.create_account(n) for n in self.names]
        perform_attempts(DeletionAttempt, self.names, self.args)
        self.assertUserCount(0)
        self.assertPasswdNotCorrupt()
