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

"""Python Provider for LMI_SoftwareInstallationService

Instruments the CIM class LMI_SoftwareInstallationService

"""

import pywbem
from pywbem.cim_provider2 import CIMProvider2

from lmi.providers import cmpi_logging
from lmi.software.core import Identity
from lmi.software.core import IdentityResource
from lmi.software.core import InstallationService
from lmi.software.core import Job
from lmi.software.yumdb import YumDB

LOG = cmpi_logging.get_logger(__name__)

class LMI_SoftwareInstallationService(CIMProvider2):
    """Instrument the CIM class LMI_SoftwareInstallationService

    A subclass of service which provides methods to install (or update)
    Software Identities in ManagedElements.

    """

    def __init__(self, _env):
        self.values = InstallationService.Values

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
        InstallationService.check_path(env, model.path, "path")

        objpath = InstallationService.get_path(env)

        for key, value in objpath.keybindings.items():
            model[key] = value

        model['Caption'] = 'Software installation service for this system.'
        model['CommunicationStatus'] = self.values. \
                CommunicationStatus.Not_Available
        model['Description'] = \
            'Software installation service using YUM packake manager.'
        model['DetailedStatus'] = self.values.DetailedStatus.Not_Available
        model['EnabledDefault'] = self.values.EnabledDefault.Not_Applicable
        model['EnabledState'] = self.values.EnabledState.Not_Applicable
        model['HealthState'] = self.values.HealthState.OK
        model['InstanceID'] = 'LMI:LMI_InstallationService'
        model['OperatingStatus'] = self.values.OperatingStatus.Servicing
        model['OperationalStatus'] = [self.values.OperationalStatus.OK]
        model['PrimaryStatus'] = self.values.PrimaryStatus.OK
        model['RequestedState'] = self.values.RequestedState.Not_Applicable
        model['Started'] = True
        model['TransitioningToState'] = self.values. \
                TransitioningToState.Not_Applicable
        return model

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
        model.path.update({'CreationClassName': None, 'SystemName': None,
            'Name': None, 'SystemCreationClassName': None})

        objpath = InstallationService.get_path(env)
        for key, value in objpath.keybindings.items():
            model[key] = value
        if not keys_only:
            model = self.get_instance(env, model)
        yield model

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
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED)

    @cmpi_logging.trace_method
    def cim_method_requeststatechange(self, env, object_name,
                                      param_requestedstate=None,
                                      param_timeoutperiod=None):
        """Implements LMI_SoftwareInstallationService.RequestStateChange()

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
                type pywbem.Uint16 self.Values.RequestStateChange.RequestedState)
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
                type pywbem.Uint32 self.Values.RequestStateChange)
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
        out_params += [pywbem.CIMParameter('Job', type='reference', value=None)]
        return ( self.values.RequestStateChange.Not_Supported
               , out_params)

    @cmpi_logging.trace_method
    def cim_method_stopservice(self, env, object_name):
        """Implements LMI_SoftwareInstallationService.StopService()

        The StopService method places the Service in the stopped state.
        Note that the function of this method overlaps with the
        RequestedState property. RequestedState was added to the model to
        maintain a record (such as a persisted value) of the last state
        request. Invoking the StopService method should set the
        RequestedState property appropriately. The method returns an
        integer value of 0 if the Service was successfully stopped, 1 if
        the request is not supported, and any other number to indicate an
        error. In a subclass, the set of possible return codes could be
        specified using a ValueMap qualifier on the method. The strings to
        which the ValueMap contents are translated can also be specified
        in the subclass as a Values array qualifier.   Note: The semantics
        of this method overlap with the RequestStateChange method that is
        inherited from EnabledLogicalElement. This method is maintained
        because it has been widely implemented, and its simple "stop"
        semantics are convenient to use.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method StopService()
            should be invoked.

        Returns a two-tuple containing the return value (type pywbem.Uint32)
        and a list of CIMParameter objects representing the output parameters

        Output parameters: none

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
        rval = pywbem.Uint32(1) # Not Supported
        return (rval, [])

    @cmpi_logging.trace_method
    def cim_method_installfromuri(self, env, object_name,
                                  param_installoptionsvalues=None,
                                  param_uri=None,
                                  param_installoptions=None,
                                  param_target=None):
        """Implements LMI_SoftwareInstallationService.InstallFromURI()

        Start a job to install software from a specific URI in a
        ManagedElement.  Note that this method is provided to support
        existing, alternative download mechanisms (such as used for
        firmware download). The 'normal' mechanism will be to use the
        InstallFromSoftwareIdentity method. If 0 is returned, the function
        completed successfully and no ConcreteJob instance was required.
        If 4096/0x1000 is returned, a ConcreteJob will be started to to
        perform the install. The Job's reference will be returned in the
        output parameter Job.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method InstallFromURI()
            should be invoked.
        param_installoptionsvalues --  The input parameter \
                InstallOptionsValues (type [unicode,])
            InstallOptionsValues is an array of strings providing
            additionalinformation to InstallOptions for the method to
            install the software. Each entry of this array is related to
            the entry in InstallOptions that is located at the same index
            providing additional information for InstallOptions.  For
            further information on the use of InstallOptionsValues
            parameter, see the description of the InstallOptionsValues
            parameter of the
            InstallationService.InstallFromSoftwareIdentity
            method.

        param_uri --  The input parameter URI (type unicode)
            A URI for the software based on RFC 2079.

        param_installoptions --  The input parameter InstallOptions (
                type [pywbem.Uint16,] self.Values.InstallFromURI.InstallOptions)
            Options to control the install process.  See the InstallOptions
            parameter of the
            InstallationService.InstallFromSoftwareIdentity method
            for the description of these values.

        param_target --  The input parameter Target (type REF (
                pywbem.CIMInstanceName(classname='CIM_ManagedElement', ...))
            The installation target.


        Returns a two-tuple containing the return value (
                type pywbem.Uint32 self.Values.InstallFromURI)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        Job -- (type REF (pywbem.CIMInstanceName(
                classname='CIM_ConcreteJob', ...))
            Reference to the job (may be null if job completed).


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
        InstallationService.check_path(env, object_name, "object_name")
        out_params = [pywbem.CIMParameter('Job', type='reference', value=None)]
        try:
            jobid = InstallationService.install_or_remove_package(
                    env, Job.JOB_METHOD_INSTALL_FROM_URI,
                    param_uri, param_target, None, param_installoptions,
                    param_installoptionsvalues)
            rval = self.values.InstallFromURI. \
                    Method_Parameters_Checked___Job_Started
            out_params[0].value = Job.job2model(jobid,
                    class_name="LMI_SoftwareInstallationJob")
        except InstallationService.InstallationError as exc:
            LOG().error("installation failed: %s", exc.description)
            rval = exc.return_code
        return (rval, out_params)

    @cmpi_logging.trace_method
    def cim_method_checksoftwareidentity(self, env, object_name,
                                         param_source=None,
                                         param_target=None,
                                         param_collection=None):
        """Implements LMI_SoftwareInstallationService.CheckSoftwareIdentity()

        This method allows a client application to determine whether a
        specific SoftwareIdentity can be installed (or updated) on a
        ManagedElement. It also allows other characteristics to be
        determined such as whether install will require a reboot. In
        addition a client can check whether the SoftwareIdentity can be
        added simulataneously to a specified SofwareIndentityCollection. A
        client MAY specify either or both of the Collection and Target
        parameters. The Collection parameter is only supported if
        SoftwareInstallationServiceCapabilities.CanAddToCollection is
        TRUE.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method CheckSoftwareIdentity()
            should be invoked.
        param_source --  The input parameter Source (type REF (
                pywbem.CIMInstanceName(classname='CIM_SoftwareIdentity', ...))
            Reference to the SoftwareIdentity to be checked.

        param_target --  The input parameter Target (type REF (
                pywbem.CIMInstanceName(classname='CIM_ManagedElement', ...))
            Reference to the ManagedElement that the Software Identity is
            going to be installed in (or updated).

        param_collection --  The input parameter Collection (type REF (
                pywbem.CIMInstanceName(classname='CIM_Collection', ...))
            Reference to the Collection to which the Software Identity will
            be added.

        Returns a two-tuple containing the return value (
            type pywbem.Uint32 self.Values.CheckSoftwareIdentity)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        InstallCharacteristics -- (type [pywbem.Uint16,]
                self.Values.CheckSoftwareIdentity.InstallCharacteristics)
            The parameter describes the characteristics of the
            installation/update that will take place if the Source
            Software Identity is installed:  Target automatic reset: The
            target element will automatically reset once the installation
            is complete.  System automatic reset: The containing system of
            the target ManagedElement (normally a logical device or the
            system itself) will automatically reset/reboot once the
            installation is complete.  Separate target reset required:
            EnabledLogicalElement.RequestStateChange MUST be used to reset
            the target element after the SoftwareIdentity is installed.
            Separate system reset required:
            EnabledLogicalElement.RequestStateChange MUST be used to
            reset/reboot the containing system of the target
            ManagedElement after the SoftwareIdentity is installed.
            Manual Reboot Required: The system MUST be manually rebooted
            by the user.  No reboot required : No reboot is required after
            installation.  User Intervention Recomended : It is
            recommended that a user confirm installation of this
            SoftwareIdentity. Inappropriate application MAY have serious
            consequences.  MAY be added to specified collection : The
            SoftwareIndentity MAY be added to specified Collection.

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
        raise pywbem.CIMError(pywbem.CIM_ERR_METHOD_NOT_AVAILABLE)

    @cmpi_logging.trace_method
    def cim_method_changeaffectedelementsassignedsequence(self,
            env, object_name,
            param_managedelements,
            param_assignedsequence):
        """Implements LMI_SoftwareInstallationService. \
                ChangeAffectedElementsAssignedSequence()

        This method is called to change relative sequence in which order
        the ManagedElements associated to the Service through
        CIM_ServiceAffectsElement association are affected. In the case
        when the Service represents an interface for client to execute
        extrinsic methods and when it is used for grouping of the managed
        elements that could be affected, the ordering represents the
        relevant priority of the affected managed elements with respect to
        each other.  An ordered array of ManagedElement instances is
        passed to this method, where each ManagedElement instance shall be
        already be associated with this Service instance via
        CIM_ServiceAffectsElement association. If one of the
        ManagedElements is not associated to the Service through
        CIM_ServiceAffectsElement association, the implementation shall
        return a value of 2 ("Error Occured").  Upon successful execution
        of this method, if the AssignedSequence parameter is NULL, the
        value of the AssignedSequence property on each instance of
        CIM_ServiceAffectsElement shall be updated such that the values of
        AssignedSequence properties shall be monotonically increasing in
        correlation with the position of the referenced ManagedElement
        instance in the ManagedElements input parameter. That is, the
        first position in the array shall have the lowest value for
        AssignedSequence. The second position shall have the second lowest
        value, and so on. Upon successful execution, if the
        AssignedSequence parameter is not NULL, the value of the
        AssignedSequence property of each instance of
        CIM_ServiceAffectsElement referencing the ManagedElement instance
        in the ManagedElements array shall be assigned the value of the
        corresponding index of the AssignedSequence parameter array. For
        ManagedElements instances which are associated with the Service
        instance via CIM_ServiceAffectsElement and are not present in the
        ManagedElements parameter array, the AssignedSequence property on
        the CIM_ServiceAffects association shall be assigned a value of 0.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method
            ChangeAffectedElementsAssignedSequence() should be invoked.
        param_managedelements --  The input parameter ManagedElements (
                type REF (pywbem.CIMInstanceName(
                    classname='CIM_ManagedElement', ...)) (Required)
            An array of ManagedElements.

        param_assignedsequence --  The input parameter AssignedSequence (
                type [pywbem.Uint16,]) (Required)
            An array of integers representing AssignedSequence for the
            ManagedElement in the corresponding index of the
            ManagedElements parameter.


        Returns a two-tuple containing the return value (
            type pywbem.Uint32 self.Values. \
                    ChangeAffectedElementsAssignedSequence)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        Job -- (type REF (pywbem.CIMInstanceName(
                classname='CIM_ConcreteJob', ...))
            Reference to the job spawned if the operation continues after
            the method returns. (May be null if the task is completed).


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
        raise pywbem.CIMError(pywbem.CIM_ERR_METHOD_NOT_AVAILABLE)

    @cmpi_logging.trace_method
    def cim_method_installfromsoftwareidentity(
            self, env, object_name,
            param_installoptions=None,
            param_target=None,
            param_collection=None,
            param_source=None,
            param_installoptionsvalues=None):
        """Implements LMI_SoftwareInstallationService. \
                InstallFromSoftwareIdentity()

        Start a job to install or update a SoftwareIdentity (Source) on a
        ManagedElement (Target).  In addition the method can be used to
        add the SoftwareIdentity simulataneously to a specified
        SofwareIndentityCollection. A client MAY specify either or both of
        the Collection and Target parameters. The Collection parameter is
        only supported if InstallationService.CanAddToCollection
        is TRUE.  If 0 is returned, the function completed successfully
        and no ConcreteJob instance was required. If 4096/0x1000 is
        returned, a ConcreteJob will be started to perform the install.
        The Job's reference will be returned in the output parameter Job.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method
                InstallFromSoftwareIdentity()
            should be invoked.
        param_installoptions --  The input parameter InstallOptions (
                type [pywbem.Uint16,] self.Values.InstallFromSoftwareIdentity.\
                        InstallOptions)
            Options to control the install process. Defer target/system
            reset : do not automatically reset the target/system. Force
            installation : Force the installation of the same or an older
            SoftwareIdentity. Install: Perform an installation of this
            software on the managed element. Update: Perform an update of
            this software on the managed element. Repair: Perform a repair
            of the installation of this software on the managed element by
            forcing all the files required for installing the software to
            be reinstalled. Reboot: Reboot or reset the system immediately
            after the install or update of this software, if the install
            or the update requires a reboot or reset. Password: Password
            will be specified as clear text without any encryption for
            performing the install or update. Uninstall: Uninstall the
            software on the managed element. Log: Create a log for the
            install or update of the software. SilentMode: Perform the
            install or update without displaying any user interface.
            AdministrativeMode: Perform the install or update of the
            software in the administrative mode. ScheduleInstallAt:
            Indicates the time at which theinstall or update of the
            software will occur.

        param_target --  The input parameter Target (type REF (
                pywbem.CIMInstanceName(classname='CIM_ManagedElement', ...))
            The installation target. If NULL then the SOftwareIdentity will
            be added to Collection only. The underlying implementation is
            expected to be able to obtain any necessary metadata from the
            Software Identity.

        param_collection --  The input parameter Collection (type REF (
                pywbem.CIMInstanceName(classname='CIM_Collection', ...))
            Reference to the Collection to which the Software Identity
            SHALL be added. If NULL then the SOftware Identity will not be
            added to a Collection.

        param_source --  The input parameter Source (type REF (
                pywbem.CIMInstanceName(classname='CIM_SoftwareIdentity', ...))
            Reference to the source of the install.

        param_installoptionsvalues --  The input parameter
                InstallOptionsValues (type [unicode,])
            InstallOptionsValues is an array of strings providing
            additional information to InstallOptions for the method to
            install the software. Each entry of this array is related to
            the entry in InstallOptions that is located at the same index
            providing additional information for InstallOptions.  If the
            index in InstallOptions has the value "Password " then a value
            at the corresponding index of InstallOptionValues shall not be
            NULL.  If the index in InstallOptions has the value
            "ScheduleInstallAt" then the value at the corresponding index
            of InstallOptionValues shall not be NULL and shall be in the
            datetime type format.  If the index in InstallOptions has the
            value "Log " then a value at the corresponding index of
            InstallOptionValues may be NULL.  If the index in
            InstallOptions has the value "Defer target/system reset",
            "Force installation","Install", "Update", "Repair" or "Reboot"
            then a value at the corresponding index of InstallOptionValues
            shall be NULL.


        Returns a two-tuple containing the return value (
            type pywbem.Uint32 self.Values.InstallFromSoftwareIdentity)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        Job -- (type REF (pywbem.CIMInstanceName(
                classname='CIM_ConcreteJob', ...))
            Reference to the job (may be null if job completed).

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
        InstallationService.check_path(env, object_name, "object_name")
        out_params = [pywbem.CIMParameter('Job', type='reference', value=None)]
        try:
            jobid = InstallationService.install_or_remove_package(
                    env, Job.JOB_METHOD_INSTALL_FROM_SOFTWARE_IDENTITY,
                    param_source, param_target, param_collection,
                    param_installoptions,
                    param_installoptionsvalues)
            rval = self.values.InstallFromSoftwareIdentity. \
                    Method_Parameters_Checked___Job_Started
            out_params[0].value = Job.job2model(jobid,
                    class_name="LMI_SoftwareInstallationJob")
        except InstallationService.InstallationError as exc:
            LOG().error("installation failed: %s", exc.description)
            rval = exc.return_code
        return (rval, out_params)

    @cmpi_logging.trace_method
    def cim_method_startservice(self, env, object_name):
        """Implements LMI_SoftwareInstallationService.StartService()

        The StartService method places the Service in the started state.
        Note that the function of this method overlaps with the
        RequestedState property. RequestedState was added to the model to
        maintain a record (such as a persisted value) of the last state
        request. Invoking the StartService method should set the
        RequestedState property appropriately. The method returns an
        integer value of 0 if the Service was successfully started, 1 if
        the request is not supported, and any other number to indicate an
        error. In a subclass, the set of possible return codes could be
        specified using a ValueMap qualifier on the method. The strings to
        which the ValueMap contents are translated can also be specified
        in the subclass as a Values array qualifier.   Note: The semantics
        of this method overlap with the RequestStateChange method that is
        inherited from EnabledLogicalElement. This method is maintained
        because it has been widely implemented, and its simple "start"
        semantics are convenient to use.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method StartService()
            should be invoked.

        Returns a two-tuple containing the return value (type pywbem.Uint32)
        and a list of CIMParameter objects representing the output parameters

        Output parameters: none

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
        rval = pywbem.Uint32(1)     # Not Supported
        return (rval, [])

    @cmpi_logging.trace_method
    def cim_method_installfrombytestream(self, env, object_name,
                                         param_installoptionsvalues=None,
                                         param_image=None,
                                         param_installoptions=None,
                                         param_target=None):
        """Implements LMI_SoftwareInstallationService.InstallFromByteStream()

        Start a job to download a series of bytes containing a software
        image to a ManagedElement.  Note that this method is provided to
        support existing, alternative download mechanisms (such as used
        for firmware download). The 'normal' mechanism will be to use the
        InstallFromSoftwareIdentity method.  If 0 is returned, the
        function completed successfully and no ConcreteJob instance was
        required. If 4096/0x1000 is returned, a ConcreteJob will be
        started to to perform the install. The Job's reference will be
        returned in the output parameter Job.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method InstallFromByteStream()
            should be invoked.
        param_installoptionsvalues --  The input parameter InstallOptionsValues (type [unicode,])
            InstallOptionsValues is an array of strings providing
            additional information to InstallOptions for the method to
            install the software. Each entry of this array is related to
            the entry in InstallOptions that is located at the same index
            providing additional information for InstallOptions.   For
            further information on the use of InstallOptionsValues
            parameter, see the description of the InstallOptionsValues
            parameter of the
            InstallationService.InstallFromSoftwareIdentity
            method.

        param_image --  The input parameter Image (type [pywbem.Uint8,])
            A array of bytes containing the install image.

        param_installoptions --  The input parameter InstallOptions (type [pywbem.Uint16,] self.Values.InstallFromByteStream.InstallOptions)
            Options to control the install process.  See the InstallOptions
            parameter of the
            InstallationService.InstallFromSoftwareIdentity method
            for the description of these values.

        param_target --  The input parameter Target (type REF (pywbem.CIMInstanceName(classname='CIM_ManagedElement', ...))
            The installation target.


        Returns a two-tuple containing the return value (type pywbem.Uint32 self.Values.InstallFromByteStream)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        Job -- (type REF (pywbem.CIMInstanceName(classname='CIM_ConcreteJob', ...))
            Reference to the job (may be null if job completed).


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
        out_params = [pywbem.CIMParameter('Job', type='reference', value=None)]
        return ( self.values.InstallFromByteStream.Not_Supported
               , out_params)

    @cmpi_logging.trace_method
    def cim_method_verifyinstalledidentity(self, env, object_name,
                                           param_source=None,
                                           param_target=None):
        """Implements LMI_SoftwareInstallationService. \
                VerifyInstalledIdentity()

        Start a job to verify installed package represented by
        SoftwareIdentity (source) (Source) on a ManagedElement (Target).
        If 0 is returned, the function completed successfully and no
        ConcreteJob instance was required. If 4096/0x1000 is returned, a
        ConcreteJob will be started to perform the verification. The Job's
        reference will be returned in the output parameter Job. In former
        case, the Failed parameterwill contain all associated file checks,
        that did not pass. In the latter case this property will be NULL.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method VerifyInstalledIdentity()
            should be invoked.
        param_source --  The input parameter Source (
                type REF (pywbem.CIMInstanceName(
                    classname='LMI_SoftwareIdentity', ...))
            Reference to the installed SoftwareIdentity to be verified.

        param_target --  The input parameter Target (
                type REF (pywbem.CIMInstanceName(
                    classname='CIM_ManagedElement', ...))
            Reference to the ManagedElement that the Software Identity is
            installed on.


        Returns a two-tuple containing the return value (
            type pywbem.Uint32 self.Values.VerifyInstalledIdentity)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        Job -- (type REF (pywbem.CIMInstanceName(
                classname='LMI_SoftwareVerificationJob', ...))
            Reference to the job (may be null if job completed).

        Failed -- (type REF (pywbem.CIMInstanceName(
                classname='LMI_SoftwareIdentityFileCheck', ...))
            Array of file checks that did not pass verification. This is
            NULL in case that asynchronous job has been started.


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
        InstallationService.check_path(env, object_name, "object_name")
        out_params = [pywbem.CIMParameter('Job', type='reference', value=None)]
        try:
            jobid = InstallationService.verify_package(
                    env, Job.JOB_METHOD_VERIFY_INSTALLED_IDENTITY,
                    param_source, param_target)
            rval = self.values.VerifyInstalledIdentity. \
                    Method_Parameters_Checked___Job_Started
            out_params[0].value = Job.job2model(jobid,
                    class_name="LMI_SoftwareVerificationJob")
        except InstallationService.InstallationError as exc:
            LOG().error("failed to launch verification job: %s",
                    exc.description)
            rval = exc.return_code
        return (rval, out_params)

    @cmpi_logging.trace_method
    def cim_method_findidentity(self, env, object_name,
            param_name=None,
            param_epoch=None,
            param_version=None,
            param_release=None,
            param_architecture=None,
            param_allowduplicates=None,
            param_exactmatch=None,
            param_repository=None):
        """Implements LMI_SoftwareInstallationService.FindIdentity()

        Search for installed or available software identity matching
        specified properties. In case "Repository" is given, only
        available packages of this repository will be browsed.
        "AllowDuplicates" causes, that packages of the name <name>.<arch>
        will be listed multiple times if more versions are available.
        Other input parameters with non-NULL values are compared to
        corresponding properties of LMI_SoftwareIdentity instances. 0 is
        returned if any matching package is found, 1 otherwise.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method FindIdentity()
            should be invoked.
        param_name --  The input parameter Name (type unicode)
        param_repository --  The input parameter Repository (
                type REF (pywbem.CIMInstanceName(
                    classname='LMI_SoftwareIdentityResource', ...))
            Allows to specify particular software repository, where the
            search shall take place. If given, only available packages
            will be browsed.

        param_epoch --  The input parameter Epoch (type pywbem.Uint32)
        param_version --  The input parameter Version (type unicode)
        param_architecture --  The input parameter Architecture (type unicode)
        param_allowduplicates --  The input parameter AllowDuplicates (
                type bool)
            Whether the different versions of the same package shall be
            included in result. This defaults to "False".
        param_exactmatch --  The input parameter ExactMatch (type bool)
            Whether to compare "Name" for exact match. If "False", package name
            and its summary string ("Caption") will be searched for occurences
            of "Name" parameter's value. Defaults to "True".
        param_release --  The input parameter Release (type unicode)

        Returns a two-tuple containing the return value (type pywbem.Uint32)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        Matches -- (type REF (pywbem.CIMInstanceName(
                classname='LMI_SoftwareIdentity', ...))
            All matching packages found shall be available in this
            parameter.

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
        InstallationService.check_path(env, object_name, "object_name")
        out_params = [pywbem.CIMParameter(
            'Matches', type='reference', value=None, is_array=True)]
        kwargs = {
                'name'             : param_name,
                'epoch'            : param_epoch,
                'version'          : param_version,
                'release'          : param_release,
                'arch'             : param_architecture,
                'allow_duplicates' : param_allowduplicates,
                'exact_match'      : param_exactmatch,
                'sort'             : True,
        }
        if param_repository is not None:
            repo = IdentityResource.object_path2repo(env, param_repository)
            kwargs['repoid'] = repo.repoid
        pkgs = YumDB.get_instance().filter_packages('all', **kwargs)
        out_params[0].value = [Identity.pkg2model(p) for p in pkgs]
        if len(pkgs) > 0:
            rval = self.values.FindIdentity.Found
        else:
            rval = self.values.FindIdentity.NoMatch
        return (rval, out_params)

