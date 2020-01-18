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
# Authors: Radek Novacek <rnovacek@redhat.com>
# Authors: Michal Minar <miminar@redhat.com>

"""
Creation and manipulation utilities with rpm cache for software tests.
"""

import copy
import os
import json
import random
import re
from collections import defaultdict
from subprocess import call, check_output, CalledProcessError

import util
from package import Package, PackageEncoder, from_json

DB_BACKUP_FILE = 'lmi_software_test_cache'

RE_AVAIL_PKG = re.compile(
        r'^(?P<name>[^\s]+)\.(?P<arch>[a-zA-Z0-9_]+)'
        r'\s+((?P<epoch>[0-9]+):)?(?P<ver>[a-zA-Z0-9._+-]+)'
        r'-(?P<rel>[a-zA-Z0-9_.]+)\s+'
        r'(?P<repo>[a-zA-Z0-9_-]+)\s*$', re.MULTILINE)
# this won't match the last entry, unless "package\n" is not appended
# at the end of the string
RE_PKG_DEPS = re.compile(
        r'^package:\s*(?P<name>[^\s]+)\.(?P<arch>[a-zA-Z0-9_]+)'
        r'\s+((?P<epoch>[0-9]+):)?(?P<ver>[a-zA-Z0-9._+-]+)'
        r'-(?P<rel>[a-zA-Z0-9_.]+)\s+(?P<dep_list>.*?)'
        r'(?=^package|\Z)', re.MULTILINE | re.DOTALL)
RE_DEPS_PROVIDERS = re.compile(
        r'^\s*dependency:\s*(?P<dependency>.+?)\s*'
        r'(?P<providers>(^\s+unsatisfied\s+dependency$)|(^\s+provider:.+?$)+)',
        re.MULTILINE | re.IGNORECASE)
RE_DEPS_UNSATISFIED = re.compile(r'^\s+unsatisfied\s+dep.*$', re.IGNORECASE)
RE_PROVIDER = re.compile(
        r'^\s+provider:\s*(?P<name>[^\s]+)\.(?P<arch>[a-zA-Z0-9_]+)'
        r'\s+((?P<epoch>[0-9]+):)?(?P<ver>[a-zA-Z0-9._+-]+)'
        r'-(?P<rel>[a-zA-Z0-9_.]+)\s*$', re.IGNORECASE | re.MULTILINE)
RE_PKG_INFO = re.compile(
        r'^Name\s*:\s*(?P<name>[^\s]+).*?'
        r'^(Epoch\s*:\s*(?P<epoch>[0-9]+)\s+)?'
        r'^Version\s*:\s*(?P<ver>[a-zA-Z0-9._+-]+)\s+'
        r'^Release\s*:\s*(?P<rel>[^\s]+)\s+.*?'
        r'^Size\s*:\s*(?P<size>\d+(\.\d+)?)( *(?P<units>[kMG]))?',
        re.MULTILINE | re.DOTALL | re.IGNORECASE)
# matching packages won't be selected for dangerous tests
RE_AVAILABLE_EXCLUDE = re.compile(
        r'^(kernel|tog-pegasus|sblim|.*openlmi).*')

# Maximum number of packages, that will be selected for testing / 2
# There are 2 sets of test packages (safe and dangerous). When running
# in dangerous mode, the resulting number will be twice as much.
MAX_PKG_DB_SIZE = 10
# step used to iterate over package names used to check for thery dependencies
# it's a number of packages, that will be passed to yum command at once
PKG_DEPS_ITER_STEP = 50

class InvalidTestCache(Exception):
    """Exception saying, that rpm test cache is not valiid."""
    pass
class MissingRPM(InvalidTestCache):
    """
    Raised, when requested rpm for package is not contained in
    rpm test cache.
    """
    def __init__(self, pkg_name):
        InvalidTestCache.__init__(self,
                "Missing package '%s' in test cache!"%pkg_name)

def _is_sane_package(pkg_name):
    """
    :returns: Whether the given package can be included among tested ones.
    :rtype: boolean
    """
    return RE_AVAILABLE_EXCLUDE.match(pkg_name) is None

def _match_nevr(match):
    """
    @param match is a regexp match object with parsed rpm package
    @return tuple (name, epoch, version, release)
    """
    epoch = match.group('epoch')
    return ( match.group('name')
           , epoch if epoch and epoch.lower() != "(none)" else "0"
           , match.group('ver')
           , match.group('rel'))

def _match2pkg(match, safe=False):
    """
    @param match is a regexp match object with attributes:
      name, epoch, version, release, arch
    @param safe if True, the packe will have up_* properties equal to
    non-prefixed ones, otherwise they will be set to None
    @return instance of Package
    """
    kwargs = {}
    if safe is True:
        kwargs['safe'] = True
    else:
        for attr in ('epoch', 'ver', 'rel', 'repo'):
            kwargs['up_'+attr] = None
    epoch = match.group('epoch')
    if not epoch or epoch.lower() == "(none)":
        epoch = '0'
    return Package(
            match.group('name'),
            epoch,
            match.group('ver'), match.group('rel'),
            match.group('arch'), match.groupdict().get('repo', None),
            **kwargs)

def _filter_duplicates(installed, avail_str):
    """
    Parse output of "yum list available" command and retuns only those
    packages occuring in multiple versions.
    @param installed is a set of installed package names
    @param avail_str yum command output
    @return [ [pkg1v1, pkg1v2, ...], [pkg2v1, pkg2v2, ...], ... ]
    Each sublist of result contain at least 2 elements, that are instances
    of Package.
    """
    dups_list = []
    cur_package_matches = []
    prev_match = None
    system_arch = util.get_system_architecture()
    for match in RE_AVAIL_PKG.finditer(avail_str):
        if (  _match_nevr(match) in [   _match_nevr(m)
                                    for m in cur_package_matches]
           or (   (  not prev_match
                  or prev_match.group('name') in
                        [m.group('name') for m in cur_package_matches])
              and match.group('arch') not in ('noarch', system_arch))
           or not _is_sane_package(match.group('name'))):
            continue
        if prev_match and prev_match.group('name') != match.group('name'):
            if (   len(cur_package_matches) > 1
               and not cur_package_matches[0].group('name') in installed):
                pkgs = [ _match2pkg(m) for m in cur_package_matches ]
                dups_list.append(pkgs)
            cur_package_matches = []
        cur_package_matches.append(match)
        prev_match = match
    if len(cur_package_matches) > 1:
        dups_list.append([ _match2pkg(m) for m in cur_package_matches ])
    return dups_list

def _check_single_pkg_deps(
        installed,
        dependencies_str):
    """
    Each package has zero or more dependencies. Each dependency
    has at least one provider. One of these provider must be installed
    for each such dependency.
    @param dependencies of single package
    @return True if all dependencies have at least one provider installed
    """
    for match_deps in RE_DEPS_PROVIDERS.finditer(dependencies_str):
        providers = []
        if RE_DEPS_UNSATISFIED.search(match_deps.group('providers')):
            return False
        for match_dep in RE_PROVIDER.finditer(match_deps.group('providers')):
            if match_dep.group('name') not in installed:
                continue
            providers.append(_match2pkg(match_dep))
        for provider in providers:
            if is_pkg_installed(provider, False):
                break
        else:   # no provider is installed
            return False
    return True

def _check_pkg_dependencies(
        installed,
        dup_list,
        number_of_packages=MAX_PKG_DB_SIZE,
        repolist=None):
    """
    Finds packages from dup_list with satisfied (installed) dependencies.
    @param installed is a set of installed package names
    @return filtered dup_list with at least number_of_packages elements.
    """
    yum_params = ['deplist']
    dups_no_deps = []
    for i in range(0, len(dup_list), PKG_DEPS_ITER_STEP):
        dups_part = dup_list[i:i+PKG_DEPS_ITER_STEP]
        yum_params = yum_params[:1]
        for dups in dups_part:
            yum_params.extend([d.get_nevra(newer=False) for d in dups])
        deplist_str = util.run_yum(*yum_params, repolist=repolist)
        matches = RE_PKG_DEPS.finditer(deplist_str)
        prev_match = None
        for pkgs in dups_part:
            satisfied = True
            remains = len(pkgs)
            for match_pkg in matches:
                if (   prev_match
                   and _match_nevr(prev_match) == _match_nevr(match_pkg)):
                    # there sometimes appear duplicates in yum deplist
                    # output, so let's skip them
                    continue
                if satisfied and not _check_single_pkg_deps(
                        installed, match_pkg.group('dep_list')):
                    satisfied = False
                prev_match = match_pkg
                remains -= 1
                if remains <= 0:
                    break
            if satisfied:
                # all packages from pkgs have satisfied dependencies
                dups_no_deps.append(pkgs)
        if len(dups_no_deps) >= number_of_packages:
            break
    return dups_no_deps

def _sorted_db_by_size(pkgdb, repolist=None):
    """
    @param pkgdb is a list of lists of packages with common name
    @return sorted instances of Package according to their size
    """
    yum_params = ['info', '--showduplicates']
    yum_params.extend([ps[0].name for ps in pkgdb])
    info_str = util.run_yum(*yum_params, repolist=repolist)
    pkg_sizes = {}
    # to get correct ordering from "yum info" command
    # { pkg_name : [(epoch, version, release), ... ] }
    pkg_version_order = defaultdict(list)
    try:
        header = "Available Packages\n"
        info_str = info_str[info_str.index(header)+len(header):]
    except ValueError:
        pass
    for info_match in RE_PKG_INFO.finditer(info_str):
        pkg_name = info_match.group('name')
        size = float(info_match.group('size'))
        units = info_match.group('units')
        if units:
            size *= defaultdict(lambda: 1,
                    {'k':10**3, 'm':10**6, 'g':10**9})[units.lower()]
        pkg_sizes[pkg_name] = size
        epoch = info_match.group('epoch')
        if not epoch:
            epoch = "0"
        pkg_version_order[pkg_name].append((
            epoch, info_match.group('ver'), info_match.group('rel')))
    pkgdb = sorted(pkgdb, key=lambda pkgs: pkg_sizes[pkgs[0].name])[
            :MAX_PKG_DB_SIZE]

    for i, pkgs in enumerate(pkgdb):
        pkgs = sorted(pkgs, key=lambda p:
                pkg_version_order[pkgs[0].name].index((p.epoch, p.ver, p.rel)))
        pkg_kwargs = dict((k, getattr(pkgs[0], k)) for k in ('name', 'arch') )
        for attr in ('epoch', 'ver', 'rel', 'repo'):
            pkg_kwargs[attr] = getattr(pkgs[0], attr)
            pkg_kwargs['up_'+attr] = getattr(pkgs[-1], attr)
        pkgdb[i] = Package(**pkg_kwargs)
    return pkgdb

def _download_dangerous(repolist, pkgdb, cache_dir=None):
    """
    Downloads all rpm packages (old and newer versions) from package database
    to current directory.
    """
    repo_pkgs = defaultdict(list)
    for pkg in pkgdb:
        repo_pkgs[pkg.repo].append(pkg.name)
        repo_pkgs[pkg.up_repo].append(pkg.name)
    base_cmd = ['yumdownloader']
    if cache_dir:
        base_cmd.extend(['--destdir', cache_dir])
    for repo, pkgs in repo_pkgs.items():
        cmd = copy.copy(base_cmd)
        repos = set(repolist)
        try:
            repos.remove(repo)
        except KeyError:    # there may be duplicates
            continue
        for not_allowed_repo in repos:
            cmd.append('--disablerepo='+not_allowed_repo)
        cmd.append('--enablerepo='+repo)
        cmd.extend(pkgs)
        call(cmd)

def _make_rpm_path(pkg, cache_dir='', newer=True, without_epoch=False):
    """
    @param newer says, whether to use EVR of package to update
    (in this case, all epoch/ver/rel attributes will be prefixed with "up_")
    @param without_epoch if True, epoch will be left out of package name
    @return path to rpm package made from instance of Package
    """
    if not isinstance(pkg, Package):
        raise TypeError("pkg must be an instance of Package ")
    nevra = pkg.get_nevra(newer,
            with_epoch='NEVER' if without_epoch else 'NOT_ZERO')
    return os.path.join(cache_dir, nevra) + '.rpm'

def get_rpm_name(pkg, cache_dir='', newer=True):
    """
    Some packages do not have epoch in their name, even if it's higher than
    zero. That's why it's necessary to try more variants of rpm name.
    @return rpm path to package in cache
    """
    path = _make_rpm_path(pkg, cache_dir, newer)
    if os.path.exists(path):
        return path
    path = _make_rpm_path(pkg, cache_dir, newer, True)
    if os.path.exists(path):
        return path
    raise MissingRPM(pkg.name)

def rpm_exists(pkg, cache_dir='', newer=True):
    """
    @return True, when rpm package is in cache.
    """
    return (  os.path.exists(_make_rpm_path(pkg, cache_dir, newer))
           or os.path.exists(_make_rpm_path(pkg, cache_dir, newer, True)))


def remove_pkg(pkg, *args):
    """
    Remove package with rpm command.
    @param pkg is either instance of Package or package name
    @param args is a list of parameters for rpm command
    """
    if isinstance(pkg, Package):
        pkg = pkg.name
    call(["rpm", "--quiet"] + list(args) + ["-e", pkg])

def install_pkg(pkg, newer=True, repolist=None):
    """
    Install a specific package.
    @param pkg is either package name or instance of Package
    In latter case, a specific version is installed.
    @param repolist is a list of repositories, that should be
    used for downloading, if using yum
    when empty, all enabled repositories are used
    """
    if isinstance(pkg, Package):
        try:
            rpm_name = get_rpm_name(pkg, newer=newer)
            call(["rpm", "--quiet", "-i", rpm_name])
            return
        except MissingRPM:
            pass
        pkg = pkg.name
    util.run_yum('-q', '-y', 'install', pkg, repolist=repolist)

def ensure_pkg_installed(pkg, newer=True, repolist=None):
    """
    Ensures, that specific version of package is installed. If other
    version is installed, it is removed and reinstalled.
    """
    if not isinstance(pkg, Package):
        raise TypeError("pkg must be a Package instance")
    if not is_pkg_installed(pkg, newer):
        if is_pkg_installed(pkg.name):
            remove_pkg(pkg.name)
        install_pkg(pkg, newer, repolist)

def verify_pkg(pkg):
    """
    @return output of command rpm, with verification output for package
    """
    if isinstance(pkg, basestring):
        name = pkg
    elif isinstance(pkg, Package):
        name = pkg.name
    else:
        raise TypeError("pkg must be either package name or Package instance")
    return call(["rpm", "--quiet", "-Va", name]) == 0

def has_pkg_config_file(pkg, file_path):
    """
    @return True, if file_path is a configuration file of package pkg.
    """
    cmd = ['rpm', '-qc', pkg.name]
    out = check_output(cmd)
    return file_path in set(out.splitlines())   #pylint: disable=E1103

def has_pkg_doc_file(pkg, file_path):
    """
    @return True, if file_path is a documentation file of package pkg.
    """
    cmd = ['rpm', '-qd', pkg.name]
    out = check_output(cmd)
    return file_path in set(out.splitlines())   #pylint: disable=E1103

def is_pkg_installed(pkg, newer=True):
    """
    Check, whether package is installed.
    """
    if not isinstance(pkg, Package):
        return call(["rpm", "--quiet", "-q", pkg]) == 0
    else:
        cmd = [ "rpm", "-q", "--qf", "%{EPOCH}:%{NVRA}\n", pkg.get_nevra(
            newer, with_epoch='NEVER') ]
        try:
            out = check_output(cmd).splitlines()[0]
            epoch, nvra = out.split(':')    #pylint: disable=E1103
            if not epoch or epoch.lower() == "(none)":
                epoch = "0"
            return (   epoch == getattr(pkg, 'up_epoch' if newer else 'epoch')
                   and nvra  == pkg.get_nevra(newer=newer, with_epoch="NEVER"))
        except CalledProcessError:
            return False

def write_pkgdb(safe_pkgs, dangerous_pkgs, cache_dir=''):
    """
    Writes package database into a file named DB_BACKUP_FILE.
    """
    with open(os.path.join(cache_dir, DB_BACKUP_FILE), 'w') as db_file:
        data = (safe_pkgs, dangerous_pkgs)
        json.dump(data, db_file, cls=PackageEncoder,
                sort_keys=True, indent=4, separators=(',', ': '))

def load_pkgdb(cache_dir=''):
    """
    This is inverse function to _write_pkgdb().
    @return (safe, dangerous) package lists loaded from file
    """
    with open(os.path.join(cache_dir, DB_BACKUP_FILE), 'r') as db_file:
        safe, dangerous = json.load(db_file, object_hook=from_json)
    #print "Loaded package database from: %s" % date_time
    return safe, dangerous

def make_dangerous_list(installed, repolist=None):
    """
    This makes a list of instances of Package for dangerous tests.
    """
    avail_str = util.run_yum('list', 'available', '--showduplicates',
            repolist=repolist)
    # list of lists of packages with the same name, longer than 2
    dups_list = _filter_duplicates(installed, avail_str)
    selected = _check_pkg_dependencies(installed, dups_list,
                number_of_packages=MAX_PKG_DB_SIZE*5,
                repolist=repolist)
    return _sorted_db_by_size(selected, repolist=repolist)

def make_safe_list(installed, exclude=None):
    """
    Makes list of installed packages for non-dangerous tests.
    @param installed is a list of installed packages names
    @param exclude is a list of package names, that won't appear in result
    """
    if exclude is None:
        exclude = set()
    base = list(installed)
    random.shuffle(base)
    res = []
    i = 0
    while len(res) < MAX_PKG_DB_SIZE and i < len(base):
        name = base[i]
        i += 1
        if name in exclude or not _is_sane_package(name):
            continue
        cmd = [ "rpm", "-q", "--qf", "%{EPOCH}:%{NVRA}\n", name ]
        envra = check_output(cmd).splitlines()[0]
        res.append(_match2pkg(util.RE_ENVRA.match(envra), safe=True))
    return res

def get_pkg_database(
        force_update=False,
        use_cache=True,
        dangerous=False,
        cache_dir='',
        repolist=None):
    """
    Checks yum database for available packages, that have at least two
    different versions in repositories. Only not installed ones with
    all of their dependencies intalled are selected.
    And from those, few of the smallest are downloaded as rpms.
    @param dangerous whether to load available, not installed
    packages for dangerous tests
    @return (safe, dangerous)
      where
        safe is a list of instances of Package, representing installed
            software, these should be used for not-dangerous tests;
            both older
        dangerous is a list of instances of Package of selected packages,
            that are not installed, but available; instances contain
            both newer and older version of package
    """
    dangerous_pkgs = []
    if (   use_cache and not force_update
       and os.path.exists(os.path.join(cache_dir, DB_BACKUP_FILE))):
        safe_pkgs, dangerous_pkgs = load_pkgdb(cache_dir)
        valid_db = True
        if len(safe_pkgs) < MAX_PKG_DB_SIZE:
            valid_db = False
        elif not dangerous and len(dangerous_pkgs) > 0:
            dangerous_pkgs = []
        elif dangerous and len(dangerous_pkgs) == 0:
            valid_db = False
        else:
            for pkg in safe_pkgs:
                if not is_pkg_installed(pkg):
                    valid_db = False
                    break
            for pkg in dangerous_pkgs:
                if (  not rpm_exists(pkg, cache_dir, False)
                   or not rpm_exists(pkg, cache_dir, True)):
                    valid_db = False
                    #print "Old package database is not valid"
                    break
        if valid_db:
            return (safe_pkgs, dangerous_pkgs)
    #print "Getting installed packages"
    installed = set(check_output(   #pylint: disable=E1103
        ['rpm', '-qa', '--qf=%{NAME}\n']).splitlines())
    if dangerous:
        dangerous_pkgs = make_dangerous_list(installed)
    safe_pkgs = make_safe_list(installed, exclude=set(
        pkg.name for pkg in dangerous_pkgs))

    if use_cache:
        repolist = util.get_repo_list() if repolist in (None, []) else repolist
        _download_dangerous(repolist, dangerous_pkgs, cache_dir)
        #print "Backing up database information"
        write_pkgdb(safe_pkgs, dangerous_pkgs, cache_dir)
    return (safe_pkgs, dangerous_pkgs)

