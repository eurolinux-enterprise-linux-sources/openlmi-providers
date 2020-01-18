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

"""Python Provider for LMI_SoftwareInstallationJob and
LMI_SoftwareVerificationJob. They will be referred to as LMI_SoftwareJob,
which is their base class.

Instruments the CIM class LMI_SoftwareJob
"""

import pywbem
from pywbem.cim_provider2 import CIMProvider2

from lmi.providers import cmpi_logging
from lmi.software.core import Job
from lmi.software.yumdb import errors, YumDB

LOG = cmpi_logging.get_logger(__name__)

class LMI_SoftwareJob(CIMProvider2):
    """Instrument the CIM class LMI_SoftwareJob

    A concrete version of Job. This class represents a generic and
    instantiable unit of work, such as a batch or a print job.

    """

    def __init__ (self, _env):
        self.values = Job.Values

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
        job = Job.object_path2job(model.path)
        return Job.job2model(job, keys_only=False, model=model)

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
        model.path.update({'InstanceID': None})

        ch = env.get_cimom_handle()
        for job in YumDB.get_instance().get_job_list():
            if ch.is_subclass(model.path.namespace,
                    sub=model.path.classname,
                    super=Job.job_class2cim_class_name(job.__class__)):
                yield Job.job2model(job, keys_only=keys_only, model=model)

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
        if not modify_existing:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED,
                    "Can not create new instance.")
        return Job.modify_instance(instance)

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
        with YumDB.get_instance() as ydb:
            job = Job.object_path2job(instance_name)
            try:
                LOG().info('deleting job "%s"' % job)
                ydb.delete_job(job.jobid)
                LOG().info('job "%s" removed' % job)
            except errors.JobNotFound as exc:
                raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                        getattr(exc, 'message', str(exc)))
            except errors.JobControlError as exc:
                raise pywbem.CIMError(pywbem.CIM_ERR_FAILED,
                        getattr(exc, 'message', str(exc)))

    @cmpi_logging.trace_method
    def cim_method_requeststatechange(self, env, object_name,
                                      param_requestedstate=None,
                                      param_timeoutperiod=None):
        """Implements LMI_SoftwareJob.RequestStateChange()

        Requests that the state of the job be changed to the value
        specified in the RequestedState parameter. Invoking the
        RequestStateChange method multiple times could result in earlier
        requests being overwritten or lost.  If 0 is returned, then the
        task completed successfully. Any other return code indicates an
        error condition.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method RequestStateChange()
            should be invoked.
        param_requestedstate --  The input parameter RequestedState (
                type pywbem.Uint16
                self.Values.RequestStateChange.RequestedState)
            RequestStateChange changes the state of a job. The possible
            values are as follows:  Start (2) changes the state to
            'Running'.  Suspend (3) stops the job temporarily. The
            intention is to subsequently restart the job with 'Start'. It
            might be possible to enter the 'Service' state while
            suspended. (This is job-specific.)  Terminate (4) stops the
            job cleanly, saving data, preserving the state, and shutting
            down all underlying processes in an orderly manner.  Kill (5)
            terminates the job immediately with no requirement to save
            data or preserve the state.  Service (6) puts the job into a
            vendor-specific service state. It might be possible to restart
            the job.

        param_timeoutperiod --  The input parameter TimeoutPeriod (
                type pywbem.CIMDateTime)
            A timeout period that specifies the maximum amount of time that
            the client expects the transition to the new state to take.
            The interval format must be used to specify the TimeoutPeriod.
            A value of 0 or a null parameter indicates that the client has
            no time requirements for the transition.  If this property
            does not contain 0 or null and the implementation does not
            support this parameter, a return code of 'Use Of Timeout
            Parameter Not Supported' must be returned.


        Returns a two-tuple containing the return value (
            type pywbem.Uint32 self.Values.RequestStateChange)
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
        job = Job.object_path2job(object_name)
        try:
            if param_requestedstate != \
                    self.values.RequestStateChange.RequestedState.Terminate:
                raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                        "Valid RequestedState can by only Terminate (%d)" %
                        self.values.RequestStateChange.RequestedState.Terminate)
            if param_timeoutperiod:
                raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                        "Timeout period is not supported.")
            YumDB.get_instance().terminate_job(job.jobid)
        except errors.JobNotFound as exc:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    getattr(exc, 'message', str(exc)))
        except errors.JobControlError as exc:
            raise pywbem.CIMError(pywbem.CIM_ERR_FAILED,
                    getattr(exc, 'message', str(exc)))
        return (self.values.GetErrors.Success, [])

    @cmpi_logging.trace_method
    def cim_method_geterrors(self, env, object_name):
        """Implements LMI_SoftwareJob.GetErrors()

        If JobState is "Completed" and Operational Status is "Completed"
        then no instance of CIM_Error is returned.  If JobState is
        "Exception" then GetErrors may return intances of CIM_Error
        related to the execution of the procedure or method invoked by the
        job. If Operatational Status is not "OK" or "Completed"then
        GetErrors may return CIM_Error instances related to the running of
        the job.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method GetErrors()
            should be invoked.

        Returns a two-tuple containing the return value (
            type pywbem.Uint32 self.Values.GetErrors)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        Errors -- (type pywbem.CIMInstance(classname='CIM_Error', ...))
            If the OperationalStatus on the Job is not "OK", then this
            method will return one or more CIM Error instance(s).
            Otherwise, when the Job is "OK", null is returned.


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
        job = Job.object_path2job(object_name)
        error = Job.job2error(env, job)
        if error is not None:
            param = pywbem.CIMParameter('Errors', type='instance',
                    is_array=True, array_size=1, value=[error])
        else:
            param = pywbem.CIMParameter('Errors', type='instance',
                    is_array=True, array_size=0, value=[])
        return (self.values.GetErrors.Success, [param])

    @cmpi_logging.trace_method
    def cim_method_killjob(self, env, object_name,
                           param_deleteonkill=None):
        """Implements LMI_SoftwareJob.KillJob()

        KillJob is being deprecated because there is no distinction made
        between an orderly shutdown and an immediate kill.
        CIM_ConcreteJob.RequestStateChange() provides 'Terminate' and
        'Kill' options to allow this distinction.  A method to kill this
        job and any underlying processes, and to remove any 'dangling'
        associations.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method KillJob()
            should be invoked.
        param_deleteonkill --  The input parameter DeleteOnKill (type bool)
            Indicates whether or not the Job should be automatically
            deleted upon termination. This parameter takes precedence over
            the property, DeleteOnCompletion.


        Returns a two-tuple containing the return value (
            type pywbem.Uint32 self.Values.KillJob)
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
        raise pywbem.CIMError(pywbem.CIM_ERR_METHOD_NOT_AVAILABLE)

    @cmpi_logging.trace_method
    def cim_method_geterror(self, env, object_name):
        """Implements LMI_SoftwareJob.GetError()

        GetError is deprecated because Error should be an array,not a
        scalar. When the job is executing or has terminated without error,
        then this method returns no CIM_Error instance. However, if the
        job has failed because of some internal problem or because the job
        has been terminated by a client, then a CIM_Error instance is
        returned.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName
            specifying the object on which the method GetError()
            should be invoked.

        Returns a two-tuple containing the return value (
            type pywbem.Uint32 self.Values.GetError)
        and a list of CIMParameter objects representing the output parameters

        Output parameters:
        Error -- (type pywbem.CIMInstance(classname='CIM_Error', ...))
            If the OperationalStatus on the Job is not "OK", then this
            method will return a CIM Error instance. Otherwise, when the
            Job is "OK", null is returned.


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
        job = Job.object_path2job(object_name)
        error = Job.job2error(env, job)
        param = pywbem.CIMParameter('Error', type='instance', value=error)
        return (self.values.GetErrors.Success, [param])
