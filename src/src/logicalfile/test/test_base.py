# Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
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
