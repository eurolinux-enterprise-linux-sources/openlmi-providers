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
Module with definition of RPM package check class.
"""

from collections import OrderedDict
from datetime import datetime
import grp
import logging
import os
import pwd
import rpm
import yum

from lmi.software.yumdb import errors

CHECKSUMTYPE_STR2NUM = dict((val.lower(), k) for (k, val) in
        yum.constants.RPM_CHECKSUM_TYPES.items())

( FILE_TYPE_UNKNOWN
, FILE_TYPE_FILE
, FILE_TYPE_DIRECTORY
, FILE_TYPE_SYMLINK
, FILE_TYPE_FIFO
, FILE_TYPE_CHARACTER_DEVICE
, FILE_TYPE_BLOCK_DEVICE
) = range(7)

FILE_TYPE_NAMES = ( 'unknown', 'file', 'directory', 'symlink', 'fifo'
                  , 'character device', 'block device')

class PackageFile(object):
    """
    Metadata related to particular file on filesystem belonging to RPM package.
    Data contained here are from RPM database.

    Attributes:
      ``path``      - (``str``) Absolute path of file.
      ``file_type`` - (``int``) One of ``FILE_TYPE_*`` identifiers above.
      ``uid``       - (``int``) User ID.
      ``gid``       - (``int``) Group ID.
      ``mode``      - (``int``) Raw file mode.
      ``device``    - (``int``) Device number.
      ``mtime``     - (``int``) Last modification time in seconds.
      ``size``      - (``long``) File size as a number of bytes.
      ``link_target`` - (``str``) Link target of symlink. None if ``file_type``
                        is not symlink.
      ``checksum``  - (``str``) Checksum as string in hexadecimal format.
                       None if file is not a regular file.
    """
    __slots__ = ("path", "file_type", "uid", "gid", "mode", "device", "mtime",
            "size", "link_target", "checksum")

    def __init__(self, path, file_type, uid, gid, mode, device, mtime, size,
            link_target, checksum):
        if not isinstance(file_type, basestring):
            raise TypeError("file_type must be a string")
        for arg in ('uid', 'gid', 'mode', 'mtime', 'size'):
            if not isinstance(locals()[arg], (int, long)):
                raise TypeError("%s must be integer" % arg)
        if not os.path.isabs(path):
            raise ValueError("path must be an absolute path")
        self.path = path
        try:
            self.file_type = FILE_TYPE_NAMES.index(file_type.lower())
        except ValueError:
            logging.getLogger(__name__).error('unrecognized file type "%s" for'
                    ' file "%s"', file_type, path)
            self.file_type = FILE_TYPE_NAMES[FILE_TYPE_UNKNOWN]
        self.uid = uid
        self.gid = gid
        self.mode = mode
        self.device = device
        self.mtime = mtime
        self.size = size
        self.link_target = (link_target
                if self.file_type == FILE_TYPE_SYMLINK else None)
        self.checksum = checksum if self.file_type == FILE_TYPE_FILE else None

    @property
    def last_modification_datetime(self):
        """
        @return instance datetime for last modification time of file
        """
        return datetime.fromtimestamp(self.mtime)

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

class PackageCheck(object):
    """
    Metadata for package concerning verification.
    It contains metadata for each file installed in "files" attribute.
    """
    __slots__ = ("objid", "file_checksum_type", "files")

    def __init__(self, objid, file_checksum_type, files=None):
        """
        @param objid is an in of original yum package object, which is used
        by server for subsequent operations on this package requested by client
        """
        if files is not None and not isinstance(
                files, (list, tuple, set, dict)):
            raise TypeError("files must be an iterable container")
        self.objid = objid
        self.file_checksum_type = file_checksum_type
        if not isinstance(files, dict):
            self.files = OrderedDict()
            if files is not None:
                for file_check in sorted(files, key=lambda f: f.path):
                    self.files[file_check.path] = file_check
        else:
            for path in sorted(files):
                self.files[path] = files[path]

    def __iter__(self):
        return iter(self.files)

    def __len__(self):
        return len(self.files)

    def __getitem__(self, filepath):
        return self.files[filepath]

    def __setitem__(self, filepath, package_file):
        if not isinstance(package_file, PackageFile):
            raise TypeError("package_file must be a PackageFile instance")
        self.files[filepath] = package_file

    def __contains__(self, fileobj):
        if isinstance(fileobj, basestring):
            return fileobj in self.files
        elif isinstance(fileobj, PackageFile):
            return fileobj.path in self.files
        else:
            raise TypeError("expected file path for argument")

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

def pkg_checksum_type(pkg):
    """
    @return integer representation of checksum type
    """
    if not isinstance(pkg, yum.packages.YumAvailablePackage):
        raise TypeError("pkg must be an instance of YumAvailablePackage")
    if isinstance(pkg, yum.rpmsack.RPMInstalledPackage):
        return pkg.hdr[rpm.RPMTAG_FILEDIGESTALGO]
    return CHECKSUMTYPE_STR2NUM[pkg.yumdb_info.checksum_type.lower()]

def make_package_check_from_db(vpkg, file_name=None):
    """
    Create instance of PackageCheck from instance of
    yum.packages._RPMVerifyPackage.

    :param file_name: (``str``) If not None, causes result to have just
        one instance of ``PackageFile`` matching this file_name.
        If it's not found in the package, ``FileNotFound`` will be raised.
    :rtype (``PackageCheck``)
    """
    if not isinstance(vpkg, yum.packages._RPMVerifyPackage):
        raise TypeError("vpkg must be instance of"
                " yum.packages._RPMVerifyPackage")
    pkg = vpkg.po

    res = PackageCheck(id(pkg), pkg_checksum_type(pkg))
    files = res.files
    for vpf in vpkg:
        if file_name is not None and file_name != vpf.filename:
            continue
        files[vpf.filename] = PackageFile(
            vpf.filename,
            vpf.ftype,
            pwd.getpwnam(vpf.user).pw_uid,
            grp.getgrnam(vpf.group).gr_gid,
            vpf.mode,
            vpf.dev,
            vpf.mtime,
            vpf.size,
            vpf.readlink,
            vpf.digest[1]
        )
        if file_name is not None:
            break
    if file_name is not None and len(files) < 1:
        raise errors.FileNotFound('File "%s" not found in package "%s".' % (
            file_name, pkg.nevra))
    return res

