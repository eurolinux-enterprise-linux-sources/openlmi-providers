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
Miscellaneous unit tests for OpenLMI Software provider.
"""

import os
import subprocess
import unittest

import swbase

BROKER = os.environ.get("LMI_CIMOM_BROKER", "tog-pegasus")
OPENLMI_SOFTWARE_PKG_NAME = 'openlmi-software'

class TestSoftwareProvider(swbase.SwTestCase):
    """
    Tests not related to particular class of OpenLMI Software profiles.
    """

    def _is_provider_ready(self):
        """
        Test whether software provider anwers requests.
        """
        service = self.ns.LMI_SoftwareInstallationService.first_instance()
        self.assertNotEqual(service, None,
                "software provider is up and running")
        insts = service.FindIdentity(Name=service, ExactMatch=True)
        self.assertGreater(len(insts), 0,
                "software provider correctly find kernel package")

    @unittest.skip("rhbz#1026270")
    def test_reinstall_broker_by_yum(self):
        """
        Test reinstallation of current broker and check everything still works.

        Get broker from BROKER global variable.
        Check afterwards that provider handles requests.
        """
        self.assertIn(BROKER, ["tog-pegasus", "sblim-sfcb"], "broker is known")
        subprocess.call(["/usr/bin/yum", "-y", "reinstall", BROKER])
        self._is_provider_ready()

    def test_reinstall_software_provider_by_yum(self):
        """
        Test reinstallation of ``openlmi-software`` package which
        provides software provider which is being tested.

        Check afterwards that provider handles requests.
        """
        subprocess.call(["/usr/bin/yum", "-y", "reinstall", "openlmi-software"])
        self._is_provider_ready()

