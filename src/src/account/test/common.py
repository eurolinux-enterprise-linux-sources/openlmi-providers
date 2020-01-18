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
"""
Base class and utilities for all OpenLMI Account tests.
"""

import os
import subprocess
from collections import defaultdict

from methods import field_is_unique
from lmi.test.ind import IndicationTestDriver
from lmi.test import lmibase
from lmi.test.util import random_string


class AccountBase(lmibase.LmiTestCase):
    """
    Base class for all LMI Account tests.
    """

    USE_EXCEPTIONS = True

    @classmethod
    def setUpClass(cls):
        lmibase.LmiTestCase.setUpClass.im_func(cls)
        cls.user_name = os.environ.get("LMI_ACCOUNT_USER")
        cls.group_name = os.environ.get("LMI_ACCOUNT_GROUP")


class PasswdFile():
    """
    Parse /etc/passwd and perform basic heuristics to assess validity.

    By heuristics, I mean it's OK to include here what is considered to
    be "normal", or "expected" rather than strictly vaid/invalid.  For
    example, you can consider "not normal" to have UID!=GID, but depending
    on what you did, it could be OK.  OTOH, keep in mind that more specific
    things should be in the test itself.
    """

    DEFAULT_OPTIONS = {
        'username_prefix': 'user',
        'unique': [
            "name",
            "uid",
        ]
    }

    def __init__(self, options=None):
        self.options = self.__class__.DEFAULT_OPTIONS
        if options is not None:
            self.options.update(options)
        self.users = []

        with open('/etc/passwd') as pf:
            lines = pf.readlines()
        self.fulltext = "".join(lines)

        for line in lines:
            fields = line.split(":")
            user = {
                "name": fields[0],
                "password": fields[1],
                "uid": fields[2],
                "gid": fields[3],
                "gecos": fields[4],
                "directory": fields[5],
                "shell": fields[6],
            }
            if user['name'].startswith(self.options['username_prefix']):
                self.users.append(user)

    def find_dups(self):
        """
        Find dups in fields that should be unique
        """
        dups = defaultdict(int)
        for field in self.options['unique']:
            if not field_is_unique(field, self.users):
                dups[field] += 1
        return dict(dups)

    def get_errors(self):
        """
        Get hash of errors.
        """
        errlist = {}
        dups = self.find_dups()
        if dups:
            errlist['duplicates'] = dups
        return errlist

    def get_names(self):
        """
        Get list of user names
        """
        return [u['name'] for u in self.users]


class UserOps(object):

    @classmethod
    def _field_in_group(cls, groupname, number):
        """
        Return numberth field in /etc/group for given groupname
        """
        for line in open("/etc/group").readlines():
            if line.startswith(groupname):
                return line.split(":")[number].strip()

    @classmethod
    def add_user_to_group(cls, username, groupname):
        """
        Will add user to group
        """
        subprocess.check_call(["usermod", "-a", "-G", groupname, username])

    @classmethod
    def clean_account(cls, name):
        """
        Force to delete testing account and remove home dir
        """
        if cls.is_user(name):
            subprocess.check_call(["userdel", "-fr", name])
        if cls.is_group(name):
            # groups should be expicitely deleted
            subprocess.check_call(["groupdel", name])

    @classmethod
    def clean_group(cls, group):
        """
        Force to delete testing group
        """
        if cls.is_group(group):
            subprocess.check_call(["groupdel", group])

    @classmethod
    def create_account(cls, username):
        """
        Force to create account; run clean_account before creation
        """
        if not cls.is_user(username):
            subprocess.check_call(["useradd", username])

    @classmethod
    def create_group(cls, group_name):
        """
        Force to create group
        """
        if not cls.is_group(group_name):
            subprocess.check_call(["groupadd", group_name])

    @classmethod
    def list_users(cls):
        """
        List user names currently on the system
        """
        accts = []
        for line in open("/etc/passwd").readlines():
            acct, __ = line.split(":", 1)
            accts.append(acct)
        return accts

    @classmethod
    def list_groups(cls):
        """
        List group names currently on the system
        """
        accts = []
        for line in open("/etc/group").readlines():
            acct, __ = line.split(":", 1)
            accts.append(acct)
        return accts

    @classmethod
    def is_group(cls, name):
        """
        Return true/false if user does/does not exist
        """
        return name in cls.list_groups()

    @classmethod
    def is_user(cls, name):
        """
        Return true/false if user does/does not exist
        """
        return name in cls.list_users()


class TestUserSet(object):
    """
    Class to hold list of "testing" users, able to make up new names
    """

    def __init__(self, prefix="test_user_", strength=8):
        self.prefix = prefix
        self.strength = strength
        self._our_users = set()

    def _new_name(self):
        """
        Make up a name that is not yet on the system
        """
        name = None
        existing = UserOps.list_users()
        while not name or name in existing:
            name = random_string(strength=self.strength, prefix=self.prefix)
        return name

    def add(self, name=None):
        """
        Create a testing user; return the name
        """
        name = name if name else self._new_name()
        UserOps.create_account(name)
        self._our_users.add(name)
        return name

    def remove(self, name=None):
        """
        Remove given user or a random one; return the removed name
        """
        try:
            name = name if name else next(iter(self._our_users))
            assert name in self._our_users
        except StopIteration:   # from above next() call
            raise ValueError("no more testing users")
        except AssertionError:
            raise ValueError("not our testing user: %s" % name)
        UserOps.clean_account(name)
        self._our_users.remove(name)
        return name

    def remove_all(self):
        """
        Remove all testing users
        """
        while self._our_users:
            UserOps.clean_account(self._our_users.pop())


class TestGroupSet(object):
    """
    Class to hold list of "testing" groups, able to make up new names
    """

    def __init__(self, prefix="test_group_", strength=8):
        self.prefix = prefix
        self.strength = strength
        self._our_groups = set()

    def _new_name(self):
        """
        Make up a name that is not yet on the system
        """
        name = None
        existing = UserOps.list_groups()
        while not name or name in existing:
            name = random_string(strength=self.strength, prefix=self.prefix)
        return name

    def add(self, name=None):
        """
        Create a testing group; return the name
        """
        name = name if name else self._new_name()
        UserOps.create_account(name)
        self._our_groups.add(name)
        return name

    def remove(self, name=None):
        """
        Remove given group or a random one; return the removed name
        """
        try:
            name = name if name else next(iter(self._our_groups))
            assert name in self._our_groups
        except StopIteration:   # from above next() call
            raise ValueError("no more testing groups")
        except AssertionError:
            raise ValueError("not our testing group: %s" % name)
        UserOps.clean_account(name)
        self._our_groups.remove(name)
        return name

    def remove_all(self):
        """
        Remove all testing groups
        """
        while self._our_groups:
            UserOps.clean_account(self._our_groups.pop())


class AccountIndicationTestDriver(IndicationTestDriver):
    """
    Test driver for account indications.

    Adds necessary values to properties of IndicationTestDriver
    to constitute a functional test driver for Account Provider
    indications.
    """

    def __init__(self, *args, **kwargs):
        super(AccountIndicationTestDriver, self).__init__(*args, **kwargs)

        self.probe.classname_aliases = {
            'LMI_AccountInstanceCreationIndication': 'ac',
            'LMI_AccountInstanceDeletionIndication': 'ad',
            'LMI_Account': 'a',
            'LMI_Group': 'g',
        }

        self.tus = TestUserSet(
            prefix=self.options['gen_prefix'] + '_user_',
            strength=self.options['gen_strength']
        )

        self.tgs = TestGroupSet(
            prefix=self.options['gen_prefix'] + '_group_',
            strength=self.options['gen_strength']
        )

        self.known_actions['ac'] = self.tus.add
        self.known_actions['ad'] = self.tus.remove
        self.known_actions['gc'] = self.tgs.add
        self.known_actions['gd'] = self.tgs.remove

        self.known_queries['ac'] = (
            'select * from LMI_AccountInstanceCreationIndication'
            ' where SourceInstance isa LMI_Account'
        )
        self.known_queries['ad'] = (
            'select * from LMI_AccountInstanceDeletionIndication'
            ' where SourceInstance isa LMI_Account'
        )
        self.known_queries['gc'] = (
            'select * from LMI_AccountInstanceCreationIndication'
            ' where SourceInstance isa LMI_Group'
        )
        self.known_queries['gd'] = (
            'select * from LMI_AccountInstanceDeletionIndication'
            ' where SourceInstance isa LMI_Group'
        )

        self.cleanup_handlers.append(self.tus.remove_all)
        self.cleanup_handlers.append(self.tgs.remove_all)

