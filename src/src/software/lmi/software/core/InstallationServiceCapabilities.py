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
Just a common functionality related to
LMI_SoftwareInstallationServiceCapabilities provider.
"""

import pywbem

from lmi.providers import cmpi_logging
from lmi.software import util

class Values(object):
    class SupportedExtendedResourceTypes(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Not_Applicable = pywbem.Uint16(2)
        Linux_RPM = pywbem.Uint16(3)
        HP_UX_Depot = pywbem.Uint16(4)
        Windows_MSI = pywbem.Uint16(5)
        Solaris_Package = pywbem.Uint16(6)
        Macintosh_Disk_Image = pywbem.Uint16(7)
        Debian_linux_Package = pywbem.Uint16(8)
        VMware_vSphere_Installation_Bundle = pywbem.Uint16(9)
        VMware_Software_Bulletin = pywbem.Uint16(10)
        HP_Smart_Component = pywbem.Uint16(11)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..
        _reverse_map = {
                0  : 'Unknown',
                1  : 'Other',
                2  : 'Not Applicable',
                3  : 'Linux RPM',
                4  : 'HP-UX Depot',
                5  : 'Windows MSI',
                6  : 'Solaris Package',
                7  : 'Macintosh Disk Image',
                8  : 'Debian linux Package',
                9  : 'VMware vSphere Installation Bundle',
                10 : 'VMware Software Bulletin',
                11 : 'HP Smart Component'
        }

    class SupportedInstallOptions(object):
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
                2  : 'Defer target/system reset',
                3  : 'Force installation',
                4  : 'Install',
                5  : 'Update',
                6  : 'Repair',
                7  : 'Reboot',
                8  : 'Password',
                9  : 'Uninstall',
                10 : 'Log',
                11 : 'SilentMode',
                12 : 'AdministrativeMode',
                13 : 'ScheduleInstallAt'
        }

    class SupportedURISchemes(object):
        data = pywbem.Uint16(2)
        file = pywbem.Uint16(3)
        ftp = pywbem.Uint16(4)
        http = pywbem.Uint16(5)
        https = pywbem.Uint16(6)
        nfs = pywbem.Uint16(7)
        tftp = pywbem.Uint16(8)
        # DMTF_Reserved = ..
        # Vendor_Specific = 0x8000..0xFFFF
        _reverse_map = {
                2 : 'data',
                3 : 'file',
                4 : 'ftp',
                5 : 'http',
                6 : 'https',
                7 : 'nfs',
                8 : 'tftp'
        }

    class SupportedAsynchronousActions(object):
        None_supported = pywbem.Uint16(2)
        Install_From_Software_Identity = pywbem.Uint16(3)
        Install_from_ByteStream = pywbem.Uint16(4)
        Install_from_URI = pywbem.Uint16(5)
        _reverse_map = {
                2 : 'None supported',
                3 : 'Install From Software Identity',
                4 : 'Install from ByteStream',
                5 : 'Install from URI'
        }

    class CreateGoalSettings(object):
        Success = pywbem.Uint16(0)
        Not_Supported = pywbem.Uint16(1)
        Unknown = pywbem.Uint16(2)
        Timeout = pywbem.Uint16(3)
        Failed = pywbem.Uint16(4)
        Invalid_Parameter = pywbem.Uint16(5)
        Alternative_Proposed = pywbem.Uint16(6)
        # DMTF_Reserved = ..
        # Vendor_Specific = 32768..65535

    class SupportedSynchronousActions(object):
        None_supported = pywbem.Uint16(2)
        Install_From_Software_Identity = pywbem.Uint16(3)
        Install_from_ByteStream = pywbem.Uint16(4)
        Install_from_URI = pywbem.Uint16(5)
        _reverse_map = {
                2 : 'None supported',
                3 : 'Install From Software Identity',
                4 : 'Install from ByteStream',
                5 : 'Install from URI'
        }

@cmpi_logging.trace_function
def get_path():
    """@return instance name with prefilled properties"""
    op = util.new_instance_name("LMI_SoftwareInstallationServiceCapabilities",
            InstanceID="LMI:LMI_SoftwareInstallationServiceCapabilities")
    return op

@cmpi_logging.trace_function
def check_path(env, caps, prop_name):
    """
    Checks instance name of SoftwareInstallationServiceCapabilities.
    @param prop_name name of object name; used for error descriptions
    """
    if not isinstance(caps, pywbem.CIMInstanceName):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "\"%s\" must be a CIMInstanceName" % prop_name)
    our_caps = get_path()
    ch = env.get_cimom_handle()
    if caps.namespace != our_caps.namespace:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                'Namespace of "%s" does not match "%s"' % (
                    prop_name, our_caps.namespace))
    if not ch.is_subclass(our_caps.namespace,
            sub=caps.classname,
            super=our_caps.classname):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "Class of \"%s\" must be a sublass of %s" % (
                    prop_name, our_caps.classname))
    if caps['InstanceID'] != our_caps['InstanceID']:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "InstanceID of \"%s\" does not match \"%s\"" %
                prop_name, our_caps['InstanceID'])
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
def get_instance(model=None):
    """
    Makes instance of LMI_SoftwareInstallationServiceCapabilities or
    fills given model with properties.
    """
    path = get_path()
    if model is None:
        model = pywbem.CIMInstance(
                "LMI_SoftwareInstallationServiceCapabilities", path=path)
    model['InstanceID'] = path['InstanceID']

    model['CanAddToCollection'] = True
    model['Caption'] = 'Capabilities of LMI:LMI_SoftwareInstallationService'
    model['Description'] = ('This instance provides information'
        ' about LMI:LMI_SoftwareInstallationService\'s capabilities.')
    model['SupportedAsynchronousActions'] = [
        Values.SupportedAsynchronousActions.Install_From_Software_Identity,
        Values.SupportedAsynchronousActions.Install_from_URI]
    model['SupportedExtendedResourceTypes'] = [
        Values.SupportedExtendedResourceTypes.Linux_RPM]
    model['SupportedInstallOptions'] = [
        Values.SupportedInstallOptions.Install,
        Values.SupportedInstallOptions.Update,
        Values.SupportedInstallOptions.Uninstall,
        Values.SupportedInstallOptions.Force_installation,
        Values.SupportedInstallOptions.Repair]
    model['SupportedSynchronousActions'] = [
        Values.SupportedSynchronousActions.None_supported]
    model['SupportedTargetTypes'] = ['rpm', 'yum']
    model['SupportedURISchemes'] = [
            Values.SupportedURISchemes.file,
            Values.SupportedURISchemes.ftp,
            Values.SupportedURISchemes.http,
            Values.SupportedURISchemes.https]
    return model

