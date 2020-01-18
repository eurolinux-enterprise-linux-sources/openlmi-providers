#!/usr/bin/env python
#
# Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
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
# Authors: Jan Grec <jgrec@redhat.com>
#
"""
Unit tests for ``LMI_SoftwareInstallationService`` provider.
"""

import time

from lmi.test import CIMError
from lmi.test import unittest
from lmi.test import wbem
from lmi.test.lmibase import enable_lmi_exceptions
from lmi.test.lmibase import with_connection_timeout

import package
import swbase
import util

CHECK_SOFTWARE_IDENTITY_NOT_SUPPORTED = 1
COMMUNICATION_STATUS_NOT_AVAILABLE = 1
COMMUNICATION_STATUS_OK = 2
DETAILED_STATUS_NOT_AVAILABLE = 0
ENABLED_DEFAULT_ENABLED = 2
ENABLED_DEFAULT_NOT_APPLICABLE = 5
ENABLED_STATE_ENABLED = 2
ENABLED_STATE_NOT_APPLICABLE = 5
FIND_IDENTITY_FOUND = 0
FIND_IDENTITY_NO_MATCH = 1
HEALTH_STATE_OK = 5
INSTALL_METHOD_NOT_SUPPORTED = 1
INSTALL_OPTIONS_FORCE = 3
INSTALL_OPTIONS_INSTALL = 4
INSTALL_OPTIONS_UPDATE = 5
INSTALL_OPTIONS_REPAIR = 6
INSTALL_OPTIONS_UNINSTALL = 9
OPERATING_STATUS_SERVICING = 2
OPERATIONAL_STATUS_OK = 2
PRIMARY_STATUS_OK = 1
REQUESTED_STATE_NOT_APPLICABLE = 12
TRANSITIONING_TO_STATE_NOT_APPLICABLE = 12

JOB_COMPLETED_WITH_NO_ERROR = 0
UNSPECIFIED_ERROR = 2
INVALID_PARAMETER = 5
METHOD_PARAMETERS_CHECKED_JOB_STARTED = 4096

class TestSoftwareInstallationService(swbase.SwTestCase):
    """
    Basic cim operations test on ``LMI_SoftwareInstallationService``.
    """

    CLASS_NAME = "LMI_SoftwareInstallationService"
    KEYS = ("CreationClassName", "Name", "SystemCreationClassName",
            "SystemName")

    def make_op(self):
        """
        :returns: Object path of ``LMI_SoftwareInstallationService``
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        return self.cim_class.new_instance_name({
            "Name" : 'LMI:' + self.CLASS_NAME,
            "SystemName" : self.SYSTEM_NAME,
            "SystemCreationClassName" : self.system_cs_name,
            "CreationClassName" : self.CLASS_NAME
        })

    def test_get_instance(self):
        """
        Test ``GetInstance()`` call on ``LMI_SoftwareInstallationService``.
        """
        objpath = self.make_op()
        inst = objpath.to_instance()
        self.assertNotEqual(inst, None)
        self.assertCIMNameEqual(inst.path, objpath)
        for key_property in objpath.key_properties():
            if key_property == 'SystemName':
                self.assertIn(inst.SystemName, (self.SYSTEM_NAME, 'localhost'))
            else:
                self.assertEqual(getattr(inst, key_property),
                        getattr(objpath, key_property))
        self.assertGreater(len(inst.Caption), 0)
        self.assertEqual(inst.CommunicationStatus,
                COMMUNICATION_STATUS_NOT_AVAILABLE)
        self.assertGreater(len(inst.Description), 0)
        self.assertEqual(inst.DetailedStatus, DETAILED_STATUS_NOT_AVAILABLE)
        self.assertEqual(inst.EnabledDefault, ENABLED_DEFAULT_NOT_APPLICABLE)
        self.assertEqual(inst.EnabledState, ENABLED_STATE_NOT_APPLICABLE)
        self.assertEqual(inst.HealthState, HEALTH_STATE_OK)
        self.assertEqual(inst.InstanceID, "LMI:" + self.CLASS_NAME)
        self.assertEqual(inst.OperatingStatus, OPERATING_STATUS_SERVICING)
        self.assertEqual(inst.OperationalStatus, [OPERATIONAL_STATUS_OK])
        self.assertEqual(inst.PrimaryStatus, PRIMARY_STATUS_OK)
        self.assertEqual(inst.RequestedState, REQUESTED_STATE_NOT_APPLICABLE)
        self.assertTrue(inst.Started)
        self.assertEqual(inst.TransitioningToState, 12)

    def test_enum_instance_names(self):
        """
        Test ``EnumerateInstanceNames()`` call on
        ``LMI_SoftwareInstallationService``.
        """
        objpath = self.make_op()
        insts = self.cim_class.instance_names()
        self.assertEqual(len(insts), 1)
        inst = insts[0]
        self.assertCIMNameEqual(inst, objpath)

    def test_enum_instances(self):
        """
        Test ``EnumerateInstances()`` call on
        ``LMI_SoftwareInstallationService``.
        """
        objpath = self.make_op()
        insts = self.cim_class.instances()
        self.assertEqual(len(insts), 1)
        inst = insts[0]
        self.assertCIMNameEqual(inst.path, objpath)
        self.assertEqual(inst.InstanceID, "LMI:" + self.CLASS_NAME)

    @enable_lmi_exceptions
    def test_check_software_identity_method(self):
        """
        Try to invoke ``CheckSoftwareIdentity()`` method which shall not be
        supported.
        """
        service = self.make_op().to_instance()
        # TODO: This should either return NOT_SUPPORTED or
        # InstallFromByteStream() should raise the same.
        # This dissimilarity is really weird.
        try:
            rval, _, _ = service.CheckSoftwareIdentity()
            self.assertEqual(rval, INSTALL_METHOD_NOT_SUPPORTED)
        except CIMError as err:
            self.assertEqual(err.args[0], wbem.CIM_ERR_METHOD_NOT_AVAILABLE)

    @enable_lmi_exceptions
    def test_install_from_byte_stream_method(self):
        """
        Try to invoke ``InstallFromByteStram()`` method which shall not be
        supported.
        """
        service = self.make_op().to_instance()
        # TODO: This should either raise METHOD_NOT_AVAILABLE or
        # CheckSoftwareIdentity() should return NOT_SUPPORTED.
        # This dissimilarity is really weird.
        rval, _, _ = service.InstallFromByteStream()
        self.assertEqual(rval, INSTALL_METHOD_NOT_SUPPORTED)

    @swbase.test_with_repos(**{
        'stable' : True,
        'updates-testing' : True,
        'updates' : False,
        'misc' : False,
    })
    @swbase.test_with_packages(**{
        'stable#pkg1' : True,
        'pkg2' : False,
        'pkg3' : False,
        'pkg4' : False,
        'misc#*' : False,
    })
    def test_query_identities_by_name(self):
        """
        Try to find packages by their name.
        """
        service = self.make_op().to_instance()

        # allow duplicates is False
        rval, oparms, _ = service.FindIdentity(Name="openlmi-sw-test")
        self.assertEqual(rval, FIND_IDENTITY_FOUND)
        self.assertIn("Matches", oparms)
        inames = oparms["Matches"]
        # pkg1 is listed twice (installed and available version)
        # pkg2, pkg3 and pkg4 just once (the newest version)
        self.assertEqual(len(inames), 5)
        nevra_set = set()
        for iname in inames:
            self.assertEqual(iname.classname, 'LMI_SoftwareIdentity')
            self.assertEqual(iname.key_properties(), ['InstanceID'])
            self.assertTrue(iname.InstanceID.startswith(
                'LMI:LMI_SoftwareIdentity'))
            nevra_set.add(iname.InstanceID[len('LMI:LMI_SoftwareIdentity:'):])
        for repoid in ('stable', 'updates-testing'):
            for pkg in self.get_repo(repoid).packages:
                if pkg.name.endswith('pkg2') and repoid == 'stable':
                    # only pkg2 from updates-testing will be present (its newer)
                    continue
                self.assertIn(pkg.nevra, nevra_set)
                nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)

        rval, oparms, _ = service.FindIdentity(Name="openlmi-sw-test",
                AllowDuplicates=True)
        self.assertEqual(rval, FIND_IDENTITY_FOUND)
        inames = oparms["Matches"]
        self.assertEqual(len(inames), 6)
        nevra_set = set()
        for iname in inames:
            nevra_set.add(iname.InstanceID[len('LMI:LMI_SoftwareIdentity:'):])
        for repoid in ('stable', 'updates-testing'):
            for pkg in self.get_repo(repoid).packages:
                self.assertIn(pkg.nevra, nevra_set)
                nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)

    @swbase.test_with_repos('stable', 'updates', 'updates-testing', 'misc')
    def test_query_not_existing_package_by_name(self):
        """
        Try to find package by part of name with exact match.
        """
        service = self.make_op().to_instance()
        rval, oparms, _ = service.FindIdentity(Name="openlmi-sw-test",
                ExactMatch=True)
        self.assertEqual(rval, FIND_IDENTITY_NO_MATCH)
        self.assertIn("Matches", oparms)
        inames = oparms["Matches"]
        self.assertEqual(len(inames), 0)

    @swbase.test_with_repos('stable', 'updates', 'updates-testing', 'misc')
    @swbase.test_with_packages(**{'pkg2' : False})
    def test_query_identity_by_name_exact(self):
        """
        Try to find package by name with exact match.
        """
        service = self.make_op().to_instance()

        # allow duplicates is False
        rval, oparms, _ = service.FindIdentity(Name="openlmi-sw-test-pkg2",
                ExactMatch=True)
        self.assertEqual(rval, FIND_IDENTITY_FOUND)
        self.assertIn("Matches", oparms)
        inames = oparms["Matches"]
        # there are two different architectures available
        self.assertEqual(len(inames), 2)
        nevra_set = set(   i.InstanceID[len('LMI:LMI_SoftwareIdentity:'):]
                       for i in inames)
        self.assertIn(self.get_repo('updates-testing')['pkg2'].nevra, nevra_set)
        self.assertIn(self.get_repo('misc')['pkg2'].nevra, nevra_set)

        # allow duplicates is True
        rval, oparms, _ = service.FindIdentity(Name="openlmi-sw-test-pkg2",
                AllowDuplicates=True,
                ExactMatch=True)
        self.assertEqual(rval, FIND_IDENTITY_FOUND)
        self.assertIn("Matches", oparms)
        inames = oparms["Matches"]
        self.assertEqual(len(inames), 4)
        nevra_set = set(   i.InstanceID[len('LMI:LMI_SoftwareIdentity:'):]
                       for i in inames)
        for repo in self.repodb.values():
            pkg = repo['pkg2']
            self.assertIn(pkg.nevra, nevra_set)
            nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)


    @with_connection_timeout(240)
    @swbase.test_with_repos('stable', 'updates', 'updates-testing', 'misc')
    def test_query_identity_by_repoid(self):
        """
        Try to filter packages by repository.
        """
        service = self.make_op().to_instance()
        repo = self.get_repo('misc')
        repo_iname = self.ns.LMI_SoftwareIdentityResource.new_instance_name({
            "Name" : repo.repoid,
            "SystemName" : self.SYSTEM_NAME,
            "SystemCreationClassName" : self.system_cs_name,
            "CreationClassName" : "LMI_SoftwareIdentityResource"
        })
        rval, oparms, _ = service.FindIdentity(Repository=repo_iname)
        self.assertEqual(rval, FIND_IDENTITY_FOUND)
        self.assertIn("Matches", oparms)
        inames = oparms['Matches']
        self.assertEqual(len(inames), len(repo.packages))
        nevra_set = set(   i.InstanceID[len('LMI:LMI_SoftwareIdentity:'):]
                       for i in inames)
        for pkg in repo.packages:
            self.assertIn(pkg.nevra, nevra_set)
            nevra_set.remove(pkg.nevra)
        self.assertEqual(len(nevra_set), 0)

    @swbase.test_with_repos(**{'stable' : False, 'misc' : False})
    @swbase.test_with_packages('stable#pkg1')
    def test_query_installed_package_by_nevra(self):
        """
        Try to find installed, not available package by its nevra.
        """
        service = self.make_op().to_instance()
        pkg = self.get_repo('stable')['pkg1']
        rval, oparms, _ = service.FindIdentity(
                Name=pkg.name,
                Epoch=pkg.epoch,
                Version=pkg.ver,
                Release=pkg.rel,
                Architecture=pkg.arch,
                AllowDuplicates=True,
                ExactMatch=True)
        self.assertEqual(rval, FIND_IDENTITY_FOUND)
        self.assertIn("Matches", oparms)
        inames = oparms['Matches']
        self.assertEqual(len(inames), 1)
        self.assertTrue(inames[0].InstanceID.endswith(pkg.nevra))

    @swbase.test_with_repos('stable', 'updates', 'updates-testing', 'misc')
    def test_query_package_by_arch(self):
        """
        Try to find package filtered by arch.
        """
        service = self.make_op().to_instance()
        pkg = self.get_repo('misc')['pkg2']
        rval, oparms, _ = service.FindIdentity(
                Name='openlmi-sw-test',
                Architecture=pkg.arch,
                AllowDuplicates=True)
        self.assertEqual(rval, FIND_IDENTITY_FOUND)
        self.assertIn("Matches", oparms)
        inames = oparms['Matches']
        self.assertEqual(len(inames), 1)
        self.assertTrue(inames[0].InstanceID.endswith(pkg.nevra))

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages(**{ 'pkg1' : False })
    def test_install_package_sync(self):
        """
        Try to synchronously install package.
        """
        pkg = self.get_repo('stable')['pkg1']
        self.assertFalse(package.is_pkg_installed(pkg))
        service = self.make_op().to_instance()
        rval, _, _ = service.SyncInstallFromSoftwareIdentity(
                Source=util.make_pkg_op(self.ns, pkg),
                InstallOptions=[INSTALL_OPTIONS_INSTALL],
                Target=self.system_iname)
        self.assertEqual(rval, JOB_COMPLETED_WITH_NO_ERROR)
        self.assertTrue(package.is_pkg_installed(pkg))

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages('stable#pkg1')
    def test_remove_package_sync(self):
        """
        Try to synchronously remove package.
        """
        pkg = self.get_repo('stable')['pkg1']
        self.assertTrue(package.is_pkg_installed(pkg))
        service = self.make_op().to_instance()
        rval, _, _ = service.SyncInstallFromSoftwareIdentity(
                Source=util.make_pkg_op(self.ns, pkg),
                InstallOptions=[INSTALL_OPTIONS_UNINSTALL],
                Target=self.system_iname)
        self.assertEqual(rval, JOB_COMPLETED_WITH_NO_ERROR)
        self.assertFalse(package.is_pkg_installed(pkg))

    @swbase.test_with_repos('stable', 'updates')
    @swbase.test_with_packages('stable#pkg1')
    def test_update_package_sync(self):
        """
        Try to synchronously update package.
        """
        pkg = self.get_repo('stable')['pkg1']
        self.assertTrue(package.is_pkg_installed(pkg))
        service = self.make_op().to_instance()
        rval, _, _ = service.SyncInstallFromSoftwareIdentity(
                Source=util.make_pkg_op(self.ns, pkg),
                InstallOptions=[INSTALL_OPTIONS_UPDATE],
                Target=self.system_iname)
        self.assertEqual(rval, JOB_COMPLETED_WITH_NO_ERROR)
        self.assertFalse(package.is_pkg_installed(pkg))
        up_pkg = self.get_repo('updates')['pkg1']
        self.assertTrue(package.is_pkg_installed(up_pkg))

    @enable_lmi_exceptions
    @swbase.test_with_repos('stable', 'updates')
    @swbase.test_with_packages(**{'pkg1' : False})
    def test_update_not_installed_package_sync(self):
        """
        Try to synchronously update package which is not installed.
        """
        pkg = self.get_repo('stable')['pkg1']
        self.assertFalse(package.is_pkg_installed(pkg.name))
        service = self.make_op().to_instance()
        try:
            rval, _, error = service.SyncInstallFromSoftwareIdentity(
                    Source=util.make_pkg_op(self.ns, pkg),
                    InstallOptions=[INSTALL_OPTIONS_UPDATE],
                    Target=self.system_iname)
            self.assertTrue(rval in (INVALID_PARAMETER, None))
            self.assertGreater(len(error), 0)
            self.assertFalse(package.is_pkg_installed(pkg.name))
        except CIMError as err:
            # Once the lmishell supports exception throwing of failed asynchronous
            # jobs, this will apply.
            self.assertEqual(err.args[0], wbem.CIM_ERR_NOT_FOUND)

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages(**{ 'pkg1' : False })
    def test_install_method_sync_without_target_and_collection(self):
        """
        Try to synchronously install package without target and collection
        parameters given.

        Software Update profile says that ``InstallFromSoftwareIdentity``
        shall return ``UNSPECIFIED_ERROR``. Which is really weird since the
        error is known.
        """
        pkg = self.get_repo('stable')['pkg1']
        self.assertFalse(package.is_pkg_installed(pkg))
        service = self.make_op().to_instance()
        for opts in (
                [INSTALL_OPTIONS_INSTALL],
                [INSTALL_OPTIONS_UNINSTALL],
                [INSTALL_OPTIONS_UPDATE]):
            rval, _, _ = service.SyncInstallFromSoftwareIdentity(
                    Source=util.make_pkg_op(self.ns, pkg),
                    InstallOptions=opts)
            self.assertTrue(rval in (UNSPECIFIED_ERROR, INVALID_PARAMETER))

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages(**{ 'pkg1' : False })
    def test_install_package_sync_with_target_and_collection(self):
        """
        Try to synchronously install package with target and collection
        parameters omitted.

        Software Update profile says that ``InstallFromSoftwareIdentity``
        shall return ``UNSPECIFIED_ERROR``. Which is really weird since the
        error is known.
        """
        pkg = self.get_repo('stable')['pkg1']
        self.assertFalse(package.is_pkg_installed(pkg))
        service = self.make_op().to_instance()
        for opts in (
                [INSTALL_OPTIONS_INSTALL],
                [INSTALL_OPTIONS_UNINSTALL],
                [INSTALL_OPTIONS_UPDATE]):
            rval, _, _ = service.SyncInstallFromSoftwareIdentity(
                    Source=util.make_pkg_op(self.ns, pkg),
                    InstallOptions=opts,
                    Target=self.system_iname,
                    Collection=self.ns.LMI_SystemSoftwareCollection \
                        .new_instance_name({
                            "InstanceID" : "LMI:LMI_SystemSoftwareCollection"
                        }))
            self.assertTrue(rval in (UNSPECIFIED_ERROR, INVALID_PARAMETER))

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages(**{ 'pkg1' : False })
    def test_install_package_sync_with_collection(self):
        """
        Try to synchronously install package with just collection given.
        """
        pkg = self.get_repo('stable')['pkg1']
        self.assertFalse(package.is_pkg_installed(pkg))
        service = self.make_op().to_instance()
        rval, _, _ = service.SyncInstallFromSoftwareIdentity(
                Source=util.make_pkg_op(self.ns, pkg),
                InstallOptions=[INSTALL_OPTIONS_INSTALL],
                Collection=self.ns.LMI_SystemSoftwareCollection \
                        .new_instance_name({
                            "InstanceID" : "LMI:LMI_SystemSoftwareCollection"
                        }))
        self.assertEqual(rval, JOB_COMPLETED_WITH_NO_ERROR)
        self.assertTrue(package.is_pkg_installed(pkg))

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages('stable#pkg1')
    def test_reinstall_package(self):
        """
        Try to reinstall package.
        """
        pkg = self.get_repo('stable')['pkg1']
        pkg_op = util.make_pkg_op(self.ns, pkg)
        inst = pkg_op.to_instance()
        install_date = inst.InstallDate
        self.assertTrue(package.is_pkg_installed(pkg))

        service = self.make_op().to_instance()
        time.sleep(1)
        rval, _, error = service.SyncInstallFromSoftwareIdentity(
                Source=pkg_op,
                InstallOptions=[INSTALL_OPTIONS_INSTALL, INSTALL_OPTIONS_FORCE],
                Target=self.system_iname)
        if self.backend == "package-kit":
            # PackageKit doesn't support reinstallation and downgrades as of
            # now
            self.assertTrue(rval in (None, wbem.CIM_ERR_NOT_SUPPORTED))
            self.assertTrue(len(error) > 0)
        else:
            self.assertEqual(rval, JOB_COMPLETED_WITH_NO_ERROR)
            self.assertTrue(package.is_pkg_installed(pkg))
            inst.refresh()
            install_date2 = inst.InstallDate
            self.assertGreater(install_date2, install_date)

    @swbase.test_with_repos('updates', 'stable')
    @swbase.test_with_packages('updates#pkg2')
    def test_downgrade_package_without_force_flag(self):
        """
        Try to downgrade package.
        """
        new = self.get_repo('updates')['pkg2']
        old = self.get_repo('stable')['pkg2']
        pkg_old = util.make_pkg_op(self.ns, old)
        self.assertTrue(package.is_pkg_installed(new))
        self.assertFalse(package.is_pkg_installed(old))

        service = self.make_op().to_instance()
        rval, _, error = service.SyncInstallFromSoftwareIdentity(
                Source=pkg_old,
                InstallOptions=[INSTALL_OPTIONS_INSTALL],
                Target=self.system_iname)
        self.assertTrue(rval in (None, wbem.CIM_ERR_FAILED))
        self.assertTrue(len(error) > 0)

    @swbase.test_with_repos('updates', 'stable')
    @swbase.test_with_packages('updates#pkg2')
    def test_downgrade_package_with_force_flag(self):
        """
        Try to downgrade package.
        """
        new = self.get_repo('updates')['pkg2']
        old = self.get_repo('stable')['pkg2']
        pkg_old = util.make_pkg_op(self.ns, old)
        self.assertTrue(package.is_pkg_installed(new))
        self.assertFalse(package.is_pkg_installed(old))

        service = self.make_op().to_instance()
        rval, _, error = service.SyncInstallFromSoftwareIdentity(
                Source=pkg_old,
                InstallOptions=[INSTALL_OPTIONS_INSTALL, INSTALL_OPTIONS_FORCE],
                Target=self.system_iname)
        if self.backend == "package-kit":
            # PackageKit doesn't support reinstallation and downgrades as of
            # now
            self.assertTrue(rval in (None, wbem.CIM_ERR_FAILED))
            self.assertTrue(len(error) > 0)
        else:
            self.assertEqual(rval, JOB_COMPLETED_WITH_NO_ERROR)
            self.assertFalse(package.is_pkg_installed(new))
            self.assertTrue(package.is_pkg_installed(old))

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareInstallationService)

if __name__ == '__main__':
    unittest.main()
