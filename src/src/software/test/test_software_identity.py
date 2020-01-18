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

from datetime import datetime, timedelta
import pywbem
import unittest

from lmi.shell.LMIUtil import lmi_wrap_cim_instance_name
from lmi.test.util import mark_dangerous
import base
import rpmcache
import util

class TestSoftwareIdentity(base.SoftwareBaseTestCase): #pylint: disable=R0904
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_SoftwareIdentity"
    KEYS = ("InstanceID", )

    def make_op(self, pkg, newer=True):
        """
        @return object path of SoftwareIdentity
        """
        return self.cim_class.new_instance_name({
            "InstanceID" : 'LMI:LMI_SoftwareIdentity:'
                + pkg.get_nevra(newer, "ALWAYS")
        })

    @mark_dangerous
    def test_get_instance(self):
        """
        Dangerous test, making sure, that properties are set correctly
        for installed and removed packages.
        """
        for pkg in self.dangerous_pkgs:
            if rpmcache.is_pkg_installed(pkg.name):
                rpmcache.remove_pkg(pkg.name)
            objpath = self.make_op(pkg)
            inst = objpath.to_instance()
            self.assertIsNone(inst.InstallDate)
            time_stamp = datetime.now(pywbem.cim_types.MinutesFromUTC(0)) \
                    - timedelta(seconds=0.01)
            rpmcache.install_pkg(pkg)
            inst.refresh()
            self.assertIsNotNone(inst.InstallDate)
            self.assertGreater(inst.InstallDate.datetime, time_stamp)

    def test_get_instance_safe(self):
        """
        Tests GetInstance call on packages from our rpm cache.
        """
        for pkg in self.safe_pkgs:
            objpath = self.make_op(pkg)
            inst = objpath.to_instance()
            self.assertCIMNameEqual(inst.path, objpath,
                    "Object paths should match for package %s"%pkg)
            for key in self.KEYS:
                self.assertIn(key, inst.properties(),
                        "OP is missing \"%s\" key for package %s"%(key, pkg))
            self.assertIsInstance(inst.Caption, basestring)
            self.assertIsInstance(inst.Description, basestring)
            self.assertEqual(pkg.up_evra, inst.VersionString,
                    "VersionString does not match evra for pkg %s" % pkg)
            self.assertTrue(inst.IsEntity)
            self.assertEqual(pkg.name, inst.Name,
                    "Name does not match for pkg %s" % pkg)
            self.assertIsInstance(inst.Epoch, pywbem.Uint32,
                    "Epoch does not match for pkg %s" % pkg)
            self.assertEqual(int(pkg.epoch), inst.Epoch)
            self.assertEqual(pkg.ver, inst.Version,
                    "Version does not match for pkg %s" % pkg)
            self.assertEqual(pkg.rel, inst.Release,
                    "Release does not match for pkg %s" % pkg)
            self.assertEqual(pkg.arch, inst.Architecture,
                    "Architecture does not match for pkg %s" % pkg)

            # try to leave out epoch part
            if pkg.epoch == "0":
                op_no_epoch = objpath.copy()
                op_no_epoch.wrapped_object["SoftwareElementID"] = \
                        util.make_nevra(pkg.name, pkg.up_epoch, pkg.up_ver,
                                pkg.up_rel, pkg.arch, "NEVER")
                self.assertNotIn(":", op_no_epoch.SoftwareElementID)
                inst = op_no_epoch.to_instance()
                self.assertCIMNameIn(inst.path, (objpath, op_no_epoch))

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareIdentity)

if __name__ == '__main__':
    unittest.main()
