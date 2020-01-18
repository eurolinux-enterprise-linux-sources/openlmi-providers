# OpenLMI tests
#
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
# Authors: Jan Safranek <jsafrane@redhat.com>
#

from lmi.test.lmibase import LmiTestCase
import lmi.shell
import unittest
import pywbem
import traceback
import sys

class TestInstances(LmiTestCase):
    """
    Test that EnumerateInstances, EnumerateInstanceNames, GetInstances,
    Associators and References works on all LMI_* classes.

    It does not check actual content of the instances, just that the above
    methods do not crash.
    """
    SKIP_CLASSES = [
            # Enumeration is not implemented (and never will be):
            "LMI_SoftwareIdentityFileCheck",
            "LMI_FIFOPipeFile",
            "LMI_DataFile",
            "LMI_FileIdentity",
            "LMI_UnixDirectory",
            "LMI_UnixFile",
            "LMI_UnixDeviceFile",
            "LMI_DirectoryContainsFile",
            "LMI_SymbolicLink",
            "LMI_UnixSocket",
            "LMI_SoftwareInstModification",
            "LMI_StorageInstModification",
            "LMI_SoftwareIdentity",
            "LMI_SoftwareIdentityChecks",
            "LMI_SoftwareInstCreation",
            "LMI_StorageInstCreation",
            "LMI_AccountInstanceDeletionIndication",
            "LMI_NetworkInstCreation",
            "LMI_NetworkInstModification",
            "LMI_NetworkInstDeletion",
            "LMI_JournalLogRecordInstanceCreationIndication",
            "LMI_ServiceInstanceModificationIndication",
            "LMI_AccountInstanceCreationIndication",

            # too much memory needed, TODO?
            "LMI_ResourceForSoftwareIdentity",
            "LMI_MemberOfSoftwareCollection",
            "LMI_SoftwareInstallationServiceAffectsElement",
    ]

    SKIP_ASSOCIATORS = [
            # too many associators(), too much memory needed:
            "LMI_SoftwareIdentityResource",
            "LMI_InstalledSoftwareIdentity",
            "LMI_SoftwareInstallationService",
            "LMI_SystemSoftwareCollection",
    ]

    def compare_paths(self, instance_names, instances):
        """
        Compare list of LMIInstanceNames and LMIInstances.
        Raise assertion error if these two lists do not represent the same
        CIM instances.
        """
        # Check every instance has appropriate instance_name
        for i in instances:
            path = i.path
            self.assertCIMNameIn(path, instance_names)

        # The other way around - check every instance_name has appropriate
        # instance
        instances_paths = [i.path for i in instances]
        for i in instance_names:
            self.assertCIMNameIn(i, instances_paths)

        # Check that there are no duplicates in instance_names
        # (this automatically ensures there are no duplicates in instances)
        inames_set = set(instance_names)
        self.assertEqual(len(inames_set), len(instance_names))

        # check, that instance key properties match their counterparts in path
        for i in instances:
            for key in i.path.key_properties():
                self.assertEqual(getattr(i, key), getattr(i.path, key))

    def check_instances(self, cls, skip_associators):
        """
        Check that EnumerateInstances and EnumerateInstanceNames work and
        return the same objects.
        """
        instances = cls.instances()
        instance_names = cls.instance_names()

        # Check that EnumerateInstances and EnumerateInstanceNames returns the
        # same instances
        self.compare_paths(instance_names, instances)

        # try GetInstance on each name, just to check it works
        for i in instance_names:
            i.to_instance()

        if skip_associators:
            return

        element_class = self.conn.root.cimv2.CIM_ManagedElement
        for instance in instances:
            if not lmi.shell.LMIUtil.lmi_isinstance(instance, element_class):
                # skip this class, it cannot have associations
                return
            # call Associators & AssociatorNames
            associators = instance.associators()
            associator_names = instance.associator_names()

            # Check that Associators and AssociatorNames returns the
            # same instances
            self.compare_paths(associator_names, associators)

            # call References & ReferenceNames
            references = instance.references()
            reference_names = instance.reference_names()

            # Check that References and ReferenceNames returns the
            # same instances
            self.compare_paths(reference_names, references)


    def test_all(self):
        """
        Enumerate all classes and start tests for all LMI_ ones.
        """
        lmi.shell.LMIUtil.lmi_set_use_exceptions(True)
        classes = self.conn.root.cimv2.classes()
        # do not stop on the first exception, scan all classes
        exception_count = 0

        for name in classes:
            try:
                if not name.startswith("LMI_"):
                    continue
                if name in self.SKIP_CLASSES:
                    print >>sys.stderr, "Skipping", name
                    continue
                skip_associators = name in self.SKIP_ASSOCIATORS
                print >>sys.stderr, "Checking ", name
                cls = lmi.shell.LMIClass(self.conn, self.conn.root.cimv2, name)
                self.check_instances(cls, skip_associators)
            except Exception, exc:
                print >>sys.stderr, "ERROR scanning class", cls, exc
                traceback.print_exc()
                exception_count += 1

        self.assertEqual(exception_count, 0)

if __name__ == '__main__':
    # Run the standard unittest.main() function to load and run all tests
    unittest.main()
