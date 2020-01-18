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

from collections import defaultdict
from datetime import datetime
import inspect
import os
import re
from subprocess import call, check_output

import util

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

class Repository(object):

    def __init__(self, repoid, name, status, revision,
            tags, last_updated, pkg_count, base_urls,
            metalink, mirror_list, filename, cost, timeout,
            gpg_check, repo_gpg_check, metadata_expire,
            config_path=None, packages=None):
        for attr in ('repoid', 'name', 'tags', 'metalink', 'filename'):
            setattr(self, attr, locals()[attr])
        for attr in ('gpg_check', 'repo_gpg_check', 'cost', 'status',
                'revision', 'last_updated', 'pkg_count',
                'base_urls', 'mirror_list', 'timeout', 'metadata_expire'):
            setattr(self, '_'+attr, None)
        self.base_urls = base_urls
        self.cost = cost
        self.gpg_check = gpg_check
        self.last_updated = last_updated
        self.metadata_expire = metadata_expire
        self.mirror_list = mirror_list
        self.pkg_count = pkg_count
        self.repo_gpg_check = repo_gpg_check
        self.revision = revision
        self.status = status
        self.timeout = timeout
        if config_path is None:
            repos_dir = os.environ.get('LMI_SOFTWARE_YUM_REPOS_DIR',
                    '/etc/yum.repos.d')
            config_path = os.path.join(repos_dir, '%s.repo' % self.repoid)
        self.config_path = config_path
        if packages is None:
            packages = set()
        self._packages = set(packages)

    @property
    def base_urls(self):
        return self._base_urls
    @base_urls.setter
    def base_urls(self, base_urls):
        if not isinstance(base_urls, list):
            raise TypeError('base_urls must be a list')
        self._base_urls = base_urls

    @property
    def local_path(self):
        if (   len(self._base_urls) == 1
           and self._base_urls[0].startswith('file://')):
            return self._base_urls[0][len('file://'):]

    @property
    def cost(self):
        return self._cost
    @cost.setter
    def cost(self, cost):
        self._cost = int(cost)

    @property
    def gpg_check(self):
        return self._gpg_check
    @gpg_check.setter
    def gpg_check(self, value):
        self._gpg_check = bool(value)

    @property
    def last_updated(self):
        return self._last_updated
    @last_updated.setter
    def last_updated(self, last_updated):
        if isinstance(last_updated, int):
            last_updated = datetime.fromtimestamp(last_updated)
        if last_updated is not None and not isinstance(last_updated, datetime):
            raise TypeError('last_updated must be a datetime object')
        self._last_updated = last_updated

    @property
    def metadata_expire(self):
        return self._metadata_expire
    @metadata_expire.setter
    def metadata_expire(self, metadata_expire):
        if not isinstance(metadata_expire, (int, long, basestring)):
            raise TypeError('metadata_expire must be either integer or datetime')
        self._metadata_expire = (
                     int(metadata_expire.strip().split(' ')[0])
                if   isinstance(metadata_expire, basestring)
                else metadata_expire)

    @property
    def mirror_list(self):
        return self._mirror_list
    @mirror_list.setter
    def mirror_list(self, mirror_list):
        if mirror_list is not None and not isinstance(mirror_list, basestring):
            raise TypeError('mirror list must be a string')
        if not mirror_list:
            mirror_list = None
        self._mirror_list = mirror_list

    @property
    def packages(self):
        return self._packages

    @property
    def pkg_count(self):
        return self._pkg_count
    @pkg_count.setter
    def pkg_count(self, pkg_count):
        self._pkg_count = None if pkg_count is None else int(pkg_count)

    @property
    def repo_gpg_check(self):
        return self._repo_gpg_check
    @repo_gpg_check.setter
    def repo_gpg_check(self, value):
        self._repo_gpg_check = bool(value)

    @property
    def revision(self):
        return self._revision
    @revision.setter
    def revision(self, value):
        self._revision = None if value is None else int(value)

    @property
    def status(self):
        return self._status
    @status.setter
    def status(self, value):
        self._status = bool(value)

    @property
    def timeout(self):
        return self._timeout
    @timeout.setter
    def timeout(self, timeout):
        self._timeout = float(timeout)

    def __getitem__(self, pkg_name):
        pkg_dict = {p.name : p for p in self._packages}
        try:
            return pkg_dict[pkg_name]
        except KeyError:
            try:
                return pkg_dict['openlmi-sw-test-' + pkg_name]
            except KeyError:
                raise KeyError(pkg_name)

    def refresh(self):
        updated_attrs = set()
        for attr, value in _parse_repo_file(self.repoid).items():
            try:
                setattr(self, attr, value)
                updated_attrs.add(attr)
            except AttributeError:
                print 'can\'t set attribute "%s"' % attr
        if not self.status:
            for attr in ('revision', 'tags', 'last_updated'):
                if attr not in updated_attrs and getattr(self, attr) is not None:
                    setattr(self, attr, None)

def to_json(encoder, repo):
    """
    Converts repository object to dictionary which json encoder can handle.

    :param encoder: Instance of json encoder.
    :param repo: Package object to convert.
    :type repo: :py:class:`Repository`
    :returns: Dictionary with repository attributes.
    :rtype: dictionary
    """
    if not isinstance(repo, Repository):
        raise TypeError("not a repo object")
    def transform_value(val):
        if not isinstance(val, (int, long, float, tuple, list,
                dict, bool, basestring)) and val is not None:
            return encoder.default(val)
        if isinstance(val, (list, tuple)):
            return type(val)((transform_value(v) for v in val))
        return val
    argspec = inspect.getargspec(repo.__init__)
    return { a: transform_value(getattr(repo, a)) for a in argspec.args[1:] }

def from_json(package_deserializer, json_object):
    """
    Constructs a repository object from dictionary loaded from json text.
    Inverse function to :py:func:`to_json`.

    :param callable package_deserializer: Function taking encoder and package
        dictionary as arguments, returning new package object.
    :param dictionary json_object: Deserialized repository as a dictionary.
    :returns: Package object.
    :rtype: :py:class:`Package`
    """
    if 'gpg_check' in json_object:
        kwargs = {}
        for key, value in json_object.items():
            if isinstance(value, (tuple, list)):
                value = type(value)((package_deserializer(v) for v in value))
            else:
                value = package_deserializer(value)
            kwargs[key] = value
        return Repository(**kwargs)
    return json_object

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
    Parse output of ``yum-config-manager`` command for single repository.

    :param string repo_name: Repository id.
    :returns: Dictionary with key-value pairs suitable for passing to
        :py:class:`Repository` constructor.
    :rtype: dictionary
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
        elif tag == 'baseurl':
            result['base_urls'] = [value]
        elif tag == 'enabled':
            result['status'] = value.lower() in ('true', 'yes', 1)
        elif tag == 'mirrorlist':
            result['mirror_list'] = value
        elif tag in {p for p in Repository.__dict__ if not p.startswith('_')}:
            continue
    return result

def make_repo(repo_info, packages=None):
    """
    Makes a Repository instance from string dumped by yum repoinfo command.
    """
    metadata = defaultdict(lambda : None)
    metadata["base_urls"] = []
    argspec = inspect.getargspec(Repository.__init__)
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
            if tag not in argspec.args:
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
    if packages is None:
        packages = set()
    metadata['packages'] = packages
    config_items = _parse_repo_file(metadata['repoid'])
    for field in argspec.args[1:]:  # leave out self argument
        if not field in metadata:
            metadata[field] = config_items.get(field, None)
    return Repository(**metadata)

def get_repo_list(kind='all'):
    if kind.lower() not in ('all', 'enabled', 'disabled'):
        raise ValueError('kind must be on of {"all", "enabled", "disabled"}')
    cmd = ["yum", "-q", "repoinfo", "all"]
    env = os.environ.copy()
    env['LC_ALL'] = 'C'
    return list(
                RE_REPO_TAG.search(m.group(1)).group('value').split('/')[0]
            for m in RE_REPO_INFOS.finditer(
                        check_output(cmd, env=env).decode('utf-8')))

def get_repo_database():
    """
    :returns: List of repository objects for all configured repositories.
    :rtype: list
    """
    result = []
    cmd = ["yum", "-q", "repoinfo", "all"]
    env = os.environ.copy()
    env['LC_ALL'] = 'C'
    repo_infos = check_output(cmd, env=env).decode('utf-8')
    for match in RE_REPO_INFOS.finditer(repo_infos):
        result.append(make_repo(match.group(1)))
    return result

def is_repo_enabled(repo):
    """
    :param repo: Either a repository id or instance of :py:class:`Repository`.
    :returns: Whether the repository is enabled.
    :rtype: boolean
    """
    if isinstance(repo, Repository):
        repo = repo.repoid
    cmd = ["yum-config-manager", repo]
    out = check_output(cmd)
    match = RE_REPO_ENABLED.search(out)
    return bool(match) and match.group(1).lower() in {"true", "yes", "1"}

def set_repos_enabled(repos, enable=True):
    """
    Enables or disables repository in its configuration file.

    :param repo: Eiether a repository id or instance of :py:class:`Repository`.
    :param boolean enable: New state of repository.
    """
    cmd = ["yum-config-manager", "--enable" if enable else "--disable"]
    repoids = []
    if isinstance(repos, (basestring, Repository)):
        repos = [repos]
    for repo in repos:
        if isinstance(repo, Repository):
            repo = repo.repoid
        repoids.append(repo)
    if len(repoids) > 0:
        cmd.extend(repoids)
        call(cmd, stdout=util.DEV_NULL, stderr=util.DEV_NULL)

