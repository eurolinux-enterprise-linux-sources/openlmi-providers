# -*- encoding: utf-8 -*-
# Copyright (C) 2012-2013 Red Hat, Inc.  All rights reserved.
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
# Authors: Roman Rakus <rrakus@redhat.com>
#

import subprocess

def user_exists(username):
    """
    Return true/false if user does/does not exists
    """
    got = field_in_passwd(username, 0)
    return got == username

def group_exists(groupname):
    """
    Return true/false if user does/does not exists
    """
    got = field_in_group(groupname, 0)
    return got == groupname

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

def clean_account(user_name):
    """
    Force to delete testing account and remove home dir
    """
    if user_exists(user_name):
        subprocess.check_call(["userdel", "-fr", user_name])
    if group_exists(user_name):
        # groups should be expicitely deleted
        subprocess.check_call(["groupdel", user_name])

def add_user_to_group(user_name, group_name):
    """
    Will add user to group
    """
    subprocess.check_call(["usermod", "-a", "-G", group_name, user_name])

def create_account(user_name):
    """
    Force to create account; run clean_account before creation
    """
    if not user_exists(user_name):
        subprocess.check_call(["useradd", user_name])

def clean_group(group_name):
    """
    Force to delete testing group
    """
    if group_exists(group_name):
        subprocess.check_call(["groupdel", group_name])

def create_group(group_name):
    """
    Force to create group
    """
    if not group_exists(group_name):
        subprocess.check_call(["groupadd", group_name])

