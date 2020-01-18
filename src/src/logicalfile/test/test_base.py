# Copyright (C) 2013-2014 Red Hat, Inc.  All rights reserved.
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
# Authors: Jan Synacek <jsynacek@redhat.com>

import os
import shutil
import subprocess
import pyudev

from lmi.test import cimbase

class LogicalFileTestBase(cimbase.CIMTestCase):
    """
    Base class for all LogicalFile tests.
    """

    @classmethod
    def setUpClass(cls):
        cimbase.CIMTestCase.setUpClass.im_func(cls)
        cls.testdir = os.environ.get("LMI_LOGICALFILE_TESTDIR", "/var/tmp/logicalfile-tests")
        cls.selinux_enabled = True
        try:
            rc = subprocess.call(['selinuxenabled'])
            if rc != 0: cls.selinux_enabled = False
        except:
            cls.selinux_enabled = False
        ctx = pyudev.Context()
        sb = os.stat(os.path.realpath(cls.testdir + "/.."))
        device = pyudev.Device.from_device_number(ctx, "block", sb.st_dev)
        dev_name = device.get("ID_FS_UUID_ENC")
        if not dev_name:
            cls.fsname = "DEVICE=" + device.get("DEVNAME")
        else:
            cls.fsname = "UUID=" + dev_name

    def setUp(self):
        pass

    def tearDown(self):
        pass


class Configuration():
    """
    Ini-like configuration.

    {group : [(key1, value1), (key2, value2), ...]}

    """
    def __init__(self, path=None):
        self.configuration = dict()
        self.path = path
        if path:
            self.backupfile = path + '.backup_openlmi_logicalfile'
        self.backup = None
        self.original_path = None

    def add(self, group, options):
        if self.configuration.has_key(group):
            self.configuration[group].extend(options)
        else:
            self.configuration[group] = options

    def save(self):
        def get_original_path(path):
            """
            Compute already existing path when creating a directory. Returns
            path to original directory or the whole path.

            Example:
            Suppose there already exists '/tmp/stuff/' directory.
            get_original_path('/tmp/stuff/new/directory/file.txt') will return '/tmp/stuff/'.
            get_original_path('/tmp/stuff/file.txt') will return '/tmp/stuff/file.txt'.
            """
            (head, tail) = os.path.split(path)
            while (head != ''):
                if os.path.exists(head):
                    return os.path.join(head, tail)
                (head, tail) = os.path.split(head)
            return None

        if os.path.exists(self.path):
            # backup
            self.backup = self.backupfile
            shutil.move(self.path, self.backupfile)
        else:
            # create new
            self.original_path = get_original_path(self.path)
            d = os.path.dirname(self.path)
            if (self.path != self.original_path):
                os.makedirs(os.path.dirname(self.path))

        f = open(self.path, 'w')
        for group in self.configuration.keys():
            f.write('[' + group + ']' + '\n')
            for (key, val) in self.configuration[group]:
                f.write(key + '=' + val + '\n')
        f.close()

    def dispose(self):
        if self.backup:
            shutil.move(self.backupfile, self.path)
        elif self.original_path:
            if self.path != self.original_path:
                shutil.rmtree(self.original_path)
            else:
                os.unlink(self.path)
        self.backup = None
        self.original_path = None
