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
Just a common functionality related to AffectedSoftwareJobElement provider.
"""

import pywbem

from lmi.providers import cmpi_logging
from lmi.providers import ComputerSystem
from lmi.software import util
from lmi.software.core import Identity
from lmi.software.core import IdentityFileCheck
from lmi.software.core import Job
from lmi.software.core import SystemCollection
from lmi.software.yumdb import jobs
from lmi.software.yumdb import PackageInfo

LOG = cmpi_logging.get_logger(__name__)

class Values(object):
    class ElementEffects(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Exclusive_Use = pywbem.Uint16(2)
        Performance_Impact = pywbem.Uint16(3)
        Element_Integrity = pywbem.Uint16(4)
        Create = pywbem.Uint16(5)
        _reverse_map = {
                0: 'Unknown',
                1: 'Other',
                2: 'Exclusive Use',
                3: 'Performance Impact',
                4: 'Element Integrity',
                5: 'Create'
        }

@cmpi_logging.trace_function
def check_path(env, op):
    """
    Checks, whether object path is valid.

    Return internal object representing job and object path of affected element
    as a pair: ``(job, affected)``.
    """
    if not isinstance(op, pywbem.CIMInstanceName):
        raise TypeError("op must be a CIMInstanceName")
    ch = env.get_cimom_handle()

    job = op['AffectingElement'] = Job.object_path2job(op['AffectingElement'])
    affected = op['AffectedElement']
    if ch.is_subclass(affected.namespace, sub=affected.classname,
            super='LMI_SoftwareIdentity'):
        pkg_info = Identity.object_path2pkg(affected, kind='all')
        if isinstance(job, jobs.YumSpecificPackageJob):
            if isinstance(job.pkg, PackageInfo):
                if pkg_info != job.pkg:
                    raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                            "AffectedElement does not match job's package:"
                            " \"%s\" != \"%s\"." % (pkg_info, job.pkg))
            else:
                flt = pkg_info.key_props
                flt.pop('repoid', None)
                if util.nevra2filter(job.pkg) != flt:
                    raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                            "AffectedElement does not match job's package:"
                            " \"%s\" != \"%s\"." % (pkg_info, job.pkg))
            affected = Identity.pkg2model(pkg_info)
        elif isinstance(job, jobs.YumInstallPackageFromURI):
            if job.state == job.COMPLETED:
                affected = Identity.pkg2model(job.result_data)
            else:
                # TODO: this should be somehow obtained from downloaded
                # package before installation
                raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                        "No SoftwareIdentity is associated to given job.")
        else:
            LOG().error("Unsupported async job: %s", job)
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    "No associated SoftwareIdentity.")
    elif ch.is_subclass(affected.namespace, sub=affected.classname,
            super='LMI_SystemSoftwareCollection'):
        SystemCollection.check_path(env, affected, "AffectedElement")
        affected = SystemCollection.get_path()
    elif ch.is_subclass(affected.namespace, sub=affected.classname,
            super="CIM_ComputerSystem"):
        ComputerSystem.check_path(env, affected, "AffectedElement")
        affected = ComputerSystem.get_path(env)
    elif ch.is_subclass(affected.namespace, sub=affected.classname,
            super='LMI_SoftwareIdentityFileCheck'):
        if not isinstance(job, jobs.YumCheckPackage):
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    "Job must point to verification job, not to: \"%s\"" % job)
        if job.state != job.COMPLETED:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    "Associations to failed file checks for verification job"
                    " \"%s\" are not yet known, since job has not completed"
                    " yet." % op["AffectingElement"]["InstanceID"])
        pkg_info, _ = job.result_data
        file_check = IdentityFileCheck.object_path2file_check(affected)
        if file_check.pkg_info.nevra != pkg_info.nevra:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    "Affected element associated to job for another package:"
                    " \"%s\" != \"%s\"." % (file_check.pkg_info, pkg_info))
        if IdentityFileCheck.file_check_passed(file_check):
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    "Given file check reference passed the verification.")
    else:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "Expected an instance of LMI_SoftwareIdentity,"
                " LMI_SystemSoftwareCollection or CIM_ComputerSystem.")
    return (job, affected)

@cmpi_logging.trace_function
def job2affected_software_identity(job):
    """
    @return (paths of affected SoftwareIdentities, ElementEffects array,
        OtherElementEffectsDescriptions array)
    """
    effects = [Values.ElementEffects.Other]
    descriptions = []
    if isinstance(job, jobs.YumSpecificPackageJob):
        affected = []
        if job.state == job.COMPLETED and job.result_data:
            if isinstance(job, jobs.YumCheckPackage):
                # get the first item out of (pkg_info, pkg_check)
                affected = [Identity.pkg2model(job.result_data[0])]
            else:
                if isinstance(job.result_data, (tuple, list, set)):
                    affected = [Identity.pkg2model(p) for p in job.result_data]
                else:
                    affected = [Identity.pkg2model(job.result_data)]
        if job.pkg:
            pkgpath = Identity.pkg2model(job.pkg)
            if pkgpath not in affected:
                affected.append(pkgpath)
        if isinstance(job, jobs.YumInstallPackage):
            descriptions.append("Installing")
        elif isinstance(job, jobs.YumRemovePackage):
            descriptions.append("Removing")
        elif isinstance(job,
                (jobs.YumUpdatePackage, jobs.YumUpdateToPackage)):
            descriptions.append("Updating")
        elif isinstance(job, jobs.YumCheckPackage):
            descriptions.append("Verifying")
        else:
            descriptions.append("Modifying")
            LOG().error("Unhandled job: %s", job)
    elif isinstance(job, jobs.YumInstallPackageFromURI):
        if job.state == job.COMPLETED:
            affected = Identity.pkg2model(job.result_data)
            descriptions.append("Installing")
        else:
            # TODO: this should be somehow obtained from from downloaded
            # package before installation
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    "No SoftwareIdentity is associated to given job.")
    else:
        LOG().error("Unsupported async job: %s", job)
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "No associated SoftwareIdentity.")
    return (affected, effects, descriptions)

@cmpi_logging.trace_function
def fill_model_computer_system(env, model, job, keys_only=True):
    """
    Fills model's AffectedElement and all non-key properties.
    """
    model["AffectedElement"] = ComputerSystem.get_path(env)
    if not keys_only:
        model["ElementEffects"] = [Values.ElementEffects.Other]
        description = "Modifying software collection."
        if isinstance(job, (jobs.YumInstallPackage,
                jobs.YumInstallPackageFromURI)):
            description = "Installing software package to collection."
        elif isinstance(job, jobs.YumRemovePackage):
            description = "Removing package from software collection."
        elif isinstance(job, (jobs.YumUpdatePackage, jobs.YumUpdateToPackage)):
            description = "Updating software package."
        model["OtherElementEffectsDescriptions"] = [description]
    return model

@cmpi_logging.trace_function
def fill_model_system_collection(model, keys_only=True):
    """
    Fills model's AffectedElement and all non-key properties.
    """
    model["AffectedElement"] = SystemCollection.get_path()
    if not keys_only:
        model["ElementEffects"] = [Values.ElementEffects.Exclusive_Use]
        model["OtherElementEffectsDescriptions"] = [
                "Package database is locked."
        ]
    return model

@cmpi_logging.trace_function
def fill_model_failed_check(model, failed_check, keys_only=True):
    """
    Fills model's AffectedElement and all non-key properties.

    :param failed_check: (``CIMInstanceName``) Is on object path of failed
        file check.
    """
    if not isinstance(failed_check, pywbem.CIMInstanceName):
        raise TypeError("failed_check must be a CIMInstanceName")
    model["AffectedElement"] = failed_check
    if not keys_only:
        model["ElementEffects"] = [Values.ElementEffects.Other]
        model["OtherElementEffectsDescriptions"] = [
                "File did not pass the verification."
        ]
    return model

@cmpi_logging.trace_function
def generate_failed_checks(model, job, keys_only=True):
    """
    Generates associations between LMI_SoftwareVerificationJob and
    LMI_SoftwareIdentityFileCheck for files that did not pass the check.
    """
    out_params = Job.get_verification_out_params(job)
    if not "Failed" in out_params:
        return
    for failed in out_params["Failed"].value:
        yield fill_model_failed_check(model, failed, keys_only)

@cmpi_logging.trace_function
def generate_models_from_job(env, job, keys_only=True, model=None):
    """
    Generates all associations between job and affected elements.
    """
    if not isinstance(job, jobs.YumJob):
        raise TypeError("pkg must be an instance of PackageInfo or nevra")
    if model is None:
        model = util.new_instance_name("LMI_AffectedSoftwareJobElement")
        if not keys_only:
            model = pywbem.CIMInstance("LMI_AffectedSoftwareJobElement",
                    path=model)
    model["AffectingElement"] = Job.job2model(job)
    (sis, element_effects, element_effects_descriptions) = \
            job2affected_software_identity(job)

    for si in sis:
        model["AffectedElement"] = si
        if not keys_only:
            model["ElementEffects"] = element_effects
            model["OtherElementEffectsDescriptions"] = \
                    element_effects_descriptions
        yield model

    if not isinstance(job, jobs.YumCheckPackage):
        fill_model_system_collection(model, keys_only=keys_only)
        yield model
    else:   # package verification - associate to failed file checks
        for model in generate_failed_checks(model, job, keys_only=keys_only):
            yield model
    fill_model_computer_system(env, model, job, keys_only=keys_only)
    yield model

