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
#
# Authors: Michal Minar <miminar@redhat.com>
#

"""
Entry module for OpenLMI Software providers.
"""

import logging
from multiprocessing import Queue

from lmi.base import singletonmixin
from lmi.providers import cmpi_logging
from lmi.providers import JobManager
from lmi.providers.IndicationManager import IndicationManager
from lmi.software import util
from lmi.software.core import Job
from lmi.software.LMI_SoftwareIdentity import LMI_SoftwareIdentity
from lmi.software.LMI_SystemSoftwareCollection import \
        LMI_SystemSoftwareCollection
from lmi.software.LMI_HostedSoftwareCollection import \
        LMI_HostedSoftwareCollection
from lmi.software.LMI_MemberOfSoftwareCollection import \
        LMI_MemberOfSoftwareCollection
from lmi.software.LMI_InstalledSoftwareIdentity import \
        LMI_InstalledSoftwareIdentity
from lmi.software.LMI_SoftwareIdentityResource import \
        LMI_SoftwareIdentityResource
from lmi.software.LMI_ResourceForSoftwareIdentity import \
        LMI_ResourceForSoftwareIdentity
from lmi.software.LMI_HostedSoftwareIdentityResource import \
        LMI_HostedSoftwareIdentityResource
from lmi.software.LMI_SoftwareInstallationService import \
        LMI_SoftwareInstallationService
from lmi.software.LMI_SoftwareInstallationServiceCapabilities import \
        LMI_SoftwareInstallationServiceCapabilities
from lmi.software.LMI_AssociatedSoftwareInstallationServiceCapabilities \
        import LMI_AssociatedSoftwareInstallationServiceCapabilities
from lmi.software.LMI_HostedSoftwareInstallationService import \
        LMI_HostedSoftwareInstallationService
from lmi.software.LMI_SoftwareInstallationServiceAffectsElement import \
        LMI_SoftwareInstallationServiceAffectsElement
from lmi.software.LMI_SoftwareJob import LMI_SoftwareJob
from lmi.software.LMI_SoftwareMethodResult import LMI_SoftwareMethodResult
from lmi.software.LMI_AffectedSoftwareJobElement import \
        LMI_AffectedSoftwareJobElement
from lmi.software.LMI_AssociatedSoftwareJobMethodResult import \
        LMI_AssociatedSoftwareJobMethodResult
from lmi.software.LMI_OwningSoftwareJobElement import \
        LMI_OwningSoftwareJobElement
from lmi.software.LMI_SoftwareIdentityFileCheck import \
        LMI_SoftwareIdentityFileCheck
from lmi.software.LMI_SoftwareIdentityChecks import LMI_SoftwareIdentityChecks
from lmi.software.yumdb import jobmanager
from lmi.software.yumdb import YumDB

def get_providers(env):
    """
    *CIMOM* callback.

    :returns: Mapping of provider names to corresponding provider instances.
    :rtype: dictionary
    """
    # first let's setup logging
    cmpi_logging.setup(env, util.Configuration.get_instance())

    # jobmanager does not understand CIM models, give it a way to transform
    # job to CIMIndication instance
    jobmanager.JOB_TO_MODEL = lambda job: Job.job2model(job, keys_only=False)

    providers = {
        "LMI_SoftwareIdentity"            : LMI_SoftwareIdentity(env),
        "LMI_SystemSoftwareCollection"    : LMI_SystemSoftwareCollection(env),
        "LMI_HostedSoftwareCollection"    : LMI_HostedSoftwareCollection(env),
        "LMI_MemberOfSoftwareCollection"  : LMI_MemberOfSoftwareCollection(env),
        "LMI_InstalledSoftwareIdentity"   : LMI_InstalledSoftwareIdentity(env),
        "LMI_SoftwareIdentityResource"    : LMI_SoftwareIdentityResource(env),
        "LMI_ResourceForSoftwareIdentity" :
                LMI_ResourceForSoftwareIdentity(env),
        "LMI_HostedSoftwareIdentityResource" :
                LMI_HostedSoftwareIdentityResource(env),
        "LMI_SoftwareInstallationService" : \
                LMI_SoftwareInstallationService(env),
        "LMI_SoftwareInstallationServiceCapabilities" : \
                LMI_SoftwareInstallationServiceCapabilities(env),
        "LMI_AssociatedSoftwareInstallationServiceCapabilities" : \
                LMI_AssociatedSoftwareInstallationServiceCapabilities(env),
        "LMI_HostedSoftwareInstallationService" : \
                LMI_HostedSoftwareInstallationService(env),
        "LMI_SoftwareInstallationServiceAffectsElement" : \
                LMI_SoftwareInstallationServiceAffectsElement(env),
        "LMI_SoftwareJob"                 : LMI_SoftwareJob(env),
        "LMI_SoftwareInstallationJob"     : LMI_SoftwareJob(env),
        "LMI_SoftwareVerificationJob"     : LMI_SoftwareJob(env),
        "LMI_SoftwareInstCreation"        : LMI_SoftwareJob(env),
        "LMI_SoftwareInstModification"    : LMI_SoftwareJob(env),
        "LMI_SoftwareMethodResult"        : LMI_SoftwareMethodResult(env),
        "LMI_AffectedSoftwareJobElement"  : LMI_AffectedSoftwareJobElement(env),
        "LMI_AssociatedSoftwareJobMethodResult" : \
                LMI_AssociatedSoftwareJobMethodResult(env),
        "LMI_OwningSoftwareJobElement"    : LMI_OwningSoftwareJobElement(env),
        "LMI_SoftwareIdentityFileCheck"   : LMI_SoftwareIdentityFileCheck(env),
        "LMI_SoftwareIdentityChecks"      : LMI_SoftwareIdentityChecks(env)
    }

    # Initialization of indication manager -- running in separate thread as
    # daemon. That means it does not have to be cleaned up.
    try:
        im = IndicationManager.get_instance(
                env, "Software", util.Configuration.get_instance().namespace,
                queue=Queue())
    except singletonmixin.SingletonException:
        logging.getLogger(__name__).warn(
                'IndicationManager instantiated for second time')
    JobManager.register_filters("LMI_SoftwareInstallationJob", im)
    JobManager.register_filters("LMI_SoftwareVerificationJob", im)

    return providers

def authorize_filter(env, fltr, class_name, op, owner):
    """
    *CIMOM* callback.

    It asks us to verify whether this filter is allowed.

    :param string fltr: Contains the filter that must be authorized.
    :param string class_name: Contains the class name extracted out of the
        filter's ``FROM`` clause.
    :param op: The name of the class for which monitoring is required.
        Only the namespace part is set if *class_name* is a process indication.
    :type op: :py:class:`pywbem.CIMInstanceName`
    :param owner: Destination owner.
    """
    IndicationManager.get_instance().authorize_filter(
            env, fltr, class_name, op, owner)

def activate_filter(env, fltr, class_name, class_path, first_activation):
    """
    *CIMOM* callback.

    It ask us to begin monitoring a resource. The function shall begin
    monitoring the resource according to the filter express only.

    :param string fltr: The filter argument contains the filter specification
        for this subscription to become active.
    :param string class_name: The class name extracted out of the filter's
        ``FROM`` clause.
    :param class_path: The name of the class for which monitoring is required.
        Only the namespace part is set if *class_name* is a process indication.
    :type class_path: :py:class:`pywbem.CIMInstanceName`
    :param boolean first_activation: Set to ``True`` if this is the first
        filter for *class_name*.
    """
    IndicationManager.get_instance().activate_filter(
            env, fltr, class_name, class_path, first_activation)

def deactivate_filter(env, fltr, class_name, class_path, last_activation):
    """
    *CIMOM* callback.

    Informs us that monitoring using this filter should stop.

    :param string fltr: The filter argument containing specification
        for a subscription becoming deactivated.
    :param string class_name: The class name extracted out the filter's
        ``FROM`` clause.
    :param class_path: The name of the class for which monitoring is
        required. Only the namespace part is set if className is a process
        indication.
    :type class_path: :py:class:`pywbem.CIMInstanceName`
    :param boolean last_activation: Set to ``True`` if this is the last filter
        for *class_name*.
    """
    IndicationManager.get_instance().deactivate_filter(
            env, fltr, class_name, class_path, last_activation)

def enable_indications(env):
    """
    *CIMOM* callback.

    Tells us that indications can now be generated. The MB is now prepared
    to process indications. The function is normally called by the MB after
    having done its intialization and processing of persistent subscription
    requests.
    """
    IndicationManager.get_instance().enable_indications(env)

def disable_indications(env):
    """
    *CIMOM* callback.

    Tells us that we should stop generating indications. MB will not accept any
    indications until enabled again. The function is normally called when the
    MB is shutting down indication services either temporarily or permanently.
    """
    IndicationManager.get_instance().disable_indications(env)

def can_unload(_env):
    """
    *CIMOM* callback.

    :returns: Whether providers can be unloaded.
    :rtype: boolean
    """
    return True

def shutdown(_env):
    """
    *CIMOM* callback.

    Release resources upon cleanup.
    """
    if YumDB.isInstantiated():
        YumDB.get_instance().clean_up()
    IndicationManager.get_instance().shutdown()
