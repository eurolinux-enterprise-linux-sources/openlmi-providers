# -*- encoding: utf-8 -*-
# Software Management Providers
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
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Authors: Michal Minar <miminar@redhat.com>
#
"""
Module holding an abstraction for RPM package.
"""

from datetime import datetime
import yum

from lmi.software import util
from lmi.software.yumdb.util import is_pkg_installed

class PackageInfo(object):
    """
    Container for package metadata. It represents rpm package in yum
    database. It's supposed to be passed from YumWorker to YumDB client
    and vice-versa. Instances of YumAvailablePackage can not be exchanged
    -- results in segfaults.

    To speed up looking up of original yum package object on server, an
    atribute "objid" is provided.
    """
    __slots__ = (
            "objid",
            "name", "epoch", "version", "release", "architecture",
            'summary', 'description', 'license', 'group', 'vendor',
            "repoid", 'size',
            'installed',    # boolean
            'install_time'  # datetime instance
    )

    def __init__(self, objid, name, epoch, version, release, arch, **kwargs):
        """
        @param objid is an in of original yum package object, which is used
        by server for subsequent operations on this package requested by client
        """
        self.objid = objid
        self.name = name
        self.epoch = epoch
        self.version = version
        self.release = release
        self.architecture = arch
        self.summary = kwargs.pop('summary', None)
        self.description = kwargs.pop('description', None)
        self.license = kwargs.pop('license', None)
        self.group = kwargs.pop('group', None)
        self.vendor = kwargs.pop('vendor', None)
        self.repoid = kwargs.pop("repoid", None)
        self.size = kwargs.pop('size', None)
        if self.size is not None and not isinstance(self.size, (int, long)):
            raise TypeError('size must be an integer')
        self.installed = kwargs.pop('installed', None)
        if self.installed is not None:
            self.installed = bool(self.installed)
        self.install_time = kwargs.pop('install_time', None)
        if (   self.install_time is not None
           and not isinstance(self.install_time, datetime)):
            raise TypeError('install_time must be a datetime')

    # *************************************************************************
    # Properties
    # *************************************************************************
    @property
    def ver(self):
        """Shortcut for version property."""
        return self.version

    @property
    def rel(self):
        """Shortcut for release property."""
        return self.release

    @property
    def arch(self):
        """Shortcut for architecture property."""
        return self.architecture

    @property
    def nevra(self):
        """@return nevra of package with epoch always present."""
        return self.get_nevra(with_epoch="ALWAYS")

    @property
    def evra(self):
        """@return evra of package."""
        return "%s:%s-%s.%s" % (
                self.epoch if self.epoch and self.epoch != "(none)" else "0",
                self.version,
                self.release,
                self.architecture)

    @property
    def key_props(self):
        """
        @return package properties as dictionary,
        that uniquelly identify package in database
        """
        return dict((k, getattr(self, k)) for k in (
            'name', 'epoch', 'version', 'release', 'arch', 'repoid'))

    # *************************************************************************
    # Public methods
    # *************************************************************************
    def get_nevra(self, with_epoch='NOT_ZERO'):
        """
        @param with_epoch may be one of:
            "NOT_ZERO" - include epoch only if it's not zero
            "ALWAYS"   - include epoch always
            "NEVER"    - do not include epoch at all
        @return nevra of package
        """
        return util.make_nevra(self.name, self.epoch, self.version,
                self.release, self.arch, with_epoch)

    # *************************************************************************
    # Special methods
    # *************************************************************************
    def __str__(self):
        return self.nevra

    def __getstate__(self):
        """
        Used for serialization with pickle.
        @return container content that will be serialized
        """
        return dict((k, getattr(self, k)) for k in self.__slots__)

    def __setstate__(self, state):
        """
        Used for deserialization with pickle.
        Restores the object from serialized form.
        @param state is an object created by __setstate__() method
        """
        for k, value in state.items():
            setattr(self, k, value)

    def __eq__(self, other):
        return (   self.name == other.name
               and self.version == other.version
               and self.release == other.release
               and self.arch == other.arch
               and self.epoch == other.epoch
               and (  (self.repoid is None or other.repoid is None)
                   or (self.repoid == other.repoid)))

def make_package_from_db(pkg, rpmdb):
    """
    Create instance of :py:class:`PackageInfo` from instance of
    :py:class:`yum.packages.PackageObject`.

    :param pkg: Yum package object.
    :type pkg: :py:class:`yum.packages.PackageObject`
    :param rpmdb: Installed package sack.
    :type rpmdb: :py:class:`yum.rpmsack.RPMDBPackageSack`
    :rtype: :py:class:`PackageInfo`
    """
    metadata = dict((k, getattr(pkg, k)) for k in (
        'summary', 'description', 'license', 'group', 'vendor', 'size',
        'repoid'))
    if is_pkg_installed(pkg, rpmdb):
        metadata['installed'] = True
        if isinstance(pkg, yum.sqlitesack.YumAvailablePackageSqlite):
            metadata['install_time'] = datetime.fromtimestamp(
                    rpmdb._tup2pkg[pkg.pkgtup].installtime)
        else:
            metadata['install_time'] = datetime.fromtimestamp(pkg.installtime)
    else:
        metadata['installed'] = False
    res = PackageInfo(id(pkg), pkg.name, pkg.epoch, pkg.version, pkg.release,
            pkg.arch, **metadata)
    return res

