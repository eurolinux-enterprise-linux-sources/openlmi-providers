# -*- encoding: utf-8 -*-
# Software Management Providers
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

"""
Just a common functionality related to LMI_SoftwareInstallationService
provider.
"""

import pywbem

from lmi.providers import cmpi_logging
from lmi.providers import ComputerSystem
from lmi.providers import is_this_system
from lmi.software import util
from lmi.software.core import Identity
from lmi.software.core import Job
from lmi.software.core import SystemCollection
from lmi.software.yumdb import errors
from lmi.software.yumdb import YumDB

JOB_METHOD_SRC_PARAM_NAMES = ["Source", "URI", "Image", "Source"]

LOG = cmpi_logging.get_logger(__name__)

class InstallationError(Exception):
    """This exception shall be raised upon any error within
    install_or_remove_package() function.
    """
    def __init__(self, return_code, description):
        Exception.__init__(self, return_code, description)
    @property
    def return_code(self):
        """@return return code of CIM method"""
        return self.args[0]
    @property
    def description(self):
        """@return description of error"""
        return self.args[1]

class Values(object):
    class DetailedStatus(object):
        Not_Available = pywbem.Uint16(0)
        No_Additional_Information = pywbem.Uint16(1)
        Stressed = pywbem.Uint16(2)
        Predictive_Failure = pywbem.Uint16(3)
        Non_Recoverable_Error = pywbem.Uint16(4)
        Supporting_Entity_in_Error = pywbem.Uint16(5)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..
        _reverse_map = {
                0: 'Not Available',
                1: 'No Additional Information',
                2: 'Stressed',
                3: 'Predictive Failure',
                4: 'Non-Recoverable Error',
                5: 'Supporting Entity in Error'
        }

    class RequestedState(object):
        Unknown = pywbem.Uint16(0)
        Enabled = pywbem.Uint16(2)
        Disabled = pywbem.Uint16(3)
        Shut_Down = pywbem.Uint16(4)
        No_Change = pywbem.Uint16(5)
        Offline = pywbem.Uint16(6)
        Test = pywbem.Uint16(7)
        Deferred = pywbem.Uint16(8)
        Quiesce = pywbem.Uint16(9)
        Reboot = pywbem.Uint16(10)
        Reset = pywbem.Uint16(11)
        Not_Applicable = pywbem.Uint16(12)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 32768..65535
        _reverse_map = {
                0: 'Unknown',
                2: 'Enabled',
                3: 'Disabled',
                4: 'Shut Down',
                5: 'No Change',
                6: 'Offline',
                7: 'Test',
                8: 'Deferred',
                9: 'Quiesce',
                10: 'Reboot',
                11: 'Reset',
                12: 'Not Applicable'
        }

    class HealthState(object):
        Unknown = pywbem.Uint16(0)
        OK = pywbem.Uint16(5)
        Degraded_Warning = pywbem.Uint16(10)
        Minor_failure = pywbem.Uint16(15)
        Major_failure = pywbem.Uint16(20)
        Critical_failure = pywbem.Uint16(25)
        Non_recoverable_error = pywbem.Uint16(30)
        # DMTF_Reserved = ..
        # Vendor_Specific = 32768..65535
        _reverse_map = {
                0: 'Unknown',
                5: 'OK',
                10: 'Degraded/Warning',
                15: 'Minor failure',
                20: 'Major failure',
                25: 'Critical failure',
                30: 'Non-recoverable error'
        }

    class InstallFromURI(object):
        Job_Completed_with_No_Error = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Unspecified_Error = pywbem.Uint32(2)
        Timeout = pywbem.Uint32(3)
        Failed = pywbem.Uint32(4)
        Invalid_Parameter = pywbem.Uint32(5)
        Target_In_Use = pywbem.Uint32(6)
        # DMTF_Reserved = ..
        Method_Parameters_Checked___Job_Started = pywbem.Uint32(4096)
        Unsupported_TargetType = pywbem.Uint32(4097)
        Unattended_silent_installation_not_supported = pywbem.Uint32(4098)
        Downgrade_reinstall_not_supported = pywbem.Uint32(4099)
        Not_enough_memory = pywbem.Uint32(4100)
        Not_enough_swap_space = pywbem.Uint32(4101)
        Unsupported_version_transition = pywbem.Uint32(4102)
        Not_enough_disk_space = pywbem.Uint32(4103)
        Software_and_target_operating_system_mismatch = pywbem.Uint32(4104)
        Missing_dependencies = pywbem.Uint32(4105)
        Not_applicable_to_target = pywbem.Uint32(4106)
        URI_not_accessible = pywbem.Uint32(4107)
        # Method_Reserved = 4108..32767
        # Vendor_Specific = 32768..65535
        class InstallOptions(object):
            Defer_target_system_reset = pywbem.Uint16(2)
            Force_installation = pywbem.Uint16(3)
            Install = pywbem.Uint16(4)
            Update = pywbem.Uint16(5)
            Repair = pywbem.Uint16(6)
            Reboot = pywbem.Uint16(7)
            Password = pywbem.Uint16(8)
            Uninstall = pywbem.Uint16(9)
            Log = pywbem.Uint16(10)
            SilentMode = pywbem.Uint16(11)
            AdministrativeMode = pywbem.Uint16(12)
            ScheduleInstallAt = pywbem.Uint16(13)
            # DMTF_Reserved = ..
            # Vendor_Specific = 32768..65535
            _reverse_map = {
                2  : "Defer Target/System Reset",
                3  : "Force Installation",
                4  : "Install",
                5  : "Update",
                6  : "Repair",
                7  : "Reboot",
                8  : "Password",
                9  : "Uninstall",
                10 : "Log",
                11 : "SilentMode",
                12 : "AdministrativeMode",
                13 : "ScheduleInstallAt",
            }

        InstallOptions.supported = set((
            InstallOptions.Install,
            InstallOptions.Update,
            InstallOptions.Uninstall,
            InstallOptions.Force_installation,
            InstallOptions.Repair,
        ))


    class CheckSoftwareIdentity(object):
        Job_Completed_with_No_Error = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Unspecified_Error = pywbem.Uint32(2)
        Timeout = pywbem.Uint32(3)
        Failed = pywbem.Uint32(4)
        Invalid_Parameter = pywbem.Uint32(5)
        Target_In_Use = pywbem.Uint32(6)
        # DMTF_Reserved = ..
        Method_Reserved = pywbem.Uint32(4096)
        Unsupported_TargetType = pywbem.Uint32(4097)
        Unattended_silent_installation_not_supported = pywbem.Uint32(4098)
        Downgrade_reinstall_not_supported = pywbem.Uint32(4099)
        Not_enough_memory = pywbem.Uint32(4100)
        Not_enough_swap_space = pywbem.Uint32(4101)
        Unsupported_version_transition = pywbem.Uint32(4102)
        Not_enough_disk_space = pywbem.Uint32(4103)
        Software_and_target_operating_system_mismatch = pywbem.Uint32(4104)
        Missing_dependencies = pywbem.Uint32(4105)
        Not_applicable_to_target = pywbem.Uint32(4106)
        No_supported_path_to_image = pywbem.Uint32(4107)
        Cannot_add_to_Collection = pywbem.Uint32(4108)
        Asynchronous_Job_already_in_progress = pywbem.Uint32(4109)
        # Method_Reserved = 4110..32767
        # Vendor_Specific = 32768..65535
        class InstallCharacteristics(object):
            Target_automatic_reset = pywbem.Uint16(2)
            System_automatic_reset = pywbem.Uint16(3)
            Separate_target_reset_Required = pywbem.Uint16(4)
            Separate_system_reset_Required = pywbem.Uint16(5)
            Manual_Reboot_Required = pywbem.Uint16(6)
            No_Reboot_Required = pywbem.Uint16(7)
            User_Intervention_recommended = pywbem.Uint16(8)
            MAY_be_added_to_specified_Collection = pywbem.Uint16(9)
            # DMTF_Reserved = ..
            # Vendor_Specific = 0x7FFF..0xFFFF

    class ChangeAffectedElementsAssignedSequence(object):
        Completed_with_No_Error = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Error_Occured = pywbem.Uint32(2)
        Busy = pywbem.Uint32(3)
        Invalid_Reference = pywbem.Uint32(4)
        Invalid_Parameter = pywbem.Uint32(5)
        Access_Denied = pywbem.Uint32(6)
        # DMTF_Reserved = 7..32767
        # Vendor_Specified = 32768..65535

    class TransitioningToState(object):
        Unknown = pywbem.Uint16(0)
        Enabled = pywbem.Uint16(2)
        Disabled = pywbem.Uint16(3)
        Shut_Down = pywbem.Uint16(4)
        No_Change = pywbem.Uint16(5)
        Offline = pywbem.Uint16(6)
        Test = pywbem.Uint16(7)
        Defer = pywbem.Uint16(8)
        Quiesce = pywbem.Uint16(9)
        Reboot = pywbem.Uint16(10)
        Reset = pywbem.Uint16(11)
        Not_Applicable = pywbem.Uint16(12)
        # DMTF_Reserved = ..
        _reverse_map = {
                0: 'Unknown',
                2: 'Enabled',
                3: 'Disabled',
                4: 'Shut Down',
                5: 'No Change',
                6: 'Offline',
                7: 'Test',
                8: 'Defer',
                9: 'Quiesce',
                10: 'Reboot',
                11: 'Reset',
                12: 'Not Applicable'
        }

    class EnabledDefault(object):
        Enabled = pywbem.Uint16(2)
        Disabled = pywbem.Uint16(3)
        Not_Applicable = pywbem.Uint16(5)
        Enabled_but_Offline = pywbem.Uint16(6)
        No_Default = pywbem.Uint16(7)
        Quiesce = pywbem.Uint16(9)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 32768..65535
        _reverse_map = {
                2: 'Enabled',
                3: 'Disabled',
                5: 'Not Applicable',
                6: 'Enabled but Offline',
                7: 'No Default',
                9: 'Quiesce'
        }

    class EnabledState(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Enabled = pywbem.Uint16(2)
        Disabled = pywbem.Uint16(3)
        Shutting_Down = pywbem.Uint16(4)
        Not_Applicable = pywbem.Uint16(5)
        Enabled_but_Offline = pywbem.Uint16(6)
        In_Test = pywbem.Uint16(7)
        Deferred = pywbem.Uint16(8)
        Quiesce = pywbem.Uint16(9)
        Starting = pywbem.Uint16(10)
        # DMTF_Reserved = 11..32767
        # Vendor_Reserved = 32768..65535
        _reverse_map = {
                0: 'Unknown',
                1: 'Other',
                2: 'Enabled',
                3: 'Disabled',
                4: 'Shutting Down',
                5: 'Not Applicable',
                6: 'Enabled but Offline',
                7: 'In Test',
                8: 'Deferred',
                9: 'Quiesce',
                10: 'Starting'
        }

    class VerifyInstalledIdentity(object):
        Job_Completed_with_No_Error = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Unspecified_Error = pywbem.Uint32(2)
        Timeout = pywbem.Uint32(3)
        Failed = pywbem.Uint32(4)
        Invalid_Parameter = pywbem.Uint32(5)
        Target_In_Use = pywbem.Uint32(6)
        # DMTF_Reserved = ..
        Method_Parameters_Checked___Job_Started = pywbem.Uint32(4096)
        Unsupported_TargetType = pywbem.Uint32(4097)
        Method_Reserved = pywbem.Uint32(4098)
        # Software_Identity_Not_Installed = 4099..32767
        Vendor_Specific = pywbem.Uint32(32768)

    class AvailableRequestedStates(object):
        Enabled = pywbem.Uint16(2)
        Disabled = pywbem.Uint16(3)
        Shut_Down = pywbem.Uint16(4)
        Offline = pywbem.Uint16(6)
        Test = pywbem.Uint16(7)
        Defer = pywbem.Uint16(8)
        Quiesce = pywbem.Uint16(9)
        Reboot = pywbem.Uint16(10)
        Reset = pywbem.Uint16(11)
        # DMTF_Reserved = ..
        _reverse_map = {
                2: 'Enabled',
                3: 'Disabled',
                4: 'Shut Down',
                6: 'Offline',
                7: 'Test',
                8: 'Defer',
                9: 'Quiesce',
                10: 'Reboot',
                11: 'Reset'
        }

    class Status(object):
        OK = 'OK'
        Error = 'Error'
        Degraded = 'Degraded'
        Unknown = 'Unknown'
        Pred_Fail = 'Pred Fail'
        Starting = 'Starting'
        Stopping = 'Stopping'
        Service = 'Service'
        Stressed = 'Stressed'
        NonRecover = 'NonRecover'
        No_Contact = 'No Contact'
        Lost_Comm = 'Lost Comm'
        Stopped = 'Stopped'

    class CommunicationStatus(object):
        Unknown = pywbem.Uint16(0)
        Not_Available = pywbem.Uint16(1)
        Communication_OK = pywbem.Uint16(2)
        Lost_Communication = pywbem.Uint16(3)
        No_Contact = pywbem.Uint16(4)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..
        _reverse_map = {
                0: 'Unknown',
                1: 'Not Available',
                2: 'Communication OK',
                3: 'Lost Communication',
                4: 'No Contact'
        }

    class OperationalStatus(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        OK = pywbem.Uint16(2)
        Degraded = pywbem.Uint16(3)
        Stressed = pywbem.Uint16(4)
        Predictive_Failure = pywbem.Uint16(5)
        Error = pywbem.Uint16(6)
        Non_Recoverable_Error = pywbem.Uint16(7)
        Starting = pywbem.Uint16(8)
        Stopping = pywbem.Uint16(9)
        Stopped = pywbem.Uint16(10)
        In_Service = pywbem.Uint16(11)
        No_Contact = pywbem.Uint16(12)
        Lost_Communication = pywbem.Uint16(13)
        Aborted = pywbem.Uint16(14)
        Dormant = pywbem.Uint16(15)
        Supporting_Entity_in_Error = pywbem.Uint16(16)
        Completed = pywbem.Uint16(17)
        Power_Mode = pywbem.Uint16(18)
        Relocating = pywbem.Uint16(19)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..
        _reverse_map = {
                0: 'Unknown',
                1: 'Other',
                2: 'OK',
                3: 'Degraded',
                4: 'Stressed',
                5: 'Predictive Failure',
                6: 'Error',
                7: 'Non-Recoverable Error',
                8: 'Starting',
                9: 'Stopping',
                10: 'Stopped',
                11: 'In Service',
                12: 'No Contact',
                13: 'Lost Communication',
                14: 'Aborted',
                15: 'Dormant',
                16: 'Supporting Entity in Error',
                17: 'Completed',
                18: 'Power Mode',
                19: 'Relocating'
        }

    class OperatingStatus(object):
        Unknown = pywbem.Uint16(0)
        Not_Available = pywbem.Uint16(1)
        Servicing = pywbem.Uint16(2)
        Starting = pywbem.Uint16(3)
        Stopping = pywbem.Uint16(4)
        Stopped = pywbem.Uint16(5)
        Aborted = pywbem.Uint16(6)
        Dormant = pywbem.Uint16(7)
        Completed = pywbem.Uint16(8)
        Migrating = pywbem.Uint16(9)
        Emigrating = pywbem.Uint16(10)
        Immigrating = pywbem.Uint16(11)
        Snapshotting = pywbem.Uint16(12)
        Shutting_Down = pywbem.Uint16(13)
        In_Test = pywbem.Uint16(14)
        Transitioning = pywbem.Uint16(15)
        In_Service = pywbem.Uint16(16)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..
        _reverse_map = {
                0: 'Unknown',
                1: 'Not Available',
                2: 'Servicing',
                3: 'Starting',
                4: 'Stopping',
                5: 'Stopped',
                6: 'Aborted',
                7: 'Dormant',
                8: 'Completed',
                9: 'Migrating',
                10: 'Emigrating',
                11: 'Immigrating',
                12: 'Snapshotting',
                13: 'Shutting Down',
                14: 'In Test',
                15: 'Transitioning',
                16: 'In Service'
        }

    class RequestStateChange(object):
        Completed_with_No_Error = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Unknown_or_Unspecified_Error = pywbem.Uint32(2)
        Cannot_complete_within_Timeout_Period = pywbem.Uint32(3)
        Failed = pywbem.Uint32(4)
        Invalid_Parameter = pywbem.Uint32(5)
        In_Use = pywbem.Uint32(6)
        # DMTF_Reserved = ..
        Method_Parameters_Checked___Job_Started = pywbem.Uint32(4096)
        Invalid_State_Transition = pywbem.Uint32(4097)
        Use_of_Timeout_Parameter_Not_Supported = pywbem.Uint32(4098)
        Busy = pywbem.Uint32(4099)
        # Method_Reserved = 4100..32767
        # Vendor_Specific = 32768..65535
        class RequestedState(object):
            Enabled = pywbem.Uint16(2)
            Disabled = pywbem.Uint16(3)
            Shut_Down = pywbem.Uint16(4)
            Offline = pywbem.Uint16(6)
            Test = pywbem.Uint16(7)
            Defer = pywbem.Uint16(8)
            Quiesce = pywbem.Uint16(9)
            Reboot = pywbem.Uint16(10)
            Reset = pywbem.Uint16(11)
            # DMTF_Reserved = ..
            # Vendor_Reserved = 32768..65535

    class InstallFromByteStream(object):
        Job_Completed_with_No_Error = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Unspecified_Error = pywbem.Uint32(2)
        Timeout = pywbem.Uint32(3)
        Failed = pywbem.Uint32(4)
        Invalid_Parameter = pywbem.Uint32(5)
        Target_In_Use = pywbem.Uint32(6)
        # DMTF_Reserved = ..
        Method_Parameters_Checked___Job_Started = pywbem.Uint32(4096)
        Unsupported_TargetType = pywbem.Uint32(4097)
        Unattended_silent_installation_not_supported = pywbem.Uint32(4098)
        Downgrade_reinstall_not_supported = pywbem.Uint32(4099)
        Not_enough_memory = pywbem.Uint32(4100)
        Not_enough_swap_space = pywbem.Uint32(4101)
        Unsupported_version_transition = pywbem.Uint32(4102)
        Not_enough_disk_space = pywbem.Uint32(4103)
        Software_and_target_operating_system_mismatch = pywbem.Uint32(4104)
        Missing_dependencies = pywbem.Uint32(4105)
        Not_applicable_to_target = pywbem.Uint32(4106)
        No_supported_path_to_image = pywbem.Uint32(4107)
        # Method_Reserved = 4108..32767
        # Vendor_Specific = 32768..65535

    InstallFromByteStream.InstallOptions = InstallFromURI.InstallOptions

    class InstallFromSoftwareIdentity(object):
        Job_Completed_with_No_Error = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Unspecified_Error = pywbem.Uint32(2)
        Timeout = pywbem.Uint32(3)
        Failed = pywbem.Uint32(4)
        Invalid_Parameter = pywbem.Uint32(5)
        Target_In_Use = pywbem.Uint32(6)
        # DMTF_Reserved = ..
        Method_Parameters_Checked___Job_Started = pywbem.Uint32(4096)
        Unsupported_TargetType = pywbem.Uint32(4097)
        Unattended_silent_installation_not_supported = pywbem.Uint32(4098)
        Downgrade_reinstall_not_supported = pywbem.Uint32(4099)
        Not_enough_memory = pywbem.Uint32(4100)
        Not_enough_swap_space = pywbem.Uint32(4101)
        Unsupported_version_transition = pywbem.Uint32(4102)
        Not_enough_disk_space = pywbem.Uint32(4103)
        Software_and_target_operating_system_mismatch = pywbem.Uint32(4104)
        Missing_dependencies = pywbem.Uint32(4105)
        Not_applicable_to_target = pywbem.Uint32(4106)
        No_supported_path_to_image = pywbem.Uint32(4107)
        Cannot_add_to_Collection = pywbem.Uint32(4108)
        # Method_Reserved = 4109..32767
        # Vendor_Specific = 32768..65535

    InstallFromSoftwareIdentity.InstallOptions = InstallFromURI.InstallOptions

    class StartMode(object):
        Automatic = 'Automatic'
        Manual = 'Manual'

    class PrimaryStatus(object):
        Unknown = pywbem.Uint16(0)
        OK = pywbem.Uint16(1)
        Degraded = pywbem.Uint16(2)
        Error = pywbem.Uint16(3)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..
        _reverse_map = {
                0: 'Unknown',
                1: 'OK',
                2: 'Degraded',
                3: 'Error'
        }

    class FindIdentity(object):
        Found = pywbem.Uint32(0)
        NoMatch = pywbem.Uint32(1)

def get_path(env):
    """@return instance name with prefilled properties"""
    systemop = ComputerSystem.get_path(env)
    clsname = "LMI_SoftwareInstallationService"
    op = util.new_instance_name(clsname,
            CreationClassName=clsname,
            SystemCreationClassName=systemop.classname,
            SystemName=systemop["Name"],
            Name="LMI:LMI_SoftwareInstallationService")
    return op

@cmpi_logging.trace_function
def check_path(env, service, prop_name):
    """
    Checks instance name of SoftwareInstallationService.
    @param prop_name name of object name; used for error descriptions
    """
    if not isinstance(service, pywbem.CIMInstanceName):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "\"%s\" must be a CIMInstanceName" % prop_name)
    our_service = get_path(env)
    ch = env.get_cimom_handle()
    if service.namespace != our_service.namespace:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                'Namespace of "%s" does not match "%s"' % (
                    prop_name, our_service.namespace))
    if not ch.is_subclass(our_service.namespace,
            sub=service.classname,
            super=our_service.classname):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "Class of \"%s\" must be a subclass of %s" % (
                    prop_name, our_service.classname))
    for key in our_service.keybindings.keys():
        if not key in service:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    "\"%s\" is missing %s key property" % ( prop_name, key))
        if key == "SystemName":
            continue
    if not is_this_system(service["SystemName"]):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "\"%s\" key property SystemName(%s) does not match \"%s\"" % (
                prop_name, service[key], our_service[key]))
    return True

@cmpi_logging.trace_function
def check_path_property(env, op, prop_name):
    """
    Checks, whether prop_name property of op object path is correct.
    If not, an exception will be raised.
    """
    if not prop_name in op:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "Missing %s key property!" % prop_name)
    return check_path(env, op[prop_name], prop_name)

@cmpi_logging.trace_function
def _check_target_and_collection(env, method, target, collection):
    """
    Checks Target and Collection parameters of provider's installation
    methods.
    """
    values = Values.InstallFromSoftwareIdentity
    if target:
        try:
            ComputerSystem.check_path(env, target, "Target")
        except pywbem.CIMError as exc:
            raise InstallationError(values.Unspecified_Error,
                    "Target must be either NULL or match managed"
                    " computer system: %s", str(exc))
    if collection:
        try:
            SystemCollection.check_path(env, collection, "Collection")
        except pywbem.CIMError as exc:
            raise InstallationError(values.Unspecified_Error,
                    "Collection does not match system software collection: %s" %
                    str(exc))
    if target and collection:
        raise InstallationError(values.Unspecified_Error,
                "Only one of Target and Collection parameters can be specified"
                " at the same time.")
    if not target and not collection:
        raise InstallationError(values.Unspecified_Error,
                  "Either Target or Collection parameter must be specified."
             if method == Job.JOB_METHOD_INSTALL_FROM_SOFTWARE_IDENTITY
             else "Missing Target parameter.")

@cmpi_logging.trace_function
def _install_or_remove_check_params(
        env, method, source, target, collection,
        install_options,
        install_options_values):
    """
    Checks parameters of provider's installation methods.
    Upon any invalid option an InstallationError will be raised.
    @return tuple (action, force, repair)
      where action is one of Values.InstallFromSoftwareIdentity properties
    """
    if not method in (
            Job.JOB_METHOD_INSTALL_FROM_URI,
            Job.JOB_METHOD_INSTALL_FROM_SOFTWARE_IDENTITY,
            Job.JOB_METHOD_INSTALL_FROM_BYTE_STREAM):
        raise ValueError("unknown method")

    values = Values.InstallFromSoftwareIdentity
    supported_options = values.InstallOptions.supported.copy()
    if method == Job.JOB_METHOD_INSTALL_FROM_URI:
        supported_options.remove(values.InstallOptions.Uninstall)
        if not isinstance(source, basestring):
            raise InstallationError(values.Unspecified_Error,
                "Expected URI string as an URI parameter.")
    else:
        if not isinstance(source, pywbem.CIMInstanceName):
            raise InstallationError(values.Unspecified_Error,
                "Expected CIM object path as a Source parameter.")

    if not source:
        raise InstallationError(values.Unspecified_Error,
                "Missing %s parameter." % (JOB_METHOD_SRC_PARAM_NAMES[method]))
    if not install_options:
        install_options = []
    elif not isinstance(install_options, list):
        raise InstallationError(values.Unspecified_Error,
                "InstallOptions must be a list of uint16 values.")
    options = set(p for p in install_options)

    if options - supported_options:
        raise InstallationError(values.Unspecified_Error,
                "unsupported install options: {%s}" %
                ", ".join([   values.InstallOptions._reverse_map[p]
                          for p in options - supported_options]))
    if install_options_values and len(options) != len(install_options_values):
        raise InstallationError(values.Unspecified_Error,
                "InstallOptions array must have the same"
                " length as InstallOptionsValues: %d != %d" % (
                    len(options), len(install_options_values)))
    if install_options_values:
        for opt, val in zip(install_options, install_options_values):
            if val:
                raise InstallationError(values.Unspecified_Error,
                        "install option \"%s\" can not have any"
                        " associated value: %s" % (opt, val))
    _check_target_and_collection(env, method, target, collection)
    exclusive = [opt for opt in options if opt in (
        values.InstallOptions.Install,
        values.InstallOptions.Update,
        values.InstallOptions.Uninstall )]
    if len(exclusive) > 1:
        raise InstallationError(values.Unspecified_Error,
            "specified more than one mutually exclusive option at once: {%s}" %
            ", ".join([   values.InstallOptions._reverse_map[p]
                      for p in exclusive]))
    if not exclusive:
        exclusive.append(values.InstallOptions.Install)
    return ( exclusive[0]
           , values.InstallOptions.Force_installation in options
           , values.InstallOptions.Repair in options)

@cmpi_logging.trace_function
def make_job_input_params(method, source, target, collection,
        install_options=None,
        install_options_values=None):
    """
    Make dictionary of input parameters, that are stored in job's metadata.
    This dictionary is used in creation of CIM_ConcreteJob and
    CIM_InstMethodCall.
    """
    input_params = {
        "Target" : pywbem.CIMProperty(
            name="Target", type="reference", value=target),
    }

    if method == Job.JOB_METHOD_VERIFY_INSTALLED_IDENTITY:
        input_params["Source"] = pywbem.CIMProperty(
            name="Source", type="reference", value=source)
        return input_params

    input_params["InstallOptionsValues"] = pywbem.CIMProperty(
            name="InstallOptionsValues",
            type="string",
            is_array=True,
            value=install_options_values)
    input_params["InstallOptions"] = pywbem.CIMProperty(
            name="InstallOptions", type="uint16",
            is_array=True,
            value=install_options)
    if method == Job.JOB_METHOD_INSTALL_FROM_URI:
        input_params["URI"] = pywbem.CIMProperty(
            name="URI", type="string", value=source)
    elif method == Job.JOB_METHOD_INSTALL_FROM_SOFTWARE_IDENTITY:
        input_params["Source"] = pywbem.CIMProperty(
            name="Source", type="reference", value=source)
        input_params["Collection"] = pywbem.CIMProperty(
            name="Collection", type="reference", value=collection)
    elif method == Job.JOB_METHOD_INSTALL_FROM_BYTE_STREAM:
        input_params["Image"] = pywbem.CIMProperty(
            name="Image", type="reference", value=source)

    return input_params

@cmpi_logging.trace_function
def install_or_remove_package(env, method,
        source, target, collection,
        install_options,
        install_options_values):
    """
    :param method: (``int``) Identifier of method defined in ``core.Job``
        module with variables prefixed with ``JOB_METHOD_``.
    """
    values = Values.InstallFromSoftwareIdentity
    (action, force, repair) = _install_or_remove_check_params(
            env, method, source, target, collection,
            install_options, install_options_values)
    input_params = make_job_input_params(method,
            source, target, collection, install_options,
            install_options_values)
    metadata = { "method" : method, "input_params" : input_params }
    try:
        ydb = YumDB.get_instance()
        if action == values.InstallOptions.Uninstall:
            src = Identity.object_path2nevra(source, with_epoch='ALWAYS')
            LOG().info('removing package %s', src)
            jobid = ydb.remove_package(src, async=True, **metadata)
        else:
            update = action == values.InstallOptions.Update
            if method == Job.JOB_METHOD_INSTALL_FROM_URI:
                LOG().info('%s package "%s"',
                        'updating' if update else 'installing', source)
                src = source
                jobid = ydb.install_package_from_uri(
                        source, update_only=update, force=force or repair,
                        async=True, **metadata)
            else:   # software identity
                src = Identity.object_path2nevra(source, with_epoch='ALWAYS')
                if update:
                    jobid = ydb.update_package(src,
                            force=force or repair, async=True, **metadata)
                else:
                    jobid = ydb.install_package(src,
                            force=force or repair, async=True, **metadata)
        LOG().info('installation job %s for pkg "%s" enqueued', jobid, src)
        return jobid

    except (pywbem.CIMError, errors.InvalidURI) as exc:
        LOG().exception('failed to install/remove package "%s" from %s: %s',
                source, JOB_METHOD_SRC_PARAM_NAMES[method].lower(), str(exc))
        raise InstallationError(values.Unspecified_Error, str(exc))

@cmpi_logging.trace_function
def verify_package(env, method, source, target):
    """
    :param method: (``int``) Identifier of method defined in ``core.Job``
        module with variables prefixed with ``JOB_METHOD_``.
    """
    values = Values.VerifyInstalledIdentity

    if method != Job.JOB_METHOD_VERIFY_INSTALLED_IDENTITY:
        raise ValueError("unknown method")

    if not source:
        raise InstallationError(values.Unspecified_Error,
                "Missing %s parameter." % (JOB_METHOD_SRC_PARAM_NAMES[method]))

    _check_target_and_collection(env, method, target, None)

    input_params = make_job_input_params(method, source, target, None)
    metadata = { "method" : method, "input_params" : input_params }
    nevra = Identity.object_path2nevra(source, with_epoch='ALWAYS')
    ydb = YumDB.get_instance()
    LOG().info('verifying package "%s"', nevra)
    jobid = ydb.check_package(nevra, async=True, **metadata)
    LOG().info('verification job %s for pkg "%s" enqueued', jobid, nevra)
    return jobid

