# -*- encoding: utf-8 -*-
# Copyright(C) 2012-2014 Red Hat, Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or(at your option) any later version.
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
# Authors: Robin Hack <rhack@redhat.com>; Jan Grec <jgrec@redhat.com>; Jan Safranek <jsafrane@redhat.com>
#

from lmi.test import unittest
import subprocess

from lmi.test import CIMError
from lmi.test import lmibase
import os
import shutil
import time

DEV_NULL = open("/dev/null", 'w')

# NOTES
# - systemd/service status returns code 3 for stopped (dead) services on Fedora

class TestServiceProvider(lmibase.LmiTestCase):
    """
    TestCase class testing OpenLMI service functionality.
    """

    CLASS_NAME = "LMI_Service"

    def test_list_services(self):
        """
        Check all services have LMI_Service instance with correct status.

        Compare all LMI_Services with output of:
            systemctl list-unit-files -t service
            systemctl status
        Cases:
            LMI_Service.instances() returns proper list of services
            service.Started returns correct value for each service against systemctl
        """
        # Get list of services from OpenLMI service provider
        lmi_services_insts = self.cim_class.instances()
        self.assertTrue(lmi_services_insts, "No services returned")

        lmi_services = dict((s.Name, s.Started) for (s) in lmi_services_insts)

        # Get list of services from systemd
        sysd_output = subprocess.check_output("systemctl list-unit-files -t service --no-pager --no-legend".split())

        #  Parse list of services. Filter blank lines and services with @.service string in name.
        sysd_services = []
        for line in sysd_output.split("\n"):
            service = line.split()
            if service and "@.service" not in service[0]:
                sysd_services.append(service)

        sysd_services = dict((s[0], False) for (s) in sysd_services)

        # Compare both lists (systemd list and OpenLMI list)
        self.assertEqual(set(sysd_services.keys()), set(lmi_services.keys()))

        # For each service check its OpenLMI status against systemd
        for service_name in sysd_services.keys():

            cmd = "systemctl status " + service_name
            ret = subprocess.call(cmd.split());

            sysd_state = (ret == 0)
            lmi_state = False
            try:
                    lmi_state = lmi_services[service_name]
            except KeyError:
                    self.fail("Service " + service_name + " is not in list of services returned by OpenLMI")

            self.assertEqual(sysd_state, lmi_state, "Different states of " + service_name)


    def test_service_up_down(self):
        """
        Test that LMI_Service instance properly stops and starts service.

        Use 'cups' service for testing.
        Compare results with:
            service cups status
        Cases:
            stopped service can be started
            running service can be stopped
        """
        # Make sure the service is not running
        subprocess.call("service cups stop".split())
        service_status_ret = subprocess.call("service cups status".split())
        self.assertEqual(service_status_ret, 3)

        service = self.cim_class.first_instance({"Name": "cups.service"})

        self.assertFalse(service.Started, "Service is not in stopped state")

        # Try to start up the service and check it's running
        service_status_stored = service.Started
        service.StartService()
        service.refresh()
        self.assertTrue(service.Started, "Service is not started")

        service_status_ret = subprocess.call("service cups status".split(), stdout=DEV_NULL, stderr=DEV_NULL)
        self.assertEqual(service_status_ret, 0)

        self.assertNotEqual(service_status_stored, service.Started)

        # Assuming the service is running from pervious code
        # Try to stop the service and check it's stopped
        service_status_stored = service.Started
        service.StopService()
        service.refresh()
        service_status_ret = subprocess.call("service cups status".split())

        self.assertFalse(service.Started)
        self.assertEqual(service_status_ret, 3)
        self.assertNotEqual(service_status_stored, service.Started)


    def test_stopped_service_restart(self):
        """
        Test that LMI_Service instance starts stopped service on restart.

        Use 'cups' service for testing.
        Compare results with:
            pidof cupsd - pid has changed
            service cups status - systemctl confirms OpenLMI status
        Cases:
            stopped service is running after restart under another pid
        """
        # Make sure the service is not running
        service_status = subprocess.call("service cups stop".split())
        self.assertEqual(service_status, 0, "Service is not stopped or is in unknown state")

        service_pid_before = 0

        # Try to start up the service by using RestartService()
        service = self.cim_class.first_instance({"Name": "cups.service"})
        service.RestartService()
        service.refresh()
        self.assertTrue(service.Started, "Service couldn't be started BACK after RestartService() [OpenLMI]")

        # Check that pid of the service has changed - service was really
        # physically stopped and started again
        service_pid = subprocess.check_output("pidof cupsd".split())
        self.assertNotEqual(service_pid_before, service_pid, "Pids of cupsd service are same as before restart.")

        service_status = subprocess.call("service cups status".split(), stdout=DEV_NULL, stderr=DEV_NULL)
        self.assertEqual(service_status, 0, "Service couldn't be started BACK after RestartService() [systemctl]")


    def test_started_service_restart(self):
        """
        Test that LMI_Service instance properly restarts running service.

        Use 'cups' service for testing.
        Compare results with:
            pidof cupsd - pid has changed
            service cups status - systemctl confirms OpenLMI status
        Cases:
            running service is running again under different pid after restart
        """
        # Make sure the service is running
        service_status = subprocess.call("service cups start".split())
        self.assertEqual(service_status, 0, "Service is not started or is in unknown state")
        service_pid_before = subprocess.check_output("pidof cupsd".split())

        # Try to restart the service by using RestartService()
        service = self.cim_class.first_instance({"Name": "cups.service"})
        service.RestartService()

        self.assertTrue(service.Started, "Service couldn't be started UP after RestartService() [OpenLMI]")

        # Check that pid of the service has changed - service was really
        # physically stopped and started again
        service_pid = subprocess.check_output("pidof cupsd".split())
        self.assertNotEqual(service_pid_before, service_pid, "Pids of cupsd service are same as before restart.")

        service_status = subprocess.call("service cups status".split(), stdout=DEV_NULL, stderr=DEV_NULL)
        self.assertEqual(service_status, 0, "Service couldn't be started UP after RestartService() [service]")


    def test_service_nonexist(self):
        """
        Test that LMI_Service can't initialize unexisting service.

        Use '..!non_exists">"' service for testing
        Compare results with:
            None - the only value that can be returned
        Cases:
            unexisting service can't be initialized (returned from LMI_Service)
        """

        # Make sure that ..!non_exists">" service really doesn't exist
        service_evil_name = "..!non_exists\">\""
        cmd = "service " + service_evil_name + " status"
        cmd = cmd.split()
        service_status = subprocess.call(cmd)
        # on RHEL6 and older /bin/service returns '1'
        # on RHEL7 (=systemd) it returns '3'
        self.assertIn(service_status, (1,3))

        # Try to get ..!non_exists">" service from OpenLMI
        service = self.cim_class.first_instance({"Name": service_evil_name + ".service"})
        self.assertEqual(service, None, "Non None object returned of non-existing service")

    def test_restart_service_broker(self):
        """
        Test if is possible to restart broker over OpenLMI
        """
        service = self.cim_class.first_instance({"Name": self.cimom + ".service"})
        service.RestartService()

        # Wait up to 10 seconds for the CIMOM to start
        timer = 10
        while timer > 0:
            time.sleep(1)
            try:
                service = self.cim_class.first_instance({"Name": self.cimom + ".service"})
                if service is not None:
                    break
            except CIMError:
                pass
            timer -= 1
        self.assertGreater(timer, 0, "Timeout while waiting for the cimom to restart.")

        self.assertTrue (service.Started)

        # now try to cumunicate with broker
        servicecups = self.cim_class.first_instance({"Name": "cups.service"})
        self.assertNotEqual(servicecups, None)
        servicecups.RestartService()
        servicecups.refresh()
        self.assertTrue (servicecups.Started)

    @unittest.skip("This tests breaks Pegasus.")
    def test_service_with_null_name(self):
        """
        Test that LMI_Service can't initialize "empty" service.

        Use '\0' (null character) service for testing
        Compare results with:
            None - the only value that can be returned
        Cases:
            the "null" service can't be initialized (returned from LMI_Service)
        """
        service = self.cim_class.first_instance({"Name": "\0"})
        self.assertEqual(service, None, "Non None object returned of non-existing service")


    def test_service_stopped_try_restart(self):
        """
        Test LMI_Service.TryRestartService() on stopped service.

        Use 'cups' service for testing.
        Compare results with:
            service cups status - systemctl confirms OpenLMI status
        Cases:
            stopped service is not running after TryRestartService
        """
        # Make sure the service is not running
        service_status = subprocess.call("service cups stop".split())
        self.assertEqual(service_status, 0, "Service is not stopped or is in unknown state")

        service = self.cim_class.first_instance({"Name": "cups.service"})
        self.assertFalse(service_status, "Service is not in stopped state")

        # Check that service can't be started by using TryRestartService()
        service.TryRestartService()
        self.assertFalse(service.Started, "Stopped service has been started by try-restart")
        service_status_ret = subprocess.call("service cups status".split())
        self.assertEqual(service_status_ret, 3, "Service is in running state")


    def test_service_started_try_restart(self):
        """
        Test LMI_Service.TryRestartService() on running service.

        Use 'cups' service for testing.
        Compare results with:
            pidof cupsd - pid has changed
            service cups status - systemctl confirms OpenLMI status
        Cases:
            running service is running under different pid after TryRestartService
        """
        # Make sure the service is running
        service_status = subprocess.call("service cups start".split())
        self.assertEqual(service_status, 0, "Service is not started or is in unknown state")
        service_pid_before = subprocess.check_output("pidof cupsd".split())

        # Try the TryRestartService and check that pid has changed - service
        # war physically restarted
        service = self.cim_class.first_instance({"Name": "cups.service"})
        service.TryRestartService()
        self.assertTrue(service.Started, "Try-restarted service NOT running")

        service_pid = subprocess.check_output("pidof cupsd".split())
        self.assertNotEqual(service_pid_before, service_pid, "Pids of cupsd service are same as before restart.")

        service_status = subprocess.call("service cups status".split(), stdout=DEV_NULL, stderr=DEV_NULL)
        self.assertEqual(service_status, 0, "Started service not running after try-restart")


    def test_service_get_status_when_stoppped(self):
        """
        Test that LMI_Service.started gets status when service is not running.

        Use 'cups' service for testing.
        Compare results with:
            LMI_Service.Started value
        Cases:
            stopped service returns proper status in OpenLMI
            running service returns proper status in OpenLMI
        """

        # Make sure the service is not running and try to get status
        service_status = subprocess.call("service cups stop".split())
        self.assertEqual(service_status, 0, "Service is not stopped or is in unknown state")

        service = self.cim_class.first_instance({"Name": "cups.service"})
        self.assertFalse(service.Started, "Service is not in stopped state")

        # Make sure the service is running and try to get status
        service_status = subprocess.call("service cups start".split())
        self.assertEqual(service_status, 0, "Service is not started or is in unknown state")

        service.refresh()
        self.assertTrue(service.Started, "Service is not in started state")


    # This TestCase returns errors. Ticket link:
    # https://fedorahosted.org/openlmi/ticket/81
    @unittest.skip("This will never work")
    def test_service_more_instances_status_when_stoppped(self):
        """
        Test LMI_Service gives the same results for multiple instances of one service.

        Use 'cups' service for testing.
        Compare results with:
            service.Stared only and between themselves
        Cases:
            service status is the same for multiple instances of one service
        """
        # Make sure the service is not running
        service_status = subprocess.call("service cups stop".split())
        self.assertEqual(service_status, 0, "Service is not stopped or is in unknown state")

        # Get two instances of the same service and check both statuses are False - not running
        service = self.cim_class.first_instance({"Name": "cups.service"})
        self.assertFalse(service.Started, "Service is not in stopped state")

        service2 = self.cim_class.first_instance({"Name": "cups.service"})
        self.assertFalse(service2.Started, "Service is not in stopped state")

        # Start up one service instance and check both statuses are True - running
        service.StartService()
        self.assertTrue(service.Started, "Service not started")

        self.assertEqual (service.Started, service2.Started, "States of instances are not equal")


    def test_service_turn_on_off(self):
        """
        Test LMI_Service.TurnServiceOff() and LMI_Service.TurnServiceOn()

        Use 'cups' service for testing.
        Compare results with:
            systemctl is-enabled cups.service - service was enabled/disabled
            systemctl status cups.service - service remains in previous state
        Cases:
            service is still running after disabled - TurnServiceOff()
            service is still stopped after enabled - TurnServiceOn()
        """
        # Start the service, disable it and check if it's disabled
        # and service is still running
        service = self.cim_class.first_instance({"Name": "cups.service"})
        service.StartService()
        service.refresh()
        service.TurnServiceOff()
        disabled = subprocess.call(["systemctl", "is-enabled", "cups.service"],
                                   stdout=DEV_NULL, stderr=DEV_NULL)
        self.assertEqual(disabled, 1,
                         "Service couldn't be disabled [systemctl]")
        self.assertTrue(service.Started,
                        "Service is not running after disabled [OpenLMI]")
        running = subprocess.call(["systemctl", "status", "cups.service"],
                                  stdout=DEV_NULL, stderr=DEV_NULL)
        self.assertEqual(running, 0,
                         "Service is not running after disabled [systemctl]")

        # Stop service, enable it and check if it's enabled
        # and service is still not running
        service.StopService()
        service.TurnServiceOn()
        service.refresh()
        disabled = subprocess.call(["systemctl", "is-enabled", "cups.service"],
                                   stdout=DEV_NULL, stderr=DEV_NULL)
        self.assertEqual(disabled, 0,
                         "Service couldn't be enabled [systemctl]")
        self.assertFalse(service.Started,
                         "Service is not stopped after enabled [OpenLMI]")
        stopped = subprocess.call(["systemctl", "status", "cups.service"],
                                  stdout=DEV_NULL, stderr=DEV_NULL)
        self.assertEqual(stopped, 3,
                         "Service is not stopped after enabled [systemctl]")


class TestServiceProviderFailingService(lmibase.LmiTestCase):
    """
    TestCase class testing LMI_Service functionality against constantly failing service.
    """

    CLASS_NAME = "LMI_Service"

    def setUp(self):
        """
        Set up connection to remote server and load failing.service.

        Failing service is copied to systemd and systemctl is reloaded.
        """
        shutil.copy2(os.path.dirname(os.path.abspath(__file__)) + "/failing.service", "/etc/systemd/system/failing.service")
        subprocess.call ("systemctl daemon-reload".split())

    def tearDown(self):
        try:
            os.unlink ("/etc/systemd/system/failing.service")
        except OSError:
            # failing.service may be removed by test_service_race_condition
            pass
        subprocess.call ("systemctl daemon-reload".split())

    def test_failing_service_start_stop(self):
        """
        Test StartService() and StopService() on failing service.

        Use 'failing' service for testing
        Compare results with:
            service failing status - systemctl confirms OpenLMI status
        Cases:
            failing service is not running after StartService
            failing service is not running after StopService
            failing service is not running after TryRestartService
        """
        # Try to start failing service and check OpenLMI knows it failed
        service = self.cim_class.first_instance({"Name": "failing.service"})
        service.StartService()
        self.assertFalse (service.Started)
        service_status = subprocess.call("service failing status".split())
        self.assertEqual(service_status, 3, "Failing service is in different state")

        # Try to stop failing service and check OpenLMI knows it failed
        service.StopService()
        self.assertFalse (service.Started)
        service_status = subprocess.call("service failing status".split())
        self.assertEqual(service_status, 3, "Failing service is in different state")

        # TryRestartService() and check OpenLMI knows it failed
        service.TryRestartService()
        self.assertFalse (service.Started)
        service_status = subprocess.call("service failing status".split())
        self.assertEqual(service_status, 3, "Failing service is in different state")


    # Following test case can be dangerous to run on your system
    def test_service_race_condition(self):
        """
        Test various methods on systemd-unlinked failing service.

        Use 'failing' service for testing
        Compare results with:
            service.Started
        Cases:
            failing service is not running after StartService
            failing service is not running after StopService
            failing service is not running after TryRestartService
            failing service is not running after TurnServiceOff
            failing service is not running after TurnServiceOn
        """
        # Get the failing service instance from LMI_Service, unlink
        # the service from systemd and reload the daemon
        service = self.cim_class.first_instance({"Name": "failing.service"})
        os.unlink ("/etc/systemd/system/failing.service")
        subprocess.call ("systemctl daemon-reload".split())

        # All the following shoud let service.Started in False state
        # Although the instance was initialized, the original service was
        # unlinked from systemd
        service.StartService()
        self.assertFalse(service.Started)

        service.StopService()
        self.assertFalse(service.Started)

        service.TryRestartService()
        self.assertFalse(service.Started)

        service.TurnServiceOff()
        self.assertFalse(service.Started)

        service.TurnServiceOn()
        self.assertFalse(service.Started)


if __name__ == '__main__':
    # Run the standard unittest.main() function to load and run all tests
    unittest.main()
