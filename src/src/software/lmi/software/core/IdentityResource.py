# -*- encoding: utf-8 -*-
# Software Management Providers
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

"""
Just a common functionality related to LMI_SoftwareIdentityResource provider.
"""

import pywbem

from lmi.providers import cmpi_logging
from lmi.providers import ComputerSystem
from lmi.providers import is_this_system
from lmi.software import util
from lmi.software.yumdb import YumDB
from lmi.software.yumdb.repository import Repository

LOG = cmpi_logging.get_logger(__name__)

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
                0 : 'Not Available',
                1 : 'No Additional Information',
                2 : 'Stressed',
                3 : 'Predictive Failure',
                4 : 'Non-Recoverable Error',
                5 : 'Supporting Entity in Error'
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
                0  : 'Unknown',
                2  : 'Enabled',
                3  : 'Disabled',
                4  : 'Shut Down',
                5  : 'No Change',
                6  : 'Offline',
                7  : 'Test',
                8  : 'Deferred',
                9  : 'Quiesce',
                10 : 'Reboot',
                11 : 'Reset',
                12 : 'Not Applicable'
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
                0  : 'Unknown',
                5  : 'OK',
                10 : 'Degraded/Warning',
                15 : 'Minor failure',
                20 : 'Major failure',
                25 : 'Critical failure',
                30 : 'Non-recoverable error'
        }

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
                0  : 'Unknown',
                2  : 'Enabled',
                3  : 'Disabled',
                4  : 'Shut Down',
                5  : 'No Change',
                6  : 'Offline',
                7  : 'Test',
                8  : 'Defer',
                9  : 'Quiesce',
                10 : 'Reboot',
                11 : 'Reset',
                12 : 'Not Applicable'
        }

    class ResourceType(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Installer_and_Payload = pywbem.Uint16(2)
        Installer = pywbem.Uint16(3)
        Payload = pywbem.Uint16(4)
        Installability_checker = pywbem.Uint16(5)
        Security_Advisory = pywbem.Uint16(6)
        Engineering_Advisory = pywbem.Uint16(7)
        Technical_release_notes = pywbem.Uint16(9)
        Change_notification = pywbem.Uint16(10)
        Whitepaper = pywbem.Uint16(11)
        Marketing_Documentation = pywbem.Uint16(12)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..0xFFFF
        _reverse_map = {
                0  : 'Unknown',
                1  : 'Other',
                2  : 'Installer and Payload',
                3  : 'Installer',
                4  : 'Payload',
                5  : 'Installability checker',
                6  : 'Security Advisory',
                7  : 'Engineering Advisory',
                9  : 'Technical release notes',
                10 : 'Change notification',
                11 : 'Whitepaper',
                12 : 'Marketing Documentation'
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
                0  : 'Unknown',
                1  : 'Other',
                2  : 'Enabled',
                3  : 'Disabled',
                4  : 'Shutting Down',
                5  : 'Not Applicable',
                6  : 'Enabled but Offline',
                7  : 'In Test',
                8  : 'Deferred',
                9  : 'Quiesce',
                10 : 'Starting'
        }

    class ExtendedResourceType(object):
        Unknown = pywbem.Uint16(0)
        Not_Applicable = pywbem.Uint16(2)
        Linux_RPM = pywbem.Uint16(3)
        HP_UX_Depot = pywbem.Uint16(4)
        Windows_MSI = pywbem.Uint16(5)
        Solaris_Package = pywbem.Uint16(6)
        Macintosh_Disk_Image = pywbem.Uint16(7)
        Debian_linux_Package = pywbem.Uint16(8)
        HP_Smart_Component = pywbem.Uint16(11)
        # Vendor_Reserved = 101..200
        HTML = pywbem.Uint16(201)
        PDF = pywbem.Uint16(202)
        Text_File = pywbem.Uint16(203)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..0xFFFF
        _reverse_map = {
                0   : 'Unknown',
                2   : 'Not Applicable',
                3   : 'Linux RPM',
                4   : 'HP-UX Depot',
                5   : 'Windows MSI',
                6   : 'Solaris Package',
                7   : 'Macintosh Disk Image',
                8   : 'Debian linux Package',
                201 : 'HTML',
                202 : 'PDF',
                11  : 'HP Smart Component',
                203 : 'Text File'
        }

    class AccessContext(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Default_Gateway = pywbem.Uint16(2)
        DNS_Server = pywbem.Uint16(3)
        SNMP_Trap_Destination = pywbem.Uint16(4)
        MPLS_Tunnel_Destination = pywbem.Uint16(5)
        DHCP_Server = pywbem.Uint16(6)
        SMTP_Server = pywbem.Uint16(7)
        LDAP_Server = pywbem.Uint16(8)
        Network_Time_Protocol__NTP__Server = pywbem.Uint16(9)
        Management_Service = pywbem.Uint16(10)
        internet_Storage_Name_Service__iSNS_ = pywbem.Uint16(11)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 32768..65535
        _reverse_map = {
                0  : 'Unknown',
                1  : 'Other',
                2  : 'Default Gateway',
                3  : 'DNS Server',
                4  : 'SNMP Trap Destination',
                5  : 'MPLS Tunnel Destination',
                6  : 'DHCP Server',
                7  : 'SMTP Server',
                8  : 'LDAP Server',
                9  : 'Network Time Protocol (NTP) Server',
                10 : 'Management Service',
                11 : 'internet Storage Name Service (iSNS)'
        }

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
                2  : 'Enabled',
                3  : 'Disabled',
                4  : 'Shut Down',
                6  : 'Offline',
                7  : 'Test',
                8  : 'Defer',
                9  : 'Quiesce',
                10 : 'Reboot',
                11 : 'Reset'
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
                0 : 'Unknown',
                1 : 'Not Available',
                2 : 'Communication OK',
                3 : 'Lost Communication',
                4 : 'No Contact'
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
                0  : 'Unknown',
                1  : 'Other',
                2  : 'OK',
                3  : 'Degraded',
                4  : 'Stressed',
                5  : 'Predictive Failure',
                6  : 'Error',
                7  : 'Non-Recoverable Error',
                8  : 'Starting',
                9  : 'Stopping',
                10 : 'Stopped',
                11 : 'In Service',
                12 : 'No Contact',
                13 : 'Lost Communication',
                14 : 'Aborted',
                15 : 'Dormant',
                16 : 'Supporting Entity in Error',
                17 : 'Completed',
                18 : 'Power Mode',
                19 : 'Relocating'
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
                0  : 'Unknown',
                1  : 'Not Available',
                2  : 'Servicing',
                3  : 'Starting',
                4  : 'Stopping',
                5  : 'Stopped',
                6  : 'Aborted',
                7  : 'Dormant',
                8  : 'Completed',
                9  : 'Migrating',
                10 : 'Emigrating',
                11 : 'Immigrating',
                12 : 'Snapshotting',
                13 : 'Shutting Down',
                14 : 'In Test',
                15 : 'Transitioning',
                16 : 'In Service'
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
                2 : 'Enabled',
                3 : 'Disabled',
                5 : 'Not Applicable',
                6 : 'Enabled but Offline',
                7 : 'No Default',
                9 : 'Quiesce'
        }

    class PrimaryStatus(object):
        Unknown = pywbem.Uint16(0)
        OK = pywbem.Uint16(1)
        Degraded = pywbem.Uint16(2)
        Error = pywbem.Uint16(3)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..
        _reverse_map = {
                0 : 'Unknown',
                1 : 'OK',
                2 : 'Degraded',
                3 : 'Error'
        }

    class InfoFormat(object):
        Other = pywbem.Uint16(1)
        Host_Name = pywbem.Uint16(2)
        IPv4_Address = pywbem.Uint16(3)
        IPv6_Address = pywbem.Uint16(4)
        IPX_Address = pywbem.Uint16(5)
        DECnet_Address = pywbem.Uint16(6)
        SNA_Address = pywbem.Uint16(7)
        Autonomous_System_Number = pywbem.Uint16(8)
        MPLS_Label = pywbem.Uint16(9)
        IPv4_Subnet_Address = pywbem.Uint16(10)
        IPv6_Subnet_Address = pywbem.Uint16(11)
        IPv4_Address_Range = pywbem.Uint16(12)
        IPv6_Address_Range = pywbem.Uint16(13)
        Dial_String = pywbem.Uint16(100)
        Ethernet_Address = pywbem.Uint16(101)
        Token_Ring_Address = pywbem.Uint16(102)
        ATM_Address = pywbem.Uint16(103)
        Frame_Relay_Address = pywbem.Uint16(104)
        URL = pywbem.Uint16(200)
        FQDN = pywbem.Uint16(201)
        User_FQDN = pywbem.Uint16(202)
        DER_ASN1_DN = pywbem.Uint16(203)
        DER_ASN1_GN = pywbem.Uint16(204)
        Key_ID = pywbem.Uint16(205)
        Parameterized_URL = pywbem.Uint16(206)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 32768..65535
        _reverse_map = {
                1   : 'Other',
                2   : 'Host Name',
                3   : 'IPv4 Address',
                4   : 'IPv6 Address',
                5   : 'IPX Address',
                6   : 'DECnet Address',
                7   : 'SNA Address',
                8   : 'Autonomous System Number',
                9   : 'MPLS Label',
                10  : 'IPv4 Subnet Address',
                11  : 'IPv6 Subnet Address',
                12  : 'IPv4 Address Range',
                13  : 'IPv6 Address Range',
                200 : 'URL',
                201 : 'FQDN',
                202 : 'User FQDN',
                203 : 'DER ASN1 DN',
                204 : 'DER ASN1 GN',
                205 : 'Key ID',
                206 : 'Parameterized URL',
                100 : 'Dial String',
                101 : 'Ethernet Address',
                102 : 'Token Ring Address',
                103 : 'ATM Address',
                104 : 'Frame Relay Address'
        }

@cmpi_logging.trace_function
def object_path2repo(env, op, kind='enabled'):
    """
    @param op must contain precise information of repository,
    otherwise an error is raised
    """
    if not isinstance(kind, basestring):
        raise TypeError("kind must be a string")
    if not isinstance(op, pywbem.CIMInstanceName):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "op must be an instance of CIMInstanceName")

    if (  (not "CreationClassName" in op or not op['CreationClassName'])
       or (not "Name" in op or not op["Name"])
       or (  not "SystemCreationClassName" in op
          or not op["SystemCreationClassName"])
       or (not "SystemName" in op or not op["SystemName"])):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER, "Wrong keys.")
    if not is_this_system(op["SystemName"]):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                'SystemName "%s" does not match "%s".' % (
                    op["SystemName"], ComputerSystem.get_system_name(env)))
    ch = env.get_cimom_handle()
    if not ch.is_subclass(util.Configuration.get_instance().namespace,
            sub=op["CreationClassName"],
            super="CIM_SoftwareIdentityResource"):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                'CreationClassName \"%s\" must be a subclass of "%s".' % (
                    op["CreationClassName"], "CIM_SoftwareIdentityResource"))
    if not ch.is_subclass(util.Configuration.get_instance().namespace,
            sub=op["SystemCreationClassName"],
            super="CIM_ComputerSystem"):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                'SystemCreationClassName of \"%s\" must be a subclass of "%s".'
                % (op["CreationClassName"], "CIM_SoftwareIdentityResource"))
    repos = YumDB.get_instance().filter_repositories(kind, repoid=op["Name"])
    if len(repos) < 1:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                'No matching repository found for Name=\"%s\".' % op["Name"])
    return repos[0]

@cmpi_logging.trace_function
def _fill_non_keys(repo, model):
    """
    Fills into the model of instance all non-key properties.
    """
    model['AccessContext'] = Values.AccessContext.Other
    access_info = None
    if repo.mirror_list:
        access_info = repo.mirror_list
    elif repo.metalink:
        access_info = repo.metalink
    elif repo.base_urls:
        if len(repo.base_urls) > 0:
            if len(repo.base_urls) > 1:
                LOG().warn('multiple base urls found for repository "%s",'
                        ' selecting the last one', repo)
            access_info = repo.base_urls[-1]
    if access_info is None:
        LOG().error('no base url found for repository "%s"', repo)
        access_info = pywbem.CIMProperty('AccessInfo', None, type='string')
    model["AccessInfo"] = access_info
    model['AvailableRequestedStates'] = [
            Values.AvailableRequestedStates.Enabled,
            Values.AvailableRequestedStates.Disabled]
    model['Caption'] = repo.name
    model['Cost'] = pywbem.Sint32(repo.cost)
    model['Description'] = "[%s] - %s for %s architecture with cost %d" % (
            repo.repoid, repo.name, repo.basearch, repo.cost)
    model['ElementName'] = repo.repoid
    model['EnabledDefault'] = Values.EnabledDefault.Not_Applicable
    if repo.enabled:
        model['EnabledState'] = Values.EnabledState.Enabled
    else:
        model['EnabledState'] = Values.EnabledState.Disabled
    model['ExtendedResourceType'] = Values.ExtendedResourceType.Linux_RPM
    model['GPGCheck'] = repo.gpg_check
    if repo.ready:
        model['HealthState'] = Values.HealthState.OK
    else:
        model['HealthState'] = Values.HealthState.Major_failure
    if repo.revision is not None:
        model["Generation"] = pywbem.Uint64(repo.revision)
    else:
        model['Generation'] = pywbem.CIMProperty('Generation',
                None, type='uint64')
    model['InfoFormat'] = Values.InfoFormat.URL
    model['InstanceID'] = 'LMI:LMI_SoftwareIdentityResource:' + repo.repoid
    if repo.mirror_list:
        model["MirrorList"] = repo.mirror_list
    else:
        model['MirrorList'] = pywbem.CIMProperty('MirrorList',
                None, type='string')
    model['OperationalStatus'] = [ Values.OperationalStatus.OK
                if repo.ready else Values.OperationalStatus.Error]
    model['OtherAccessContext'] = "YUM package repository"
    model['OtherResourceType'] = "RPM Software Package"
    # this would need to populateSack, which is expensive
    #model["PackageCount"] = pywbem.Uint32(repo.pkg_count)
    if repo.ready:
        model['PrimaryStatus'] = Values.PrimaryStatus.OK
    else:
        model['PrimaryStatus'] = Values.PrimaryStatus.Error
    model['RepoGPGCheck'] = repo.repo_gpg_check
    if repo.enabled:
        model['RequestedState'] = Values.RequestedState.Enabled
    else:
        model['RequestedState'] = Values.RequestedState.Disabled
    model['ResourceType'] = Values.ResourceType.Other
    model['StatusDescriptions'] = [ "Ready" if repo.ready else "Not Ready" ]
    model['TimeOfLastStateChange'] = pywbem.CIMDateTime(repo.last_edit)
    if repo.last_update is not None:
        model['TimeOfLastUpdate'] = pywbem.CIMDateTime(repo.last_update)
    else:
        model['TimeOfLastUpdate'] = pywbem.CIMProperty('TimeOfLastUpdate',
                None, type='datetime')
    model['TransitioningToState'] = Values.TransitioningToState.Not_Applicable

@cmpi_logging.trace_function
def repo2model(env, repo, keys_only=True, model=None):
    """
    @param model if not None, will be filled with data, otherwise
    a new instance of CIMInstance or CIMObjectPath is created
    """
    if not isinstance(repo, Repository):
        raise TypeError("pkg must be an instance of Repository")
    if model is None:
        model = util.new_instance_name("LMI_SoftwareIdentityResource")
        if not keys_only:
            model = pywbem.CIMInstance(
                    "LMI_SoftwareIdentityResource", path=model)
    if isinstance(model, pywbem.CIMInstance):
        def _set_key(k, value):
            """Sets the value of key property of cim instance"""
            model[k] = value
            model.path[k] = value #pylint: disable=E1103
    else:
        _set_key = model.__setitem__
    _set_key('CreationClassName', "LMI_SoftwareIdentityResource")
    _set_key("Name", repo.repoid)
    _set_key("SystemCreationClassName",
            util.Configuration.get_instance().system_class_name)
    _set_key("SystemName", ComputerSystem.get_system_name(env))
    if not keys_only:
        _fill_non_keys(repo, model)

    return model

