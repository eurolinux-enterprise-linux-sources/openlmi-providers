#!/usr/bin/env python
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
Unit tests for ``LMI_SoftwareIdentityFileCheck`` provider.
"""

import shutil
import os
import pywbem
import re
import stat
import subprocess
import unittest

from lmi.test.lmibase import enable_lmi_exceptions
import package
import swbase
import util

RE_CHECKSUM = re.compile(r'^([0-9a-fA-F]+)\s+.*')

SOFTWARE_ELEMENT_STATE_EXECUTABLE = 2
FILE_TYPE_UNKNOWN                 = 0
FILE_TYPE_FILE                    = 1
FILE_TYPE_DIRECTORY               = 2
FILE_TYPE_SYMLINK                 = 3
FILE_TYPE_FIFO                    = 4
FILE_TYPE_CHARDEV                 = 5
FILE_TYPE_BLOCKDEV                = 6
FAILED_FLAGS_EXISTENCE            = 0
FAILED_FLAGS_SIZE                 = 1
FAILED_FLAGS_MODE                 = 2
FAILED_FLAGS_CHECKSUM             = 3
FAILED_FLAGS_DEVNUM               = 4
FAILED_FLAGS_TARGET               = 5
FAILED_FLAGS_UID                  = 6
FAILED_FLAGS_GID                  = 7
FAILED_FLAGS_MTIME                = 8
FILE_MODE_XOTH                    = 0
FILE_MODE_WOTH                    = 1
FILE_MODE_ROTH                    = 2
FILE_MODE_XGRP                    = 3
FILE_MODE_WGRP                    = 4
FILE_MODE_RGRP                    = 5
FILE_MODE_XUSR                    = 6
FILE_MODE_WUSR                    = 7
FILE_MODE_RUSR                    = 8
FILE_MODE_SVTX                    = 9 # sticky bit
FILE_MODE_SGID                    = 10
FILE_MODE_SUID                    = 11
INVOKE_SATISFIED                  = 0
INVOKE_NOT_SATISFIED              = 2

# maps algorithm id to the shell command
HASH_COMMAND = {
        1  : "md5sum",
        2  : "sha1sum",
        8  : "sha256sum",
        9  : "sha384sum",
        10 : "sha512sum",
        11 : "sha224sum"
}

# maps algorithm id to the length of digest string
HASH_DIGEST_LENGTH = {
        1  : 32,    #MD5
        2  : 40,    #SHA1
        8  : 64,    #SHA256
        9  : 96,    #SHA384
        10 : 128,   #SHA512
        11 : 56     #SHA224
}

class TestSoftwareIdentityFileCheck(swbase.SwTestCase):
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_SoftwareIdentityFileCheck"
    KEYS = ("CheckID", "Name", "SoftwareElementID", "SoftwareElementState")

    def make_checksum_str(self, csumnum, filename):
        """
        :returns: Checksum of installed file.
        :rtype: string
        """
        return RE_CHECKSUM.match(subprocess.check_output([
            HASH_COMMAND[csumnum], filename])).group(1).lower()

    def make_op(self, pkg, file_name):
        """
        :returns: Object path of ``LMI_SoftwareIdentityFileCheck``
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        return self.cim_class.new_instance_name({
            "CheckID" : 'LMI:LMI_SoftwareIdentityFileCheck',
            "Name" : file_name,
            "SoftwareElementID" : pkg.nevra,
            "SoftwareElementState" : pywbem.Uint16(
                SOFTWARE_ELEMENT_STATE_EXECUTABLE),
            "TargetOperatingSystem" : util.get_target_operating_system(),
            "Version" : pkg.evra
        })

    def setUp(self):
        to_uninstall = set()
        for repo in self.repodb.values():
            for pkg in repo.packages:
                to_uninstall.add(pkg.name)
        to_uninstall = list(package.filter_installed_packages(to_uninstall))
        package.remove_pkgs(to_uninstall, suppress_stderr=True)

    def do_check_symlink(self, pkg, filepath, inst):
        """
        Check symbolink link.
        """
        target = os.readlink(filepath)
        stats = os.lstat(filepath)

        self.assertEqual(inst.FileType, pywbem.Uint16(FILE_TYPE_SYMLINK),
                "Unexpected file type of symlink for %s:%s"
                % (pkg.name, filepath))
        self.assertEqual(inst.UserID, stats.st_uid,
                "Unexpected uid of symlink for %s:%s"  % (pkg.name, filepath))
        self.assertEqual(inst.GroupID, stats.st_gid,
                "Unexpected gid of symlink for %s:%s"  % (pkg.name, filepath))
        self.assertEqual(inst.FileMode, stats.st_mode,
                "Unexpected mode of symlink for %s:%s" % (pkg.name, filepath))
        self.assertEqual(inst.FileSize, stats.st_size,
                "Unexpected size of symlink for %s:%s" % (pkg.name, filepath))
        self.assertEqual(inst.LinkTarget, target,
                "Unexpected target of symlink for %s:%s" % (pkg.name, filepath))
        self.assertEqual(inst.LastModificationTime, int(stats.st_mtime),
                "Unexpected mtime of symlink for %s:%s"  % (pkg.name, filepath))
        self.assertIsNone(inst.Checksum,
                "Checksum should be None for symlink %s:%s"
                % (pkg.name, filepath))
        self.assertIsNone(inst.FileChecksum,
                "FileChecksum should be None for symlink %s:%s"
                % (pkg.name, filepath))
        self.assertIsNone(inst.MD5Checksum,
                "MD5Checksum should be None for symlink %s:%s"
                % (pkg.name, filepath))

        # modify owner
        prev_user = inst.UserID
        os.lchown(filepath, stats.st_uid + 1, -1)
        inst.refresh()
        self.assertEqual(inst.UserID, inst.UserIDOriginal + 1,
            "Unexpected uid of modified symlink for %s:%s"%(
            pkg.name, filepath))
        self.assertEqual(inst.UserID, prev_user + 1,
            "Unexpected uid of modified symlink for %s:%s"%(
            pkg.name, filepath))
        self.assertEqual(inst.GroupID, stats.st_gid,
            "Unexpected gid of modified symlink for %s:%s"%(
            pkg.name, filepath))
        self.assertEqual(inst.FailedFlags, [FAILED_FLAGS_UID])

        # modify link_target
        os.remove(filepath)
        lt_modif = "wrong" + "*"*len(inst.LinkTargetOriginal)
        os.symlink(lt_modif, filepath)
        os.lchown(filepath, inst.UserIDOriginal, inst.GroupIDOriginal)

        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_TARGET]))
        self.assertGreater(len(inst.LinkTarget), len(inst.LinkTargetOriginal))

        self.assertTrue(inst.FileExists,
            "File %s:%s should exist"%(pkg.name, filepath))
        self.assertEqual(inst.FileType, inst.FileTypeOriginal,
            "File type should match for symlink %s:%s"%(pkg.name, filepath))
        self.assertNotEqual(inst.FileSizeOriginal, inst.FileSize,
            "File size should not match for symlink %s:%s"%(
                pkg.name, filepath))
        self.assertEqual(inst.FileMode, inst.FileModeOriginal,
            "File mode should match for symlink %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.LinkTarget, lt_modif,
            "Link target should match modified path %s:%s"%(
                pkg.name, filepath))
        self.assertNotEqual(inst.LinkTargetOriginal, inst.LinkTarget,
            "Link target should not match for symlink %s:%s"%(
                pkg.name, filepath))
        self.assertEqual(inst.UserID, inst.UserIDOriginal,
            "File uid should match for symlink %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.GroupID, inst.GroupIDOriginal,
            "File gid should match for symlink %s:%s"%(pkg.name, filepath))

    def do_check_directory(self, pkg, filepath, inst):
        """
        Check directory.
        """
        stats = os.lstat(filepath)

        self.assertEqual(inst.FileType, pywbem.Uint16(FILE_TYPE_DIRECTORY),
                "Unexpected type for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.UserID, stats.st_uid,
                "Unexpected uid for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.GroupID, stats.st_gid,
                "Unexpected gid for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.FileMode, stats.st_mode,
                "Unexpected mode for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.FileSize, stats.st_size,
                "Unexpected size for directory %s:%s"%(pkg.name, filepath))
        self.assertIs(inst.LinkTarget, None)
        self.assertIsNone(inst.FileChecksum,
                "Unexpected checksum for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.LastModificationTime, int(stats.st_mtime),
                "Unexpected mtime for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.FailedFlags, [])

    def do_check_file(self, pkg, filepath, inst):
        """
        Check regular file.
        """
        stats = os.lstat(filepath)

        self.assertEqual(inst.FileType, pywbem.Uint16(FILE_TYPE_FILE),
            "Unexpected file type for %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.UserID, stats.st_uid,
            "Unexpected file uid for %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.GroupID, stats.st_gid,
            "Unexpected gid for regular file %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.FileMode, stats.st_mode,
            "Unexpected mode for regular file %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.FileSize, stats.st_size,
            "Unexpected size for regular file %s:%s"%(pkg.name, filepath))
        self.assertIs(inst.LinkTarget, None)
        csum = self.make_checksum_str(inst.ChecksumType, filepath)
        self.assertEqual(inst.FileChecksum.lower(), csum,
            "Unexpected checksum for regular file %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.LastModificationTime,
                inst.LastModificationTimeOriginal,
                "Unexpected mtime for regular file %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.LastModificationTime, int(stats.st_mtime),
                "Unexpected mtime for regular file %s:%s"%(pkg.name, filepath))

        # make it longer
        with open(filepath, "a+") as fobj:
            fobj.write("data\n")
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([
                FAILED_FLAGS_SIZE,
                FAILED_FLAGS_CHECKSUM,
                FAILED_FLAGS_MTIME,
            ]))

        self.assertGreater(inst.FileSize, inst.FileSizeOriginal,
                "File size should be greater, then expected for regular file"
                " %s:%s"%(pkg.name, filepath))
        self.assertGreater(inst.LastModificationTime,
                inst.LastModificationTimeOriginal,
                "Unexpected mtime for regular file %s:%s"%(pkg.name, filepath))

        self.assertTrue(inst.FileExists,
            "Regular file should exist %s:%s"%(pkg.name, filepath))

        # change file type
        os.remove(filepath)
        os.symlink(filepath, filepath)
        os.lchown(filepath, inst.UserIDOriginal,
                inst.GroupIDOriginal)
        inst.refresh()
        self.assertNotEqual(inst.LinkTargetOriginal, inst.LinkTarget,
                "Link target should not match for %s:%s"%(pkg.name, filepath))
        self.assertNotEqual(inst.FileSizeOriginal, inst.FileSize,
                "File size should not match for %s:%s"%(pkg.name, filepath))
        self.assertGreater(inst.LastModificationTime,
                inst.LastModificationTimeOriginal,
                "File mtime should be greater than expected for %s:%s"%(
                    pkg.name, filepath))
        self.assertNotEqual(inst.FileTypeOriginal, inst.FileType,
                "File type should not match for %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.FileType, pywbem.Uint16(FILE_TYPE_SYMLINK),
                "File type should match for %s:%s"%(pkg.name, filepath))
        self.assertIn(pywbem.Uint16(FAILED_FLAGS_MODE), inst.FailedFlags)

        # remove it
        os.remove(filepath)
        inst.refresh()
        self.assertEqual(inst.LinkTarget, inst.LinkTargetOriginal,
                "Link target does not match for regular file %s:%s"%(
                    pkg.name, filepath))
        self.assertNotEqual(inst.FileSizeOriginal, inst.FileSize,
                "File size should not match for regular file %s:%s"%(
                    pkg.name, filepath))
        self.assertIsNone(inst.LastModificationTime)
        self.assertIsNone(inst.FileType)
        self.assertIsNone(inst.FileChecksum)
        self.assertIsNone(inst.FileMode)
        self.assertIsNone(inst.UserID)
        self.assertIsNone(inst.GroupID)
        self.assertFalse(inst.FileExists)
        self.assertEqual(inst.FailedFlags, [FAILED_FLAGS_EXISTENCE])

    def do_check_dev(self, pkg, filepath, inst):
        """
        Check device file.
        """
        stats = os.lstat(filepath)

        is_block = stat.S_ISBLK(stats.st_mode)
        self.assertEqual(inst.FileType, pywbem.Uint16(
            FILE_TYPE_BLOCKDEV if is_block else FILE_TYPE_CHARDEV),
                "Unexpected file type of dev file for %s:%s"
                % (pkg.name, filepath))
        self.assertEqual(inst.UserID, stats.st_uid,
                "Unexpected uid of dev file for %s:%s"  % (pkg.name, filepath))
        self.assertEqual(inst.GroupID, stats.st_gid,
                "Unexpected gid of dev file for %s:%s"  % (pkg.name, filepath))
        self.assertEqual(inst.FileMode, stats.st_mode,
                "Unexpected mode of dev file for %s:%s" % (pkg.name, filepath))
        self.assertEqual(inst.FileSize, stats.st_size,
                "Unexpected size of dev file for %s:%s" % (pkg.name, filepath))
        self.assertEqual(inst.LinkTarget, None)
        self.assertEqual(inst.LastModificationTime, int(stats.st_mtime),
                "Unexpected mtime of dev file for %s:%s" % (pkg.name, filepath))
        self.assertIsNone(inst.Checksum,
                "Checksum should be None for dev file %s:%s"
                % (pkg.name, filepath))
        self.assertIsNone(inst.FileChecksum,
                "FileChecksum should be None for dev file %s:%s"
                % (pkg.name, filepath))
        self.assertIsNone(inst.MD5Checksum,
                "MD5Checksum should be None for dev file %s:%s"
                % (pkg.name, filepath))

        os.remove(filepath)
        subprocess.call(['/usr/bin/mknod', filepath, 'b' if is_block else 'c',
            str(os.major(stats.st_rdev) + 1), str(os.minor(stats.st_rdev) + 1)])
        self.assertTrue(os.path.exists(filepath))
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_DEVNUM]))
        self.assertEqual(inst.FileMode, stats.st_mode)

    def check_filepath(self, pkg, filepath):
        """
        Make a check of particular file of package.
        All files are expected to have no flaw.
        """
        objpath = self.make_op(pkg, filepath)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)
        self.assertCIMNameEqual(inst.path, objpath,
            msg="Object paths of instance must match for %s:%s"%(
            pkg.name, filepath))
        for key in self.KEYS:
            if key.lower() == "targetoperatingsystem":
                self.assertIsInstance(getattr(objpath, key), (int, long))
            else:
                self.assertEqual(getattr(objpath, key), getattr(inst, key),
                    "OP key %s values should match for %s:%s"%(
                    key, pkg.name, filepath))

        self.assertTrue(inst.FileExists,
            "File %s:%s must exist"%(pkg.name, filepath))
        self.assertEqual(len(inst.FailedFlags), 0,
                "FailedFlags must be empty for %s:%s, not: %s" % (
                    str(pkg), filepath, inst.FailedFlags))

        for prop in ( "FileType", "UserID", "GroupID"
                    , "FileMode", "FileSize", "LinkTarget"
                    , "FileChecksum", "FileModeFlags"):
            if (  (  (  os.path.islink(filepath)
                     or (not os.path.isfile(filepath)))
                  and prop == "FileSize")
               or (os.path.islink(filepath) and prop == "FileMode")):
                continue
            self.assertEqual(
                    getattr(inst, prop+"Original"), getattr(inst, prop),
                "%s should match for %s:%s"%(prop, pkg.name, filepath))
        if os.path.islink(filepath):
            self.do_check_symlink(pkg, filepath, inst)
        elif os.path.isdir(filepath):
            self.do_check_directory(pkg, filepath, inst)
        elif os.path.isfile(filepath):
            self.do_check_file(pkg, filepath, inst)

    @swbase.test_with_packages('stable#pkg1')
    def test_get_directory(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentityFileCheck`` with
        directory.
        """
        pkg = self.get_repo('stable')['pkg1']
        filepath = '/usr/share/openlmi-sw-test-pkg1'
        objpath = self.make_op(pkg, filepath)
        inst = objpath.to_instance()
        self.assertEqual(inst.FileType, FILE_TYPE_DIRECTORY)
        self.check_filepath(pkg, filepath)

    @swbase.test_with_packages('stable#pkg1')
    def test_check_config(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentityFileCheck`` with
        config file..
        """
        pkg = self.get_repo('stable')['pkg1']
        filepath = '/etc/openlmi/software/test/dummy_config.cfg'
        objpath = self.make_op(pkg, filepath)
        inst = objpath.to_instance()
        self.assertEqual(inst.FileType, FILE_TYPE_FILE)
        self.check_filepath(pkg, filepath)

    @enable_lmi_exceptions
    @swbase.test_with_packages('stable#pkg1', 'stable#pkg2')
    def test_get_instance_invalid(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentityFileCheck`` with
        file not belonging to particular package.
        """
        stable = self.get_repo('stable')
        pkg1 = stable['pkg1']
        pkg2 = stable['pkg2']
        fp1 = '/etc/openlmi/software/test/dummy_config.cfg'
        fp2 = '/usr/share/openlmi-sw-test-pkg2'
        objpath1 = self.make_op(pkg1, fp2)
        self.assertRaisesCIM(pywbem.CIM_ERR_NOT_FOUND, objpath1.to_instance)
        objpath2 = self.make_op(pkg2, fp1)
        self.assertRaisesCIM(pywbem.CIM_ERR_NOT_FOUND, objpath2.to_instance)
        objpath1.wrapped_object["Name"] = fp1
        self.assertNotEqual(objpath1.to_instance(), None)
        objpath2.wrapped_object["Name"] = fp2
        self.assertNotEqual(objpath2.to_instance(), None)

    @swbase.test_with_packages('stable#pkg2')
    def test_check_symlink(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentityFileCheck`` with
        symbolic link.
        """
        pkg = self.get_repo('stable')['pkg2']
        filepath = '/usr/share/openlmi-sw-test-pkg2/symlinks/absolute'
        objpath = self.make_op(pkg, filepath)
        inst = objpath.to_instance()
        self.assertEqual(inst.FileType, FILE_TYPE_SYMLINK)
        self.check_filepath(pkg, filepath)

    @swbase.test_with_packages('stable#pkg3')
    def test_check_chardev(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentityFileCheck`` with
        block and character device files.
        """
        pkg = self.get_repo('stable')['pkg3']
        filepath = '/usr/share/openlmi-sw-test-pkg3/devs/char12'
        objpath = self.make_op(pkg, filepath)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)
        self.assertEqual(inst.FileType, FILE_TYPE_CHARDEV)
        self.check_filepath(pkg, filepath)
        self.do_check_dev(pkg, filepath, inst)

    @swbase.test_with_packages('stable#pkg3')
    def test_check_blockdev(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentityFileCheck`` with
        block and character device files.
        """
        pkg = self.get_repo('stable')['pkg3']
        filepath = '/usr/share/openlmi-sw-test-pkg3/devs/block56'
        objpath = self.make_op(pkg, filepath)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)
        self.assertEqual(inst.FileType, FILE_TYPE_BLOCKDEV)
        self.check_filepath(pkg, filepath)
        self.do_check_dev(pkg, filepath, inst)

    @swbase.test_with_packages('stable#pkg4')
    def test_permissions(self):
        """
        Test ``FileMode`` property of ``LMI_SoftwareIdentityFileCheck``.
        """
        pkg = self.get_repo('stable')['pkg4']
        permsdir = '/usr/share/openlmi-sw-test-pkg4/perms'
        objpath = self.make_op(pkg, permsdir)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)
        permsdir += '/'
        self.assertEqual(inst.FileMode, stat.S_IFDIR | stat.S_ISVTX | 0777)
        self.assertEqual(set(inst.FileModeFlags), set(range(10)))

        objpath.wrapped_object["Name"] = permsdir + 'rwxrwxrwx'
        inst = objpath.to_instance()
        self.assertEqual(inst.FileMode, stat.S_IFREG | 0777)
        self.assertEqual(set(inst.FileModeFlags), set(range(9)))

        objpath.wrapped_object["Name"] = permsdir + 'rw-rw-rw-'
        inst = objpath.to_instance()
        self.assertEqual(inst.FileMode, stat.S_IFREG | 0666)
        self.assertEqual(set(inst.FileModeFlags), set([
            FILE_MODE_ROTH, FILE_MODE_RGRP, FILE_MODE_RUSR,
            FILE_MODE_WOTH, FILE_MODE_WGRP, FILE_MODE_WUSR]))

        objpath.wrapped_object["Name"] = permsdir + 'r--r--r--'
        inst = objpath.to_instance()
        self.assertEqual(inst.FileMode, stat.S_IFREG | 0444)
        self.assertEqual(set(inst.FileModeFlags), set([
            FILE_MODE_ROTH, FILE_MODE_RGRP, FILE_MODE_RUSR]))

        objpath.wrapped_object["Name"] = permsdir + 'r-xr-xr-x'
        inst = objpath.to_instance()
        self.assertEqual(inst.FileMode, stat.S_IFREG | 0555)
        self.assertEqual(set(inst.FileModeFlags), set([
            FILE_MODE_ROTH, FILE_MODE_RGRP, FILE_MODE_RUSR,
            FILE_MODE_XOTH, FILE_MODE_XGRP, FILE_MODE_XUSR]))

        objpath.wrapped_object["Name"] = permsdir + 'r---w---x'
        inst = objpath.to_instance()
        self.assertEqual(inst.FileMode, stat.S_IFREG | 0421)
        self.assertEqual(set(inst.FileModeFlags), set([
            FILE_MODE_RUSR, FILE_MODE_WGRP, FILE_MODE_XOTH]))

        objpath.wrapped_object["Name"] = permsdir + 'rwSr--r--'
        inst = objpath.to_instance()
        self.assertEqual(inst.FileMode, stat.S_IFREG | 04644)
        self.assertEqual(set(inst.FileModeFlags), set([
            FILE_MODE_RUSR, FILE_MODE_WUSR,
            FILE_MODE_RGRP, FILE_MODE_ROTH, FILE_MODE_SUID]))

        objpath.wrapped_object["Name"] = permsdir + 'rw-r-Sr--'
        inst = objpath.to_instance()
        self.assertEqual(inst.FileMode, stat.S_IFREG | 02644)
        self.assertEqual(set(inst.FileModeFlags), set([
            FILE_MODE_RUSR, FILE_MODE_WUSR,
            FILE_MODE_RGRP, FILE_MODE_ROTH, FILE_MODE_SGID]))

        objpath.wrapped_object["Name"] = permsdir + 'rw-r--r-T'
        inst = objpath.to_instance()
        self.assertEqual(inst.FileMode, stat.S_IFREG | 01644)
        self.assertEqual(set(inst.FileModeFlags), set([
            FILE_MODE_RUSR, FILE_MODE_WUSR,
            FILE_MODE_RGRP, FILE_MODE_ROTH, FILE_MODE_SVTX]))

        objpath.wrapped_object["Name"] = permsdir + 'rwsr-sr-t'
        inst = objpath.to_instance()
        self.assertEqual(inst.FileMode, stat.S_IFREG | 07755)
        self.assertEqual(set(inst.FileModeFlags), set([
            FILE_MODE_ROTH, FILE_MODE_RGRP, FILE_MODE_RUSR,
            FILE_MODE_XOTH, FILE_MODE_XGRP, FILE_MODE_XUSR,
            FILE_MODE_WUSR, FILE_MODE_SUID, FILE_MODE_SGID, FILE_MODE_SVTX]))

    @enable_lmi_exceptions
    def test_enum_instance_names(self):
        """
        Test ``EnumInstanceNames`` call on ``LMI_SoftwareIdentityFileCheck``
        that should not be supported.
        """
        self.assertRaisesCIM(pywbem.CIM_ERR_NOT_SUPPORTED,
                self.cim_class.instance_names)

    @swbase.test_with_packages('stable#pkg1')
    def test_method_invoke_on_file(self):
        """
        Test ``Invoke`` method of ``LMI_SoftwareIdentityFileCheck`` on regular
        file.
        """
        pkg = self.get_repo('stable')['pkg1']
        fp = '/usr/share/openlmi-sw-test-pkg1/README'
        objpath = self.make_op(pkg, fp)
        inst = objpath.to_instance()
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED,
                "Invoke method should be successful for %s:%s"  % (pkg, fp))
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED,
                "InvokeOnSystem method should be successful for %s:%s"
                % (pkg.name, fp))

        stats = os.stat(fp)
        backup = fp + '.bak'
        shutil.copy(fp, backup)

        # change mode
        os.chmod(fp, 0600)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_MODE]))

        # restore it
        shutil.copy(backup, fp)
        os.utime(fp, (stats.st_atime, stats.st_mtime))
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED)

        # modify contents
        with open(fp, 'a') as fobj:
            fobj.write('new line\n')
        os.utime(fp, (stats.st_atime, stats.st_mtime))
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags),
                set([FAILED_FLAGS_SIZE, FAILED_FLAGS_CHECKSUM]))

        # restore it
        shutil.copy(backup, fp)
        os.utime(fp, (stats.st_atime, stats.st_mtime))
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED)

        # touch it
        os.utime(fp, None)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_MTIME]))

    @enable_lmi_exceptions
    @swbase.test_with_packages('stable#pkg1')
    def test_method_invoke_on_uninstalled_package(self):
        """
        Test ``Invoke`` method of ``LMI_SoftwareIdentityFileCheck`` on
        uninstalled package.
        """
        pkg = self.get_repo('stable')['pkg1']
        fp = '/usr/share/openlmi-sw-test-pkg1/README'
        objpath = self.make_op(pkg, fp)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)
        package.remove_pkgs(pkg.name)
        self.assertRaisesCIM(pywbem.CIM_ERR_NOT_FOUND, inst.Invoke)

    @swbase.test_with_packages('stable#pkg2')
    def test_method_invoke_on_symlink(self):
        """
        Test ``Invoke`` method of ``LMI_SoftwareIdentityFileCheck`` on symbolic
        link.
        """
        pkg = self.get_repo('stable')['pkg2']
        fp = '/usr/share/openlmi-sw-test-pkg2/symlinks/relative'
        objpath = self.make_op(pkg, fp)
        inst = objpath.to_instance()
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED,
                "Invoke method should be successful for %s:%s"  % (pkg, fp))
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED,
                "InvokeOnSystem method should be successful for %s:%s"
                % (pkg.name, fp))
        stats = os.stat(fp)
        backup = fp + '.bak'
        shutil.copy(fp, backup)

        # touch it (mtime)
        os.utime(fp, None)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED)

        # change owner
        os.lchown(fp, 1, -1)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_UID]))

        # remove symlink
        os.remove(fp)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_EXISTENCE]))

        # restore it (mtime)
        os.symlink("../data/target.txt", fp)
        os.utime(fp, (stats.st_atime, stats.st_mtime))
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED)

        # point to different target
        os.remove(fp)
        os.symlink("../data/target.xtx", fp)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_TARGET]))

    @swbase.test_with_packages('stable#pkg3')
    def test_method_invoke_on_fifo(self):
        """
        Test ``Invoke`` method of ``LMI_SoftwareIdentityFileCheck`` on fifo.
        """
        pkg = self.get_repo('stable')['pkg3']
        fp = '/usr/share/openlmi-sw-test-pkg3/fifos/pipe'
        objpath = self.make_op(pkg, fp)
        inst = objpath.to_instance()
        self.assertEqual(inst.FileType, FILE_TYPE_FIFO)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED,
                "Invoke method should be successful for %s:%s"  % (pkg, fp))
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED,
                "InvokeOnSystem method should be successful for %s:%s"
                % (pkg.name, fp))
        stats = os.stat(fp)

        # touch it (mtime)
        os.utime(fp, None)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED)

        # change owner
        os.chown(fp, 1, -1)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_UID]))

        # remove fifo
        os.remove(fp)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_EXISTENCE]))

        # restore it
        os.mkfifo(fp, 0644)
        os.utime(fp, (stats.st_atime, stats.st_mtime))
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED)

        # change mode
        os.chmod(fp, 0400)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_MODE]))

        # replace it with regular file
        os.remove(fp)
        with open(fp, 'w') as fobj:
            fobj.write('dummy file')
        os.utime(fp, (stats.st_atime, stats.st_mtime))
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(inst.FileType, FILE_TYPE_FILE)
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_MODE]))

    @swbase.test_with_packages('stable#pkg4')
    def test_method_invoke_on_directory(self):
        """
        Test ``Invoke`` method of ``LMI_SoftwareIdentityFileCheck``
        on directory.
        """
        pkg = self.get_repo('stable')['pkg4']
        fp = '/usr/share/openlmi-sw-test-pkg4/perms'
        objpath = self.make_op(pkg, fp)
        inst = objpath.to_instance()
        self.assertEqual(inst.FileType, FILE_TYPE_DIRECTORY)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED,
                "Invoke method should be successful for %s:%s"  % (pkg, fp))
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED,
                "InvokeOnSystem method should be successful for %s:%s"
                % (pkg.name, fp))

        # touch it (mtime)
        os.utime(fp, None)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED)

        # remove child
        os.remove(os.path.join(fp, 'rwxrwxrwx'))
        os.utime(fp, None)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_SATISFIED)

        # chmod
        os.chmod(fp, 0755)
        (rval, _, _) = inst.Invoke()
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        (rval, _, _) = inst.InvokeOnSystem(TargetSystem=self.system_iname)
        self.assertEqual(rval, INVOKE_NOT_SATISFIED)
        inst.refresh()
        self.assertEqual(set(inst.FailedFlags), set([FAILED_FLAGS_MODE]))

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareIdentityFileCheck)

if __name__ == '__main__':
    unittest.main()
