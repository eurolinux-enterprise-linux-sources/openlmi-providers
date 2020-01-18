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
Abstraction for RPM package for test purposes.
"""

import re
import subprocess

import util

RE_NOT_INSTALLED = re.compile(r'package\s+([^[:space:]]+)\s+is not installed',
        re.IGNORECASE)

class Package(object):
    """
    Element of test package database. It's a container for package
    informations.
    """
    def __init__(self, name, epoch, ver, rel, arch, repoid,
            rpm_path, files=None):
        self._name = name
        if not epoch or (isinstance(epoch, basestring)
                and epoch.lower() == "(none)"):
            epoch = "0"
        self._epoch = int(epoch)
        self._ver = ver
        self._rel = rel
        self._arch = arch
        self._repoid = repoid
        self._rpm_path = rpm_path
        if files is None:
            files = set()
        else:
            files = set(files)
        self._files = files

    def __str__(self):
        return self.get_nevra()

    @property
    def name(self):
        """ :returns: Package name. """
        return self._name
    @property
    def epoch(self):
        """ :returns: Package epoch as an integer. """
        return self._epoch
    @property
    def ver(self):
        """ :returns: Version string of package. """
        return self._ver
    @property
    def rel(self):
        """ :returns: Release string of package. """
        return self._rel
    @property
    def arch(self):
        """ :returns: Architecture string of package. """
        return self._arch
    @property
    def repoid(self):
        """ :returns: Repository id of package, where it is available. """
        return self._repoid
    @property
    def nevra(self):
        """
        :returns: Nevra string of package with epoch part always present.
        """
        return self.get_nevra('ALWAYS')
    @property
    def evra(self):
        """
        :returns: Evra string of package. That's the same as *nevra* without a
            name.
        """
        attrs = ('epoch', 'ver', 'rel', 'arch')
        return util.make_evra(*[getattr(self, '_'+a) for a in attrs])
    @property
    def rpm_path(self):
        """
        :returns: Absolute path to rpm package.
        """
        return self._rpm_path

    @property
    def summary(self):
        """
        :returns: Package summary string.
        """
        return subprocess.check_output(
                ['/usr/bin/rpm', '-q', '--qf', '%{SUMMARY}', '-p',
                    self.rpm_path])

    def get_nevra(self, with_epoch='NOT_ZERO'):
        """
        :param string with_epoch: Says when the epoch part should appear
            in resulting string. There are following possible values:

            * NOT_ZERO - epoch shall be present if it's greater than 0
            * ALWAYS - epoch will be present
            * NEVER - epoch won't be present

        :returns: Package nevra string.
        :rtype: string
        """
        attrs = ('name', 'epoch', 'ver', 'rel', 'arch')
        return util.make_nevra(*[getattr(self, '_'+a) for a in attrs],
                with_epoch=with_epoch)

    def __contains__(self, path):
        return path in self._files
    def __iter__(self):
        for pkg in self._files.__iter__():
            yield pkg
    def __len__(self):
        return len(self._files)
    @property
    def files(self):
        """
        :returns: Set of files and directories installed by this package.
        :rtype: set
        """
        return self._files.copy()

def to_json(_encoder, pkg):
    """
    Converts package object to dictionary which json encoder can handle.

    :param _encoder: Instance of json encoder.
    :param pkg: Package object to convert.
    :type pkg: :py:class:`Package`
    :returns: Dictionary with package attributes.
    :rtype: dictionary
    """
    if not isinstance(pkg, Package):
        raise TypeError("not a Package object")
    return pkg.__dict__

def from_json(json_object):
    """
    Constructs a package object from dictionary loaded from json text.
    Inverse function to :py:func:`to_json`.

    :param dictionary json_object: Deserialized package as a dictionary.
    :returns: Package object.
    :rtype: :py:class:`Package`
    """
    if isinstance(json_object, dict) and '_arch' in json_object:
        kwargs = {k[1:]: v for k, v in json_object.items()}
        return Package(**kwargs)
    return json_object

def is_pkg_installed(pkg):
    """
    Check, whether package is installed.
    """
    if not isinstance(pkg, Package):
        return subprocess.call(["rpm", "--quiet", "-q", pkg]) == 0
    else:
        cmd = ["/usr/bin/rpm", "-q", "--qf", "%{EPOCH}:%{NVRA}\n", pkg.nevra]
        try:
            out = subprocess.check_output(cmd).splitlines()[0]
            epoch, _ = out.split(':')
            if not epoch or epoch.lower() == "(none)":
                epoch = "0"
            return int(epoch) == int(pkg.epoch)
        except subprocess.CalledProcessError:
            return False

def filter_installed_packages(pkgs, installed=True):
    """
    Filter the package set returning only installed or not installed packages.

    :param list pkgs: Packages to filter. If removed packages are requested
        only Package objects may be present, otherwise package names or nevra
        strings are allowed.
    :param boolean installed: Whether to return installed or uninstalled
        packages.
    :returns: Filtered packages. Nevra strings are returned for any installed
        packages from *pkgs* represented by strings (either by name or nevra),
        package objects will be returned as package objects.
    :rtype: set
    """
    if len(pkgs) < 1:
        return set()
    pkg_strings = []
    pkg_map = {}
    for pkg in pkgs:
        if isinstance(pkg, Package):
            pkg_map[pkg.nevra] = pkg
            pkg = pkg.nevra
        elif not installed:
            raise TypeError("packages must be objects of Package")
        pkg_strings.append(pkg)
    cmd = [ "/usr/bin/rpm", "-q", "--qf"
          , "%{NAME}-%{EPOCH}:%{VERSION}-%{RELEASE}.%{ARCH}\n"]
    cmd.extend(pkg_strings)
    process = subprocess.Popen(cmd,
            stdout=subprocess.PIPE, stderr=util.DEV_NULL)
    out, _ = process.communicate()
    if installed:
        result = set()
        for line in out.splitlines():
            if not util.RE_NEVRA.match(line):
                continue
            if line in result:
                continue
            result.add(pkg_map.get(line, line))
    else:
        result = set(pkg_strings)
        for match in RE_NOT_INSTALLED.finditer(out):
            if match.group(1) not in pkg_strings:
                raise ValueError('got unexpected package nevra "%s" which'
                        ' should be one of: %s'
                        % (match.group(1), str(pkg_strings)))
            pkg_strings.remove(match.group(1))
            result.remove(result)
        result = {pkg_map.get(pstr, pstr) for pstr in result}
    return result

def remove_pkgs(pkgs, *args, **kwargs):
    """
    Remove package with rpm command.

    :param pkgs: Either an instance of :py:class:`Package` or package name.
        If it's a name, any version will be removed. Otherwise the exact
        version must be installed for command to be successful.
    :param list args: List of parameters for rpm command.
    :param boolean suppress_stderr: Whether the standard error output of ``rpm``
        command shall be suppressed. Defaults to ``False``.
    """
    suppress_stderr = kwargs.pop('suppress_stderr', False)
    cmd = ["rpm", "--quiet"] + list(args) + ['-e']
    if isinstance(pkgs, (basestring, Package)):
        pkgs = [pkgs]
    pkg_strings = []
    for pkg in pkgs:
        if isinstance(pkg, Package):
            pkg_strings.append(pkg.nevra)
        else:
            pkg_strings.append(pkg)
    if len(pkg_strings) > 0:
        cmd.extend(pkg_strings)
        kwargs = {}
        if suppress_stderr:
            kwargs['stderr'] = util.DEV_NULL
        subprocess.call(cmd, **kwargs)

def install_pkgs(pkgs, *args):
    """
    Install a specific package.

    :param pkgs: Package object.
    :type pkgs: :py:class:`Package`
    """
    cmd = ["rpm", "--quiet"] + list(args) + ["-i"]
    if isinstance(pkgs, Package):
        pkgs = [pkgs]
    pkg_paths = []
    for pkg in pkgs:
        if not isinstance(pkg, Package):
            raise TypeError("pkg must be a Package instance")
        pkg_paths.append(pkg.rpm_path)
    if len(pkg_paths) > 0:
        cmd.extend(pkg_paths)
        subprocess.call(cmd)

