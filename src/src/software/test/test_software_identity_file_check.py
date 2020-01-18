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
Unit tests for LMI_SoftwareIdentity provider.
"""

import os
import pywbem
import re
import subprocess
import unittest

from lmi.test.lmibase import enable_lmi_exceptions
from lmi.test.util import mark_dangerous
import base
import rpmcache
import util

RE_CHECKSUM = re.compile(r'^([0-9a-fA-F]+)\s+.*')

class TestSoftwareIdentityFileCheck(
        base.SoftwareBaseTestCase): #pylint: disable=R0904
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_SoftwareIdentityFileCheck"
    KEYS = ("CheckID", "Name", "SoftwareElementID", "SoftwareElementState")

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

    @classmethod
    def needs_pkgdb_files(cls):
        return True

    def make_checksum_str(self, csumnum, filename):
        """
        @return checksum of installed file
        """
        return RE_CHECKSUM.match(subprocess.check_output([
            self.HASH_COMMAND[csumnum], filename])).group(1).lower()

    def make_op(self, pkg, file_name, newer=True):
        """
        :returns:  object path of `LMI_SoftwareIdentityFileCheck`
        """
        return self.cim_class.new_instance_name({
            "CheckID" : 'LMI:LMI_SoftwareIdentityFileCheck',
            "Name" : file_name,
            "SoftwareElementID" : pkg.get_nevra(
                newer=newer, with_epoch="ALWAYS"),
            "SoftwareElementState" : pywbem.Uint16(2),      #Executable
            "TargetOperatingSystem" : util.get_target_operating_system(),
            "Version" : getattr(pkg, 'up_evra' if newer else 'evra')
        })

    def do_check_symlink(self, pkg, filepath, inst, safe=False):
        """
        Assert some details about symlink.
        :param safe: (``bool``) If True, no modification is done to file.
        """
        target = os.readlink(filepath)
        stats = os.lstat(filepath)

        self.assertEqual(pywbem.Uint16(3), inst.FileType,
                "Unexpected file type of symlink for %s:%s"%(
                    pkg.name, filepath))
        self.assertEqual(stats.st_uid, inst.UserID,
                "Unexpected uid of symlink for %s:%s"%(pkg.name, filepath))
        self.assertEqual(stats.st_gid, inst.GroupID,
                "Unexpected gid of symlink for %s:%s"%(pkg.name, filepath))
        self.assertEqual(stats.st_mode, inst.FileMode,
                "Unexpected mode of symlink for %s:%s" %(pkg.name, filepath))
        self.assertEqual(stats.st_size, inst.FileSize,
                "Unexpected size of symlink for %s:%s"%(pkg.name, filepath))
        self.assertEqual(target, inst.LinkTarget,
                "Unexpected size of symlink for %s:%s"%(pkg.name, filepath))
        self.assertEqual(int(stats.st_mtime), inst.LastModificationTime,
                "Unexpected mtime of symlink for %s:%s"%(pkg.name, filepath))
        self.assertIsNone(inst.Checksum,
                "Checksum should be None for symlink %s:%s"%(
                    pkg.name, filepath))
        self.assertIsNone(inst.FileChecksum,
                "FileChecksum should be None for symlink %s:%s"%(
                    pkg.name, filepath))
        self.assertIsNone(inst.MD5Checksum,
                "MD5Checksum should be None for symlink %s:%s"%(
                    pkg.name, filepath))

        if safe:
            return

        # modify owner
        prev_user = inst.UserID
        os.lchown(filepath, stats.st_uid + 1, -1)
        inst.refresh()
        self.assertEqual(inst.UserIDOriginal + 1, inst.UserID,
            "Unexpected uid of modified symlink for %s:%s"%(
            pkg.name, filepath))
        self.assertEqual(prev_user + 1, inst.UserID,
            "Unexpected uid of modified symlink for %s:%s"%(
            pkg.name, filepath))
        self.assertEqual(stats.st_gid, inst.GroupID,
            "Unexpected gid of modified symlink for %s:%s"%(
            pkg.name, filepath))
        self.assertEqual([
                pywbem.Uint16(7) #UserID
            ], inst.FailedFlags)

        # modify link_target
        os.remove(filepath)
        lt_modif = "wrong" + "*"*len(inst.LinkTargetOriginal)
        os.symlink(lt_modif, filepath)
        os.lchown(filepath, inst.UserIDOriginal, inst.GroupIDOriginal)

        inst.refresh()
        self.assertEqual(set([
                pywbem.Uint16(1), #FileSize
                pywbem.Uint16(6), #LinkTarget
            ]), set(inst.FailedFlags))
        self.assertGreater(len(inst.LinkTarget),
                len(inst.LinkTargetOriginal))

        self.assertTrue(inst.FileExists,
            "File %s:%s should exist"%(pkg.name, filepath))
        self.assertEqual(inst.FileTypeOriginal, inst.FileType,
            "File type should match for symlink %s:%s"%(pkg.name, filepath))
        self.assertNotEqual(inst.FileSizeOriginal, inst.FileSize,
            "File size should not match for symlink %s:%s"%(
                pkg.name, filepath))
        self.assertEqual(inst.FileModeOriginal, inst.FileMode,
            "File mode should match for symlink %s:%s"%(pkg.name, filepath))
        self.assertEqual(lt_modif, inst.LinkTarget,
            "Link target should match modified path %s:%s"%(
                pkg.name, filepath))
        self.assertNotEqual(inst.LinkTargetOriginal, inst.ListTarget,
            "Link target should not match for symlink %s:%s"%(
                pkg.name, filepath))
        self.assertEqual(inst.UserIDOriginal, inst.UserID,
            "File uid should match for symlink %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.GroupIDOriginal, inst.GroupID,
            "File gid should match for symlink %s:%s"%(pkg.name, filepath))

    def do_check_directory(self, pkg, filepath, inst):
        """
        Assert some details about directory.
        """
        stats = os.lstat(filepath)

        self.assertEqual(pywbem.Uint16(2), inst.FileType,
                "Unexpected type for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(stats.st_uid, inst.UserID,
                "Unexpected uid for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(stats.st_gid, inst.GroupID,
                "Unexpected gid for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(stats.st_mode, inst.FileMode,
                "Unexpected mode for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(stats.st_size, inst.FileSize,
                "Unexpected size for directory %s:%s"%(pkg.name, filepath))
        self.assertIs(inst.LinkTarget, None)
        self.assertIsNone(inst.FileChecksum,
                "Unexpected checksum for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual(int(stats.st_mtime), inst.LastModificationTime,
                "Unexpected mtime for directory %s:%s"%(pkg.name, filepath))
        self.assertEqual([], inst.FailedFlags)

    def do_check_file(self, pkg, filepath, inst, safe=False):
        """
        Assert some details about regular file.
        :param safe: (``bool``) If True, no modification is done to file.
        """
        stats = os.lstat(filepath)

        self.assertEqual(pywbem.Uint16(1), inst.FileType,
            "Unexpected file type for %s:%s"%(pkg.name, filepath))
        self.assertEqual(stats.st_uid, inst.UserID,
            "Unexpected file uid for %s:%s"%(pkg.name, filepath))
        self.assertEqual(stats.st_gid, inst.GroupID,
            "Unexpected gid for regular file %s:%s"%(pkg.name, filepath))
        self.assertEqual(stats.st_mode, inst.FileMode,
            "Unexpected mode for regular file %s:%s"%(pkg.name, filepath))
        self.assertEqual(stats.st_size, inst.FileSize,
            "Unexpected size for regular file %s:%s"%(pkg.name, filepath))
        self.assertIs(inst.LinkTarget, None)
        csum = self.make_checksum_str(inst.ChecksumType, filepath)
        self.assertEqual(csum, inst.FileChecksum.lower(),
            "Unexpected checksum for regular file %s:%s"%(pkg.name, filepath))
        self.assertEqual(inst.LastModificationTime,
                inst.LastModificationTimeOriginal,
                "Unexpected mtime for regular file %s:%s"%(pkg.name, filepath))
        self.assertEqual(int(stats.st_mtime), inst.LastModificationTime,
                "Unexpected mtime for regular file %s:%s"%(pkg.name, filepath))

        if safe:
            return

        # make it longer
        with open(filepath, "a+") as fobj:
            fobj.write("data\n")
        inst.refresh()
        self.assertEqual(set([
                pywbem.Uint16(1),   #FileSize
                pywbem.Uint16(3),   #Checksum
                pywbem.Uint16(9),   #LastModificationTime
                ]), set(inst.FailedFlags))

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
        self.assertEqual(pywbem.Uint16(3), inst.FileType,
                "File type should match for %s:%s"%(pkg.name, filepath))
        self.assertIn(pywbem.Uint16(2),     #FileMode
                inst.FailedFlags)

        # remove it
        os.remove(filepath)
        inst.refresh()
        self.assertEqual(inst.LinkTargetOriginal, inst.LinkTarget,
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
        self.assertEqual([
                pywbem.Uint16(0), #exists
            ], inst.FileExists)

    def check_filepath(self, pkg, filepath, safe=False):
        """
        Make a check of particular file of package.
        All files are expected to have no flaw.

        :param safe: (``bool``) says, whether modifications to file can be done
        """
        objpath = self.make_op(pkg, filepath)
        inst = objpath.to_instance()
        self.assertCIMNameEqual(objpath, inst.path,
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
        self.assertEqual(0, len(inst.FailedFlags),
            "FailedFlags must be empty")

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
            self.do_check_symlink(pkg, filepath, inst, safe=safe)
        elif os.path.isdir(filepath):
            self.do_check_directory(pkg, filepath, inst)
        elif os.path.isfile(filepath):
            self.do_check_file(pkg, filepath, inst, safe=safe)

    @unittest.skip("not reliable test for random packages")
    def test_get_instance_safe(self):
        """
        Tests GetInstance call.
        """
        for pkg in self.safe_pkgs:
            files = self.pkgdb_files[pkg.name]
            for filepath in files:
                self.check_filepath(pkg, filepath, safe=True)

    @enable_lmi_exceptions
    def test_enum_instance_names(self):
        """
        Tests EnumInstanceNames - which should not be supported.
        """
        self.assertRaisesCIM(pywbem.CIM_ERR_NOT_SUPPORTED,
                self.cim_class.instance_names)

    @enable_lmi_exceptions
    @unittest.skip("not reliable test for random packages")
    def test_invoke_method_safe(self):
        """
        Test Invoke method in a safe manner.
        """
        for pkg in self.safe_pkgs:
            files = self.pkgdb_files[pkg.name]
            for filepath in files:
                objpath = self.make_op(pkg, filepath)
                inst = objpath.to_instance()

                (rval, _, _) = inst.Invoke()
                self.assertEqual(pywbem.Uint32(0),   #Satisfied
                        rval,
                        msg="Invoke method should be successful for %s:%s"%(
                            pkg.name, filepath))

                (rval, _, _) = inst.InvokeOnSystem(
                        TargetSystem=self.system_iname)

                self.assertEqual(pywbem.Uint32(0),  #Satisfied
                        rval, msg="InvokeOnSystem method should be successful"
                            " for %s:%s"%(pkg.name, filepath))

                system_iname = self.system_iname.wrapped_object
                system_iname["CreationClassName"] = "Bad class name"
                system_iname["Name"] = "some random name"
                self.assertRaisesCIM(pywbem.CIM_ERR_NOT_FOUND,
                        inst.InvokeOnSystem, TargetSystem=system_iname)

    @mark_dangerous
    def test_invoke_method(self):
        """
        Tests Invoke method invocation.
        """
        for pkg in self.dangerous_pkgs:
            if (   rpmcache.is_pkg_installed(pkg.name)
               and not util.verify_pkg(pkg.name)):
                rpmcache.remove_pkg(pkg.name)
            if not rpmcache.is_pkg_installed(pkg.name):
                rpmcache.install_pkg(pkg)
            for filepath in self.pkgdb_files[pkg.name]:
                objpath = self.make_op(pkg, filepath)
                inst = objpath.to_instance()

                (rval, _) = inst.Invoke()
                self.assertEqual(pywbem.Uint32(0),   #Satisfied
                    rval, "Invoke method should be successful for %s:%s"
                        % (pkg.name, filepath))

                # modify file
                if os.path.isfile(filepath):
                    os.remove(filepath)
                else:
                    os.lchown(filepath, os.stat(filepath).st_uid + 1, -1)
                (rval, _) = inst.Invoke()
                self.assertEqual(pywbem.Uint32(2), #Not Satisfied
                    rval,
                    "Invoke method should not pass for modified file %s:%s"
                        % (pkg.name, filepath))

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareIdentityFileCheck)

if __name__ == '__main__':
    unittest.main()
