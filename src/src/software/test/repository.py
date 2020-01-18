#!/usr/bin/python
# -*- Coding:utf-8 -*-
#
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
# Lesser General Public License for more details. #
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Authors: Michal Minar <miminar@redhat.com>
"""
Abstraction of YUM repository for test purposes.
"""

from collections import defaultdict, namedtuple
from datetime import datetime
import os
import re
from subprocess import call, check_output

RE_REPO_TAG = re.compile(
    ur'^repo-(?P<tag>[a-z_-]+)[\s\u00a0]*:'
    ur'[\s\u00a0]*(?P<value>.*?)[\s\u00a0]*$', re.I | re.U | re.M)
RE_REPO_INFOS = re.compile(
    r'^(repo-id\s*:.*?)(?:^\s*$)',
    re.IGNORECASE | re.MULTILINE | re.DOTALL)
RE_REPO_CONFIG = re.compile(
    ur'^(?P<tag>[a-z_-]+)[ \t\u00a0]*='
    ur'[ \t\u00a0]*((?P<value>.*?)[ \t\u00a0]*)?$',
    re.I | re.M | re.U)
RE_REPO_ENABLED = re.compile(
    r'^enabled\s*=\s*(true|false|0|1|yes|no)\s*$', re.I | re.M)

Repository = namedtuple("Repository",   #pylint: disable=C0103
        # repo properties
        [ "repoid"
        , "name"
        , "status"
        , "revision"
        , "tags"
        , "last_updated"
        , "pkg_count"
        , "base_urls"
        , "metalink"
        , "mirror_list"
        , "filename"
        , "cost"
        , "gpg_check"
        , "repo_gpg_check"
        ])

# example of repo information
#Repo-id      : updates-testing/18/x86_64
#Repo-name    : Fedora 18 - x86_64 - Test Updates
#Repo-status  : enabled
#Repo-revision: 1360887143
#Repo-tags    : binary-x86_64
#Repo-updated : Fri Feb 15 02:02:20 2013
#Repo-pkgs    : 13 808
#Repo-size    : 14 G
#Repo-baseurl : file:///mnt/globalsync/fedora/linux/updates/testing/18/x86_64
#Repo-expire  : 21 600 second(s) (last: Fri Feb 15 10:32:13 2013)
#Repo-filename: ///etc/yum.repos.d/fedora-updates-testing.repo

def _parse_repo_file(repo_name):
    """
    Parse output of yum-config-manager command for single repository.
    @return dictionary with key-value pairs suitable for passing to Repository
    constructor.
    """
    cmd = ["yum-config-manager", repo_name]
    out = check_output(cmd).decode('utf-8')
    result = {}
    for match in RE_REPO_CONFIG.finditer(out):
        tag = match.group('tag')
        value = match.group('value')
        if tag == "gpgcheck":
            result["gpg_check"] = value.lower() == "true"
        elif tag == "repo_gpgcheck":
            result["repo_gpg_check"] = value.lower() == "true"
        elif tag == "timeout":
            result[tag] = float(value)
        elif tag == "metadata_expire":
            result[tag] = int(value)
        elif tag == "cost":
            result[tag] = int(value)
        else:
            continue
    return result

def make_repo(repo_info):
    """
    Makes a Repository instance from string dumped by yum repoinfo command.
    """
    metadata = defaultdict(lambda : None)
    metadata["base_urls"] = []
    fields = set(Repository._fields)
    for match in RE_REPO_TAG.finditer(repo_info):
        tag = match.group('tag')
        value = match.group('value')
        try:
            new_tag = \
                { 'id'      : 'repoid'
                , 'updated' : 'last_updated'
                , 'pkgs'    : 'pkg_count'
                , 'baseurl' : 'base_urls'
                , 'mirrors' : 'mirror_list'
                }[tag]
        except KeyError:
            if tag not in fields:
                continue # unexpeted, or unwanted tag found
            new_tag = tag
        if new_tag == 'repoid':
            metadata[new_tag] = value.split('/')[0]
        elif new_tag == "base_urls":
            metadata[new_tag].append(value)
        elif new_tag == "last_updated":
            metadata[new_tag] = datetime.strptime(
                    value, "%a %b %d %H:%M:%S %Y")
        elif new_tag == "pkg_count":
            metadata[new_tag] = int(
                    u"".join(re.split(ur'[\s\u00a0]+', value, re.UNICODE)))
        elif new_tag == "status":
            metadata[new_tag] = value == "enabled"
        elif new_tag == 'revision':
            metadata[new_tag] = int(value)
        else:
            metadata[new_tag] = value
    config_items = _parse_repo_file(metadata['repoid'])
    for field in Repository._fields:
        if not field in metadata:
            metadata[field] = config_items.get(field, None)
    return Repository(**dict((f, metadata[f]) for f in Repository._fields))

def get_repo_database():
    """
    @return list of Repository instances for all configured repositories.
    """
    result = []
    cmd = ["yum", "-q", "repoinfo", "all"]
    repo_infos = check_output(cmd, env={'LANG': 'C'}).decode('utf-8')
    for match in RE_REPO_INFOS.finditer(repo_infos):
        result.append(make_repo(match.group(1)))
    return result

def is_repo_enabled(repo):
    """
    @return True, if repository is enabled
    """
    if isinstance(repo, Repository):
        repo = repo.repoid
    cmd = ["yum-config-manager", repo]
    out = check_output(cmd)
    match = RE_REPO_ENABLED.search(out)
    return bool(match) and match.group(1).lower() in {"true", "yes", "1"}

def set_repo_enabled(repo, enable):
    """
    Enables or disables repository in its configuration file.
    """
    if isinstance(repo, Repository):
        repo = repo.repoid
    cmd = ["yum-config-manager", "--enable" if enable else "--disable", repo]
    with open('/dev/null', 'w') as out:
        call(cmd, stdout=out, stderr=out)

