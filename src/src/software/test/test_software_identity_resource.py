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
# Authors: Jan Grec <jgrec@redhat.com>
# Authors: Michal Minar <miminar@redhat.com>
#
"""
Unit tests for ``LMI_SoftwareIdentityResource`` provider.
"""

import os
import pywbem
import time
import shutil
import subprocess
import unittest

from lmi.test.lmibase import enable_lmi_exceptions
import repository
import swbase
import util

ENABLED_DEFAULT_NOT_APPLICABLE = 5
ENABLED_STATE_DISABLED = 3
ENABLED_STATE_ENABLED = 2
EXTENDED_RESOURCE_TYPE_RPM = 3
HEALTH_STATE_MAJOR_FAILURE = 20
HEALTH_STATE_OK = 5
INFO_FORMAT_URL = 200
OPERATIONAL_STATUS_ERROR = 6
OPERATIONAL_STATUS_OK = 2
REQUESTED_STATE_ENABLED = 2
REQUESTED_STATE_DISABLED = 3
REQUEST_STATE_CHANGE_SUCCESSFUL = 0
REQUEST_STATE_CHANGE_ERROR = 2
RESOURCE_TYPE_OTHER = 1
PRIMARY_STATUS_ERROR = 3
PRIMARY_STATUS_OK = 1
TRANSITIONING_TO_STATE_NOT_APPLICABLE = 12

DUMMY_TEST_REPO = 'lmi-test'

class TestSoftwareIdentityResource(swbase.SwTestCase):
    """
    Basic cim operations test on ``LMI_SoftwareIdentityResource``.
    """

    CLASS_NAME = "LMI_SoftwareIdentityResource"
    KEYS = ("CreationClassName", "Name", "SystemCreationClassName",
            "SystemName")

    def make_op(self, repo):
        """
        :returns: Object path of ``LMI_SoftwareIdentityResource``
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        if isinstance(repo, repository.Repository):
            repo = repo.repoid
        return self.cim_class.new_instance_name({
            "Name" : repo,
            "SystemName" : self.SYSTEM_NAME,
            "SystemCreationClassName" : self.system_cs_name,
            "CreationClassName" : self.CLASS_NAME
        })

    def _check_repo_instance(self, repo, inst):
        """
        Checks instance properties of repository.
        """
        objpath = self.make_op(repo)
        self.assertCIMNameEqual(objpath, inst.path)
        for key in self.KEYS:
            if key == 'SystemName':
                self.assertIn(inst.SystemName, (self.SYSTEM_NAME, 'localhost'))
                self.assertIn(inst.path.SystemName, (self.SYSTEM_NAME, 'localhost'))
            else:
                self.assertEqual(getattr(inst, key), getattr(inst.path, key))
        self.assertEqual(inst.Name, repo.repoid)

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
        # Unfortunately, yum-config-manager shortens lines on its output
        # therefor we can not check very long names.
        # 61 characters in version yum-utils-1.1.31-19.fc21
        if len(repo.name) > 60:
            self.assertTrue(inst.Caption.startswith(repo.name))
        else:
            self.assertEqual(inst.Caption, repo.name)
        self.assertIsInstance(inst.Cost, pywbem.Sint32)
        self.assertIsInstance(inst.Description, basestring)
        self.assertEqual(inst.ElementName, repo.repoid)
        self.assertEqual(inst.EnabledDefault, ENABLED_DEFAULT_NOT_APPLICABLE)
        self.assertEqual(
                       ENABLED_STATE_ENABLED
                    if repo.status else ENABLED_STATE_DISABLED,
                inst.EnabledState,
                "EnabledState does not match for repo %s" % repo.repoid)
        self.assertEqual(inst.ExtendedResourceType, EXTENDED_RESOURCE_TYPE_RPM)
        if repo.revision is not None:
            self.assertTrue(isinstance(inst.Generation, (int, long)),
                    'Generation must be integer for repo %s' % repo.repoid)
        if repo.status:
            self.assertEqual(inst.HealthState, HEALTH_STATE_OK)
            self.assertEqual(inst.OperationalStatus, [OPERATIONAL_STATUS_OK])
            self.assertEqual(inst.PrimaryStatus, PRIMARY_STATUS_OK)
        else:
            # repo may not be enabled, but still it can be ready
            # (which is not documented), so let's check both posibilities
            # TODO: health state should be OK for disabled repository
            self.assertIn(inst.HealthState,
                    [HEALTH_STATE_OK, HEALTH_STATE_MAJOR_FAILURE])
            self.assertIn(inst.OperationalStatus,
                    [[OPERATIONAL_STATUS_OK], [OPERATIONAL_STATUS_ERROR]])
            self.assertIn(inst.PrimaryStatus, [PRIMARY_STATUS_OK, PRIMARY_STATUS_ERROR])
        self.assertIsInstance(inst.GPGCheck, bool)
        self.assertEqual(inst.InfoFormat, INFO_FORMAT_URL)
        self.assertEqual("LMI:LMI_SoftwareIdentityResource:"+repo.repoid,
                inst.InstanceID)
        self.assertEqual(inst.MirrorList, repo.mirror_list,
                'MirrorList must match for repo %s' % repo.repoid)
        self.assertIsInstance(inst.OtherAccessContext, basestring)
        self.assertIsInstance(inst.RepoGPGCheck, bool)
        self.assertEqual(
                   ENABLED_STATE_ENABLED
                if repo.status else ENABLED_STATE_DISABLED,
                inst.RequestedState)
        self.assertEqual(inst.ResourceType, RESOURCE_TYPE_OTHER)
        self.assertIsInstance(inst.StatusDescriptions, list)
        self.assertEqual(1, len(inst.StatusDescriptions))
        if repo.last_updated is not None:
            time_stamp = repo.last_updated.replace(
                    microsecond=0, tzinfo=pywbem.cim_types.MinutesFromUTC(0))
            self.assertGreaterEqual(inst.TimeOfLastUpdate.datetime, time_stamp)
        self.assertEqual(TRANSITIONING_TO_STATE_NOT_APPLICABLE,
                inst.TransitioningToState)

    @swbase.test_with_repos(stable=True, updates=False)
    def test_get_instance(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareIdentityResource``.
        """
        stablerepo = self.get_repo('stable')
        self.assertTrue(stablerepo.status, "stable repository is enabled")
        objpath = self.make_op(stablerepo)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None,
                "GetInstance() call on stable repository is successful"
                ' with path: "%s"' % str(objpath))
        self._check_repo_instance(stablerepo, inst)
        self.assertEqual(set(self.KEYS), set(inst.path.key_properties()))

        updaterepo = self.get_repo('updates')
        objpath = self.make_op(updaterepo)
        self.assertFalse(updaterepo.status, "updates repository is disabled")
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None,
                'GetInstance() call on updates repository is successful'
                ' with path "%s"' % str(objpath))
        self._check_repo_instance(updaterepo, inst)
        self.assertEqual(set(self.KEYS), set(inst.path.key_properties()))

    def test_enum_instance_names(self):
        """
        Test ``EnumInstanceNames()`` call on ``LMI_SoftwareIdentityResource``.
        """
        all_repo_names = set(self.repodb.keys()).union(
                set(self.other_repos.keys()))
        inames = self.cim_class.instance_names()
        for iname in inames:
            self.assertEqual(set(iname.key_properties()), set(self.KEYS))
            self.assertIn(iname.Name, all_repo_names)
            all_repo_names.remove(iname.Name)
            self.assertEqual(iname.SystemCreationClassName, self.system_cs_name)
            self.assertEqual(iname.CreationClassName, self.CLASS_NAME)
        self.assertEqual(len(all_repo_names), 0)

    def test_enum_instances(self):
        """
        Test ``EnumInstances()`` call on ``LMI_SoftwareIdentityResource``.
        """
        all_repo_names = set(self.repodb.keys()).union(
                set(self.other_repos.keys()))
        insts = self.cim_class.instances()
        for inst in insts:
            self.assertEqual(set(inst.path.key_properties()), set(self.KEYS))
            self.assertIn(inst.Name, all_repo_names)
            all_repo_names.remove(inst.Name)
            if inst.Name in self.repodb:
                repo = self.repodb[inst.Name]
            else:
                repo = self.other_repos[inst.Name]
            self._check_repo_instance(repo, inst)
        self.assertEqual(len(all_repo_names), 0)

    @swbase.test_with_repos('updates')
    def test_disable_repo(self):
        """
        Test whether ``LMI_SoftwareIdentityResource`` provider recognizes
        that repository has been disabled.
        """
        repo = self.get_repo('updates')
        self.assertTrue(repo.status)
        objpath = self.make_op(repo)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None,
                'GetInstance() call on updates repository is successful'
                ' with path "%s"' % str(objpath))
        self.assertEqual(inst.EnabledState, ENABLED_STATE_ENABLED)
        self.assertEqual(inst.HealthState, HEALTH_STATE_OK)
        self.assertEqual(inst.OperationalStatus, [OPERATIONAL_STATUS_OK])
        self.assertEqual(inst.PrimaryStatus, PRIMARY_STATUS_OK)

        repository.set_repos_enabled(repo, False)
        repo.refresh()
        self.assertFalse(repo.status)
        time.sleep(0.1)
        inst.refresh()
        self.assertNotEqual(inst, None,
                'GetInstance() call on updates repository is successful'
                ' with path "%s"' % str(objpath))
        self.assertEqual(inst.EnabledState, ENABLED_STATE_DISABLED)
        self.assertEqual(inst.HealthState, HEALTH_STATE_OK)
        self.assertEqual(inst.OperationalStatus, [OPERATIONAL_STATUS_OK])
        self.assertEqual(inst.PrimaryStatus, PRIMARY_STATUS_OK)


    @swbase.test_with_repos(updates=False)
    def test_enable_repo(self):
        """
        Test whether ``LMI_SoftwareIdentityResource`` provider recognizes that
        repository has been enabled.
        """
        repo = self.get_repo('updates')
        self.assertFalse(repo.status)
        objpath = self.make_op(repo)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None,
                'GetInstance() call on updates repository is successful'
                ' with path "%s"' % str(objpath))
        self.assertEqual(inst.EnabledState, ENABLED_STATE_DISABLED)
        self.assertEqual(inst.HealthState, HEALTH_STATE_OK)
        self.assertEqual(inst.OperationalStatus, [OPERATIONAL_STATUS_OK])
        self.assertEqual(inst.PrimaryStatus, PRIMARY_STATUS_OK)

        repository.set_repos_enabled(repo, True)
        repo.refresh()
        self.assertTrue(repo.status)
        inst.refresh()
        self.assertNotEqual(inst, None,
                'GetInstance() call on updates repository is successful'
                ' with path "%s"' % str(objpath))
        self.assertEqual(inst.EnabledState, ENABLED_STATE_ENABLED)
        self.assertEqual(inst.HealthState, HEALTH_STATE_OK)
        self.assertEqual(inst.OperationalStatus, [OPERATIONAL_STATUS_OK])
        self.assertEqual(inst.PrimaryStatus, PRIMARY_STATUS_OK)

    @swbase.test_with_repos('stable')
    def test_disable_enabled_repo(self):
        """
        Test ``RequestStateChange()`` method invocation on
        ``LMI_SoftwareIdentityResource`` by requesting disablement of
        repository.
        """
        repo = self.get_repo('stable')
        self.assertTrue(repo.status)
        objpath = self.make_op(repo)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None,
                'GetInstance() call on stable repository is successful'
                ' with path "%s"' % str(objpath))
        self.assertEqual(inst.EnabledState, ENABLED_STATE_ENABLED)

        # try to disable it with method invocation
        (rval, oparms, _) = inst.RequestStateChange(
                RequestedState=REQUESTED_STATE_DISABLED)
        self.assertEqual(rval, REQUEST_STATE_CHANGE_SUCCESSFUL)
        self.assertEqual(len(oparms), 0)
        repo.refresh()
        self.assertFalse(repo.status)
        inst.refresh()
        self.assertEqual(inst.EnabledState, ENABLED_STATE_DISABLED)
        self.assertEqual(inst.HealthState, HEALTH_STATE_OK)
        self.assertEqual(inst.OperationalStatus, [OPERATIONAL_STATUS_OK])
        self.assertEqual(inst.PrimaryStatus, PRIMARY_STATUS_OK)

    @swbase.test_with_repos(stable=False)
    def test_enable_disabled_repo(self):
        """
        Test ``RequestStateChange()`` method invocation on
        ``LMI_SoftwareIdentityResource`` by requesting enablement of
        repository.
        """
        repo = self.get_repo('stable')
        self.assertFalse(repo.status)
        objpath = self.make_op(repo)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None,
                'GetInstance() call on stable repository is successful'
                ' with path "%s"' % str(objpath))
        self.assertEqual(inst.EnabledState, ENABLED_STATE_DISABLED)

        # try to enable it with method invocation
        (rval, oparms, _) = inst.RequestStateChange(
                RequestedState=REQUESTED_STATE_ENABLED)
        self.assertEqual(rval, REQUEST_STATE_CHANGE_SUCCESSFUL)
        self.assertEqual(len(oparms), 0)
        repo.refresh()
        self.assertTrue(repo.status)
        inst.refresh()
        self.assertEqual(inst.EnabledState, ENABLED_STATE_ENABLED)
        self.assertEqual(inst.HealthState, HEALTH_STATE_OK)
        self.assertEqual(inst.OperationalStatus, [OPERATIONAL_STATUS_OK])
        self.assertEqual(inst.PrimaryStatus, PRIMARY_STATUS_OK)

    @swbase.test_with_repos('stable')
    def test_enable_enabled_repo(self):
        """
        Test ``RequestStateChange()`` method invocation on
        ``LMI_SoftwareIdentityResource`` by requesting enablement of
        enabled repository.
        """
        repo = self.get_repo('stable')
        self.assertTrue(repo.status)
        objpath = self.make_op(repo)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None,
                'GetInstance() call on stable repository is successful'
                ' with path "%s"' % str(objpath))
        self.assertEqual(inst.EnabledState, ENABLED_STATE_ENABLED)

        # try to enable it with method invocation
        (rval, oparms, _) = inst.RequestStateChange(
                RequestedState=REQUESTED_STATE_ENABLED)
        self.assertEqual(rval, REQUEST_STATE_CHANGE_SUCCESSFUL)
        self.assertEqual(len(oparms), 0)
        repo.refresh()
        self.assertTrue(repo.status)
        inst.refresh()
        self.assertEqual(inst.EnabledState, ENABLED_STATE_ENABLED)
        self.assertEqual(inst.HealthState, HEALTH_STATE_OK)
        self.assertEqual(inst.OperationalStatus, [OPERATIONAL_STATUS_OK])
        self.assertEqual(inst.PrimaryStatus, PRIMARY_STATUS_OK)

    @swbase.test_with_repos(stable=False)
    def test_disable_disabled_repo(self):
        """
        Test ``RequestStateChange()`` method invocation on
        ``LMI_SoftwareIdentityResource`` by requesting disablement of
        disabled repository.
        """
        repo = self.get_repo('stable')
        self.assertFalse(repo.status)
        objpath = self.make_op(repo)
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None,
                'GetInstance() call on stable repository is successful'
                ' with path "%s"' % str(objpath))
        self.assertEqual(inst.EnabledState, ENABLED_STATE_DISABLED)

        # try to enable it with method invocation
        (rval, oparms, _) = inst.RequestStateChange(
                RequestedState=REQUESTED_STATE_DISABLED)
        self.assertEqual(rval, REQUEST_STATE_CHANGE_SUCCESSFUL)
        self.assertEqual(len(oparms), 0)
        repo.refresh()
        self.assertFalse(repo.status)
        inst.refresh()
        self.assertEqual(inst.EnabledState, ENABLED_STATE_DISABLED)
        self.assertEqual(inst.HealthState, HEALTH_STATE_OK)
        self.assertEqual(inst.OperationalStatus, [OPERATIONAL_STATUS_OK])
        self.assertEqual(inst.PrimaryStatus, PRIMARY_STATUS_OK)

    @enable_lmi_exceptions
    def test_disable_unexisting_repository(self):
        """
        Test disable unexisting repository.

        Enable DUMMY_TEST_REPO repository, get its instance, delete it
        from /etc/yum.repos.d and try to disable it by OpenLMI.
        Cases:
            OpenLMI won't disable repository - returns error in LMIReturnValue
        """
        repo_file_path = os.path.join(self.yum_repos_dir,
                DUMMY_TEST_REPO + ".repo")
        shutil.copy2(
                "%s/%s.repo" % (
                    os.path.dirname(swbase.__file__),
                    DUMMY_TEST_REPO),
                repo_file_path)
        try:
            repository.set_repos_enabled(DUMMY_TEST_REPO, True)
            subprocess.call(["/usr/bin/yum-config-manager",
                "--enable", DUMMY_TEST_REPO],
                stdout=util.DEV_NULL, stderr=util.DEV_NULL)
            repo = self.cim_class.first_instance_name(
                    {"Name": DUMMY_TEST_REPO}).to_instance()
            self.assertNotEqual(repo, None, "dummy repository exists")

            os.remove(repo_file_path)

            self.assertRaisesCIM(pywbem.CIM_ERR_NOT_FOUND,
                    repo.RequestStateChange,
                    RequestedState=REQUESTED_STATE_DISABLED)
        finally:
            if os.path.exists(repo_file_path):
                os.remove(repo_file_path)

    @enable_lmi_exceptions
    def test_enable_unexisting_repository(self):
        """
        Test enable deleted repository.

        Get DUMMY_TEST_REPO's instance, delete it from /etc/yum.repos.d
        and try to enable it by OpenLMI.
        Cases:
            OpenLMI won't enable repository - returns error in LMIReturnValue
        """
        repo_file_path = os.path.join(self.yum_repos_dir,
                DUMMY_TEST_REPO + ".repo")
        shutil.copy2(
                "%s/%s.repo" % (
                    os.path.dirname(swbase.__file__),
                    DUMMY_TEST_REPO),
                repo_file_path)
        try:

            repo = self.cim_class.first_instance_name(
                    {"Name": DUMMY_TEST_REPO}).to_instance()

            os.remove(repo_file_path)

            self.assertRaisesCIM(pywbem.CIM_ERR_NOT_FOUND,
                    repo.RequestStateChange,
                    RequestedState=REQUESTED_STATE_ENABLED)
        finally:
            if os.path.exists(repo_file_path):
                os.remove(repo_file_path)

def suite():
    """ For unittest loaders. """
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareIdentityResource)

if __name__ == '__main__':
    unittest.main()
