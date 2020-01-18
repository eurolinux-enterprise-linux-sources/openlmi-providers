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
#
# Authors: Michal Minar <miminar@redhat.com>
#

"""Python Provider for LMI_SoftwareIdentityResource

Instruments the CIM class LMI_SoftwareIdentityResource

"""

import pywbem
from pywbem.cim_provider2 import CIMProvider2

from lmi.providers import cmpi_logging
from lmi.software.core import IdentityResource
from lmi.software.yumdb import YumDB, errors

LOG = cmpi_logging.get_logger(__name__)

class LMI_SoftwareIdentityResource(CIMProvider2):
    """Instrument the CIM class LMI_SoftwareIdentityResource

    SoftwareIdentityResource describes the URL of a file or other resource
    that contains all or part of of a SoftwareIdentity for use by the
    SoftwareInstallationService. For example, a CIM_SoftwareIdentity might
    consist of a meta data file, a binary executable file, and a
    installability checker file for some software on a system. This class
    allows a management client to selectively access the constituents of
    the install package to perform a check function, or retrieve some meta
    data information for the install package represented by the
    SoftwareIdentity class without downloading the entire package.
    SoftwareIdentityResources will be related to the SoftwareIdentity
    using the SAPAvailableForElement association.

    """

    def __init__(self, _env):
        self.values = IdentityResource.Values

    @cmpi_logging.trace_method
    def get_instance(self, env, model):
        """Return an instance.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        model -- A template of the pywbem.CIMInstance to be returned.  The
            key properties are set on this instance to correspond to the
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the
            request.  Only properties present in the model need to be
            given values.  If you prefer, you can set all of the
            values, and the instance will be filtered for you.

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized
            or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the CIM Class does exist, but the requested CIM
            Instance does not exist in the specified namespace)
        CIM_ERR_FAILED (some other unspecified error occurred)
        """
        repo = IdentityResource.object_path2repo(env, model.path, kind='all')
        return IdentityResource.repo2model(env, repo, keys_only=False, model=model)

    @cmpi_logging.trace_method
    def enum_instances(self, env, model, keys_only):
        """Enumerate instances.

        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method.
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        model -- A template of the pywbem.CIMInstances to be generated.
            The properties of the model are already filtered according to
            the PropertyList from the request.  Only properties present in
            the model need to be given values.  If you prefer, you can
            always set all of the values, and the instance will be filtered
            for you.
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        Possible Errors:
        CIM_ERR_FAILED (some other unspecified error occurred)

        """
        # Prime model.path with knowledge of the keys, so key values on
        # the CIMInstanceName (model.path) will automatically be set when
        # we set property values on the model.
        model.path.update({'CreationClassName': None, 'SystemName': None,
            'Name': None, 'SystemCreationClassName': None})

        repolist = YumDB.get_instance().get_repository_list('all')
        for repo in repolist:
            yield IdentityResource.repo2model(
                    env, repo, keys_only=keys_only, model=model)

    @cmpi_logging.trace_method
    def set_instance(self, env, instance, modify_existing):
        """Return a newly created or modified instance.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        instance -- The new pywbem.CIMInstance.  If modifying an existing
            instance, the properties on this instance have been filtered by
            the PropertyList from the request.
        modify_existing -- True if ModifyInstance, False if CreateInstance

        Return the new instance.  The keys must be set on the new instance.

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_NOT_SUPPORTED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized
            or otherwise incorrect parameters)
        CIM_ERR_ALREADY_EXISTS (the CIM Instance already exists -- only
            valid if modify_existing is False, indicating that the operation
            was CreateInstance)
        CIM_ERR_NOT_FOUND (the CIM Instance does not exist -- only valid
            if modify_existing is True, indicating that the operation
            was ModifyInstance)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """
        # TODO creation and modification should be supported
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED)

    @cmpi_logging.trace_method
    def delete_instance(self, env, instance_name):
        """Delete an instance.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        instance_name -- A pywbem.CIMInstanceName specifying the instance
            to delete.

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_NOT_SUPPORTED
        CIM_ERR_INVALID_NAMESPACE
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized
            or otherwise incorrect parameters)
        CIM_ERR_INVALID_CLASS (the CIM Class does not exist in the specified
            namespace)
        CIM_ERR_NOT_FOUND (the CIM Class does exist, but the requested CIM
            Instance does not exist in the specified namespace)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """
        # TODO removal should also be supported
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED)

    @cmpi_logging.trace_method
    def cim_method_requeststatechange(self, env, object_name,
                                      param_requestedstate=None,
                                      param_timeoutperiod=None):
        """Implements LMI_SoftwareIdentityResource.RequestStateChange()

        Requests that the state of the element be changed to the value
        specified in the RequestedState parameter. When the requested
        state change takes place, the EnabledState and RequestedState of
        the element will be the same. Invoking the RequestStateChange
        method multiple times could result in earlier requests being
        overwritten or lost.  A return code of 0 shall indicate the state
        change was successfully initiated.  A return code of 3 shall
        indicate that the state transition cannot complete within the
        interval specified by the TimeoutPeriod parameter.  A return code
        of 4096 (0x1000) shall indicate the state change was successfully
        initiated, a ConcreteJob has been created, and its reference
        returned in the output parameter Job. Any other return code
        indicates an error condition.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method RequestStateChange()
            should be invoked.
        param_requestedstate --  The input parameter RequestedState (
                type pywbem.Uint16 Values.RequestStateChange.RequestedState)
            The state requested for the element. This information will be
            placed into the RequestedState property of the instance if the
            return code of the RequestStateChange method is 0 ('Completed
            with No Error'), or 4096 (0x1000) ('Job Started'). Refer to
            the description of the EnabledState and RequestedState
            properties for the detailed explanations of the RequestedState
            values.

        param_timeoutperiod --  The input parameter TimeoutPeriod (
                type pywbem.CIMDateTime)
            A timeout period that specifies the maximum amount of time that
            the client expects the transition to the new state to take.
            The interval format must be used to specify the TimeoutPeriod.
            A value of 0 or a null parameter indicates that the client has
            no time requirements for the transition.  If this property
            does not contain 0 or null and the implementation does not
            support this parameter, a return code of 'Use Of Timeout
            Parameter Not Supported' shall be returned.


        Returns a two-tuple containing the return value (
            type pywbem.Uint32 Values.RequestStateChange)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        Job -- (type REF (pywbem.CIMInstanceName(
                    classname='CIM_ConcreteJob', ...))
            May contain a reference to the ConcreteJob created to track the
            state transition initiated by the method invocation.

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate,
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)
        """
        out_params = []
        if param_timeoutperiod is not None:
            return ( self.values.RequestStateChange. \
                        Use_of_Timeout_Parameter_Not_Supported
                   , out_params)
        if param_requestedstate not in (
                self.values.RequestStateChange.RequestedState.Enabled,
                self.values.RequestStateChange.RequestedState.Disabled ):
            return ( self.values.RequestStateChange.Invalid_State_Transition
                   , out_params)

        try:
            with YumDB.get_instance() as ydb:
                repo = IdentityResource.object_path2repo(env,
                        object_name, 'all')
                enable = param_requestedstate == \
                        self.values.RequestStateChange.RequestedState.Enabled
                LOG().info("%s repository %s" % ("enabling"
                    if enable else "disabling", repo))
                try:
                    prev = ydb.set_repository_enabled(
                            repo, enable)
                except errors.RepositoryChangeError as exc:
                    msg = "failed to modify repository %s: %s" % (repo, str(exc))
                    LOG().error(msg)
                    raise pywbem.CIMError(pywbem.CIM_ERR_FAILED, msg)
                msg = ( "repository %s already %s"
                      if prev == enable else "repository %s %s")
                LOG().info(msg % (repo, "enabled" if enable else "disabled"))
        except errors.RepositoryNotFound as err:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND, err.message)

        rval = self.values.RequestStateChange.Completed_with_No_Error
        return (rval, out_params)

