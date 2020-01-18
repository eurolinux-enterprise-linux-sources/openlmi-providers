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
Just a common functionality related to LMI_SoftwareIdentity provider.
"""

import pywbem

from lmi.providers import cmpi_logging
from lmi.software import util
from lmi.software.yumdb import PackageInfo, YumDB

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

    class Classifications(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Driver = pywbem.Uint16(2)
        Configuration_Software = pywbem.Uint16(3)
        Application_Software = pywbem.Uint16(4)
        Instrumentation = pywbem.Uint16(5)
        Firmware_BIOS = pywbem.Uint16(6)
        Diagnostic_Software = pywbem.Uint16(7)
        Operating_System = pywbem.Uint16(8)
        Middleware = pywbem.Uint16(9)
        Firmware = pywbem.Uint16(10)
        BIOS_FCode = pywbem.Uint16(11)
        Support_Service_Pack = pywbem.Uint16(12)
        Software_Bundle = pywbem.Uint16(13)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..0xFFFF

    class ExtendedResourceType(object):
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

    class CommunicationStatus(object):
        Unknown = pywbem.Uint16(0)
        Not_Available = pywbem.Uint16(1)
        Communication_OK = pywbem.Uint16(2)
        Lost_Communication = pywbem.Uint16(3)
        No_Contact = pywbem.Uint16(4)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..

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

    class PrimaryStatus(object):
        Unknown = pywbem.Uint16(0)
        OK = pywbem.Uint16(1)
        Degraded = pywbem.Uint16(2)
        Error = pywbem.Uint16(3)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..


@cmpi_logging.trace_function
def object_path2nevra(op, with_epoch='NOT_ZERO'):
    """Get nevra out of object path. Also checks for validity."""
    if not isinstance(op, pywbem.CIMInstanceName):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "op must be an instance of CIMInstanceName")

    if (not "InstanceID" in op or not op['InstanceID']):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER, "Wrong keys.")
    instid = op['InstanceID']
    if not instid.lower().startswith("lmi:lmi_softwareidentity:"):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "InstanceID must start with LMI:LMI_SoftwareIdentity: prefix.")
    instid = instid[len("LMI:LMI_SoftwareIdentity:"):]
    match = util.RE_NEVRA_OPT_EPOCH.match(instid)
    if not match:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "Wrong InstanceID. Expected valid nevra"
                ' (name-[epoch:]version-release.arch): "%s".' %
                instid)
    epoch = match.group('epoch')
    if not epoch:
        epoch = "0"
    return util.make_nevra(
            match.group('name'),
            epoch,
            match.group('version'),
            match.group('release'),
            match.group('arch'), with_epoch=with_epoch)

@cmpi_logging.trace_function
def object_path2pkg(op,
        kind='installed',
        include_repos=None,
        exclude_repos=None,
        repoid=None,
        return_all=False):
    """
    @param op must contain precise information of package,
    otherwise an error is raised
    @param kind one of yumdb.jobs.YumGetPackageList.SUPPORTED_KINDS
    says, where to look for given package
    @param repoid if not None, specifies repoid filter on package;
    note, that this does not make sure, that repoid will be enabled.
    @param return_all if True, return list of matching packages as returned
    by YumDB.filter_packages(), otherwise single package is returned
    """
    if not isinstance(kind, basestring):
        raise TypeError("kind must be a string")

    pkglist = YumDB.get_instance().filter_packages(kind,
            allow_duplicates=kind not in ('installed', 'avail_reinst'),
            include_repos=include_repos,
            exclude_repos=exclude_repos,
            repoid=repoid,
            nevra=object_path2nevra(op, 'ALWAYS'))
    if return_all is True:
        return pkglist
    if len(pkglist) > 0:
        return pkglist[0]
    raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
            'No matching package found for InstanceID=\"%s\".' %
            op["InstanceID"])

@cmpi_logging.trace_function
def pkg2model(pkg, keys_only=True, model=None):
    """
    @param pkg can either be an instance of PackageInfo or nevra as string
    @param model if not None, will be filled with data, otherwise
    a new instance of CIMInstance or CIMObjectPath is created
    """
    if not isinstance(pkg, (basestring, PackageInfo)):
        raise TypeError("pkg must be an instance of PackageInfo or nevra")
    if isinstance(pkg, basestring) and not keys_only:
        raise ValueError("can not create instance out of nevra")
    if model is None:
        model = util.new_instance_name("LMI_SoftwareIdentity")
        if not keys_only:
            model = pywbem.CIMInstance("LMI_SoftwareIdentity", path=model)
    nevra = pkg if isinstance(pkg, basestring) else pkg.nevra
    model['InstanceID'] = 'LMI:LMI_SoftwareIdentity:'+nevra
    if not keys_only:
        model.path['InstanceID'] = model['InstanceID']  #pylint: disable=E1103
        model['Caption'] = pkg.summary
        model['Classifications'] = [pywbem.Uint16(0)]
        model['Description'] = pkg.description
        model['ElementName'] = pkg.nevra
        if pkg.installed:
            model['InstallDate'] = util.date_time_to_cim_tz_aware(
                    pkg.install_time)
        else:
            model['InstallDate'] = pywbem.CIMProperty(
                    'InstallDate', None, type='datetime')
        model['IsEntity'] = True
        model['Name'] = pkg.name
        try:
            model["Epoch"] = pywbem.Uint32(int(pkg.epoch))
        except ValueError:
            LOG().error('Could not convert epoch "%s"'
                ' to integer for package \"%s\"!' % (pkg.epoch, pkg))
            model["Epoch"] = pywbem.CIMProperty('Epoch', None, type='uint32')
        model['Version'] = pkg.version
        model['Release'] = pkg.release
        model['Architecture'] = pkg.arch
        model['TargetTypes'] = ['rpm', 'yum']
        model['VersionString'] = pkg.evra
    return model

