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
Unit tests for LMI_SoftwareIdentityResource provider.
"""

import pywbem
import time
import unittest

from lmi.software.core.IdentityResource import Values
from lmi.test.lmibase import enable_lmi_exceptions
from lmi.test.util import mark_dangerous
import base
import repository
import util

class TestSoftwareIdentityResource(
        base.SoftwareBaseTestCase): #pylint: disable=R0904
    """
    Basic cim operations test.
    """

    CLASS_NAME = "LMI_SoftwareIdentityResource"
    KEYS = ("CreationClassName", "Name", "SystemCreationClassName",
            "SystemName")

    @classmethod
    def needs_pkgdb(cls):
        return False

    @classmethod
    def needs_repodb(cls):
        return True

    def make_op(self, repo):
        """
        @param ses SoftwareElementState property value
        @return object path of SoftwareIdentityResource
        """
        if isinstance(repo, repository.Repository):
            repo = repo.repoid
        return self.cim_class.new_instance_name({
            "Name" : repo,
            "SystemName" : self.SYSTEM_NAME,
            "SystemCreationClassName" : self.system_cs_name,
            "CreationClassName" : self.CLASS_NAME
        })

    def tearDown(self):
        for repo in self.repodb:
            if repository.is_repo_enabled(repo) != repo.status:
                repository.set_repo_enabled(repo, repo.status)

    def _check_repo_instance(self, repo, inst):
        """
        Checks instance properties of repository.
        """
        objpath = self.make_op(repo)
        self.assertCIMNameEqual(objpath, inst.path)
        for key in self.KEYS:
            self.assertEqual(getattr(inst, key), getattr(inst.path, key))
        self.assertEqual(repo.repoid, inst.Name)
        self.assertIsInstance(inst.AccessContext, pywbem.Uint16)

        access_info = inst.AccessInfo
        if access_info is not None:
            access_info = access_info.rstrip('/')
        if access_info is not None:
            self.assertIsInstance(access_info, basestring)
        if repo.mirror_list:
            self.assertEqual(access_info, repo.mirror_list,
                    "AccessInfo does not match mirror_list for repo %s" %
                    repo.repoid)
        elif repo.metalink:
            self.assertEqual(access_info, repo.metalink,
                    "AccessInfo does not match metalink for repo %s" %
                    repo.repoid)
        elif access_info is not None:
            self.assertIn(access_info, {u.rstrip('/') for u in repo.base_urls},
                "AccessInfo missing in base_urls for repo %s" % repo.repoid)

        self.assertIsInstance(inst.AvailableRequestedStates, list)
        self.assertEqual(repo.name, inst.Caption)
        self.assertIsInstance(inst.Cost, pywbem.Sint32)
        self.assertIsInstance(inst.Description, basestring)
        self.assertEqual(repo.repoid, inst.ElementName)
        self.assertEqual(5, inst.EnabledDefault)
        self.assertEqual(2 if repo.status else 3, inst.EnabledState,
                "EnabledState does not match for repo %s" % repo.repoid)
        self.assertEqual(3, inst.ExtendedResourceType)
        if repo.revision is None:
            self.assertIsNone(inst.Generation)
        else:
            self.assertTrue(isinstance(inst.Generation, (int, long)))
        if repo.status:
            self.assertEqual(5, inst.HealthState)    # OK
            self.assertEqual([2], inst.OperationalStatus)    # OK
            self.assertEqual(1, inst.PrimaryStatus)  # OK
        else:
            # repo may not be enabled, but still it can be ready
            # (which is not documented), so let's check both posibilities
            self.assertIn(inst.HealthState, [5, 20])     # OK, Major Failure
            self.assertIn(inst.OperationalStatus, [[2], [6]])    # [OK], [Error]
            self.assertIn(inst.PrimaryStatus, [1, 3])    # OK, Error
        self.assertIsInstance(inst.GPGCheck, bool)
        self.assertEqual(200, inst.InfoFormat)
        self.assertEqual("LMI:LMI_SoftwareIdentityResource:"+repo.repoid,
                inst.InstanceID)
        if repo.mirror_list is None:
            self.assertIsNone(inst.MirrorList)
        else:
            self.assertEqual(
                    repo.metalink if repo.metalink else repo.mirror_list,
                    inst.MirrorList)
        self.assertIsInstance(inst.OtherAccessContext, basestring)
        #self.assertEqual(repo.pkg_count, inst.PackageCount,
                #"PackageCount does not match for repo %s" % repo.repoid)
        self.assertIsInstance(inst.RepoGPGCheck, bool)
        self.assertEqual(2 if repo.status else 3, inst.RequestedState)
        self.assertEqual(1, inst.ResourceType)
        self.assertIsInstance(inst.StatusDescriptions, list)
        self.assertEqual(1, len(inst.StatusDescriptions))
        if repo.last_updated is None:
            self.assertIsNone(inst.TimeOfLastUpdate)
        else:
            time_stamp = repo.last_updated.replace(
                    microsecond=0, tzinfo=pywbem.cim_types.MinutesFromUTC(0))
            self.assertGreaterEqual(inst.TimeOfLastUpdate.datetime, time_stamp)
        self.assertEqual(12, inst.TransitioningToState)

    def test_get_instance_safe(self):
        """
        Tests GetInstance call.
        """
        for repo in self.repodb:
            objpath = self.make_op(repo)
            inst = objpath.to_instance()
            self._check_repo_instance(repo, inst)

    @mark_dangerous
    def test_get_instance(self):
        """
        Dangerous test, making sure, that properties are set correctly
        for enabled and disabled repositories.
        """
        for repo in self.repodb:
            objpath = self.make_op(repo)
            self.assertIs(repository.is_repo_enabled(repo), repo.status)
            inst = objpath.to_instance()
            self.assertCIMNameEqual(objpath, inst.path)
            for key in self.KEYS:
                self.assertEqual(getattr(inst, key), getattr(inst.path, key))
            self.assertEqual(repo.repoid, inst.Name)
            self.assertEqual(2 if repo.status else 3, inst.EnabledState)
            self.assertEqual(2 if repo.status else 3, inst.RequestedState)
        for repo in self.repodb:
            objpath = self.make_op(repo)
            time.sleep(1) # to make sure, that change will be noticed
            repository.set_repo_enabled(repo, not repo.status)
            self.assertIs(repository.is_repo_enabled(repo), not repo.status)
            inst = objpath.to_instance()
            self.assertEqual(repo.repoid, inst.Name)
            self.assertEqual(3 if repo.status else 2, inst.EnabledState)
            self.assertEqual(3 if repo.status else 2, inst.RequestedState)

    def test_enum_instance_names(self):
        """
        Tests EnumInstanceNames call on all repositories.
        """
        inames = self.cim_class.instance_names()
        self.assertEqual(len(inames), len(self.repodb))
        repoids = set(r.repoid for r in self.repodb)
        for iname in inames:
            self.assertEqual(iname.namespace, 'root/cimv2')
            self.assertEqual(set(iname.key_properties()), set(self.KEYS))
            self.assertIn(iname.Name, repoids)
            objpath = self.make_op(iname.Name)
            self.assertCIMNameEqual(objpath, iname)

    def test_enum_instances(self):
        """
        Tests EnumInstances call on all repositories.
        """
        insts = self.cim_class.instances()
        self.assertGreater(len(insts), 0)
        repoids = dict((r.repoid, r) for r in self.repodb)
        for inst in insts:
            self.assertIn(inst.Name, repoids)
            self._check_repo_instance(repoids[inst.Name], inst)

    @enable_lmi_exceptions
    @mark_dangerous
    def test_request_state_change(self):
        """
        Tests InvokeMethod on RequestStateChange().
        """
        for repo in self.repodb:
            objpath = self.make_op(repo)
            self.assertIs(repository.is_repo_enabled(repo), repo.status)

            # change state of repository (enabled/disabled)
            req_state = (  Values.RequestStateChange.RequestedState.Disabled
                        if repo.status else
                           Values.RequestStateChange.RequestedState.Enabled)
            inst = objpath.to_instance()
            (res, oparms, _) = inst.RequestStateChange(RequestedState=req_state)
            self.assertEqual(0, res) #Completed with no error
            self.assertEqual(0, len(oparms))
            inst.refresh()
            self.assertEqual(inst.EnabledState, req_state)
            for pkg in self.dangerous_pkgs:
                if pkg.up_repo == repo:
                    pkg_iname = util.make_identity_path(pkg)
                    self.assertRaisesCIM(pywbem.CIM_ERR_NOT_FOUND,
                        objpath.to_instance)

            # revert back to original configuration
            req_state = (  Values.RequestStateChange.RequestedState.Enabled
                        if repo.status else
                           Values.RequestStateChange.RequestedState.Disabled)
            (res, oparms, _) = inst.RequestStateChange(RequestedState=req_state)
            self.assertEqual(0, res) #Completed with no error
            self.assertEqual(0, len(oparms))
            inst.refresh()
            self.assertEqual(inst.EnabledState, req_state)
            for pkg in self.dangerous_pkgs:
                if pkg.up_repo == repo:
                    pkg_iname = util.make_identity_path(pkg)
                    inst = pkg_iname.to_instance()
                    self.assertNotEqual(inst, None)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareIdentityResource)

if __name__ == '__main__':
    unittest.main()
