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

import random


def field_in_passwd(username, number):
    """
    Return numberth field in /etc/passwd for given username
    """
    for line in open("/etc/passwd").readlines():
        if line.startswith(username):
            return line.split(":")[number].strip()


def field_in_shadow(username, number):
    """
    Return numberth field in /etc/shadow for given username
    """
    for line in open("/etc/shadow").readlines():
        if line.startswith(username):
            return line.split(":")[number].strip()


def field_in_group(groupname, number):
    """
    Return numberth field in /etc/group for given groupname
    """
    for line in open("/etc/group").readlines():
        if line.startswith(groupname):
            return line.split(":")[number].strip()


def random_shell():
    """
    Make up a funny shell
    """
    return random.choice([
        "/bin/ash",
        "/bin/cash",
        "/bin/dash",
        "/bin/hash",
        "/bin/nash",
        "/bin/mash",
        "/bin/sash",
        "/bin/stash",
        "/bin/splash",
        "/bin/wash",
    ])


def field_is_unique(fname, records):
    """
    True if the field in `records` has unique values.
    """
    seen = []
    for record in records:
        if record[fname] in seen:
            return False
        else:
            seen.append(record[fname])
    return True
