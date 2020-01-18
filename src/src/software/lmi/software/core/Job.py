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
Just a common functionality related to SoftwareInstallationJob
and SoftwareVerificationJob.
"""

from datetime import datetime, timedelta
import pywbem
import time

from lmi.providers import cmpi_logging
from lmi.software import util
from lmi.software.core import Error
from lmi.software.yumdb import errors
from lmi.software.yumdb import jobs
from lmi.software.yumdb import YumDB

JOB_CLASS_NAMES = (
    "LMI_SoftwareInstallationJob",
    "LMI_SoftwareVerificationJob"
)

JOB_DESCRIPTIONS = {
    jobs.YumInstallPackage :
        'Software package installation job %(jobid)d for "%(pkg)s".',
    jobs.YumRemovePackage :
        'Software package removal job %(jobid)d for "%(pkg)s".',
    jobs.YumUpdateToPackage :
        'Software package update job %(jobid)d for "%(pkg)s".',
    jobs.YumUpdatePackage :
        'Software package update job %(jobid)d for "%(pkg)s".',
    jobs.YumInstallPackageFromURI :
        'Software package installation job %(jobid)d from uri: "%(uri)s".',
    jobs.YumCheckPackage :
        'Software package check job %(jobid)d for "%(pkg)s".',
    jobs.YumCheckPackageFile :
        'File verification job %(jobid)d for package "%(pkg)s".',
}

# identificators of InstallationService method, which may trigger
# creation of asynchronous job
( JOB_METHOD_INSTALL_FROM_SOFTWARE_IDENTITY
, JOB_METHOD_INSTALL_FROM_URI
, JOB_METHOD_INSTALL_FROM_BYTE_STREAM
, JOB_METHOD_VERIFY_INSTALLED_IDENTITY) = range(4)

# above identificators point to this array to their description
JOB_METHOD_NAMES = (
        "InstallFromSoftwareIdentity",
        "InstallFromURI",
        "InstallFromByteStream",
        "VerifyInstalledIdentity")

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
                0: 'Not Available',
                1: 'No Additional Information',
                2: 'Stressed',
                3: 'Predictive Failure',
                4: 'Non-Recoverable Error',
                5: 'Supporting Entity in Error'
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

    class JobState(object):
        New = pywbem.Uint16(2)
        Starting = pywbem.Uint16(3)
        Running = pywbem.Uint16(4)
        Suspended = pywbem.Uint16(5)
        Shutting_Down = pywbem.Uint16(6)
        Completed = pywbem.Uint16(7)
        Terminated = pywbem.Uint16(8)
        Killed = pywbem.Uint16(9)
        Exception = pywbem.Uint16(10)
        Service = pywbem.Uint16(11)
        Query_Pending = pywbem.Uint16(12)
        # DMTF_Reserved = 13..32767
        # Vendor_Reserved = 32768..65535
        _reverse_map = {
                2: 'New',
                3: 'Starting',
                4: 'Running',
                5: 'Suspended',
                6: 'Shutting Down',
                7: 'Completed',
                8: 'Terminated',
                9: 'Killed',
                10: 'Exception',
                11: 'Service',
                12: 'Query Pending'
        }

    class GetError(object):
        Success = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Unspecified_Error = pywbem.Uint32(2)
        Timeout = pywbem.Uint32(3)
        Failed = pywbem.Uint32(4)
        Invalid_Parameter = pywbem.Uint32(5)
        Access_Denied = pywbem.Uint32(6)
        # DMTF_Reserved = ..
        # Vendor_Specific = 32768..65535

    class KillJob(object):
        Success = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Unknown = pywbem.Uint32(2)
        Timeout = pywbem.Uint32(3)
        Failed = pywbem.Uint32(4)
        Access_Denied = pywbem.Uint32(6)
        Not_Found = pywbem.Uint32(7)
        # DMTF_Reserved = ..
        # Vendor_Specific = 32768..65535

    class RecoveryAction(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Do_Not_Continue = pywbem.Uint16(2)
        Continue_With_Next_Job = pywbem.Uint16(3)
        Re_run_Job = pywbem.Uint16(4)
        Run_Recovery_Job = pywbem.Uint16(5)
        _reverse_map = {
                0: 'Unknown',
                1: 'Other',
                2: 'Do Not Continue',
                3: 'Continue With Next Job',
                4: 'Re-run Job',
                5: 'Run Recovery Job'
        }

    class RunDayOfWeek(object):
        _Saturday = pywbem.Sint8(-7)
        _Friday = pywbem.Sint8(-6)
        _Thursday = pywbem.Sint8(-5)
        _Wednesday = pywbem.Sint8(-4)
        _Tuesday = pywbem.Sint8(-3)
        _Monday = pywbem.Sint8(-2)
        _Sunday = pywbem.Sint8(-1)
        ExactDayOfMonth = pywbem.Sint8(0)
        Sunday = pywbem.Sint8(1)
        Monday = pywbem.Sint8(2)
        Tuesday = pywbem.Sint8(3)
        Wednesday = pywbem.Sint8(4)
        Thursday = pywbem.Sint8(5)
        Friday = pywbem.Sint8(6)
        Saturday = pywbem.Sint8(7)
        _reverse_map = {
                0: 'ExactDayOfMonth',
                1: 'Sunday',
                2: 'Monday',
                3: 'Tuesday',
                4: 'Wednesday',
                5: 'Thursday',
                6: 'Friday',
                7: 'Saturday',
                -1: '-Sunday',
                -7: '-Saturday',
                -6: '-Friday',
                -5: '-Thursday',
                -4: '-Wednesday',
                -3: '-Tuesday',
                -2: '-Monday'
        }

    class RunMonth(object):
        January = pywbem.Uint8(0)
        February = pywbem.Uint8(1)
        March = pywbem.Uint8(2)
        April = pywbem.Uint8(3)
        May = pywbem.Uint8(4)
        June = pywbem.Uint8(5)
        July = pywbem.Uint8(6)
        August = pywbem.Uint8(7)
        September = pywbem.Uint8(8)
        October = pywbem.Uint8(9)
        November = pywbem.Uint8(10)
        December = pywbem.Uint8(11)
        _reverse_map = {
                0: 'January',
                1: 'February',
                2: 'March',
                3: 'April',
                4: 'May',
                5: 'June',
                6: 'July',
                7: 'August',
                8: 'September',
                9: 'October',
                10: 'November',
                11: 'December'
        }

    class GetErrors(object):
        Success = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Unspecified_Error = pywbem.Uint32(2)
        Timeout = pywbem.Uint32(3)
        Failed = pywbem.Uint32(4)
        Invalid_Parameter = pywbem.Uint32(5)
        Access_Denied = pywbem.Uint32(6)
        # DMTF_Reserved = ..
        # Vendor_Specific = 32768..65535

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

    class LocalOrUtcTime(object):
        Local_Time = pywbem.Uint16(1)
        UTC_Time = pywbem.Uint16(2)
        _reverse_map = {
                1: 'Local Time',
                2: 'UTC Time'
        }

    class RequestStateChange(object):
        Completed_with_No_Error = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Unknown_Unspecified_Error = pywbem.Uint32(2)
        Can_NOT_complete_within_Timeout_Period = pywbem.Uint32(3)
        Failed = pywbem.Uint32(4)
        Invalid_Parameter = pywbem.Uint32(5)
        In_Use = pywbem.Uint32(6)
        # DMTF_Reserved = ..
        Method_Parameters_Checked___Transition_Started = pywbem.Uint32(4096)
        Invalid_State_Transition = pywbem.Uint32(4097)
        Use_of_Timeout_Parameter_Not_Supported = pywbem.Uint32(4098)
        Busy = pywbem.Uint32(4099)
        # Method_Reserved = 4100..32767
        # Vendor_Specific = 32768..65535
        class RequestedState(object):
            Start = pywbem.Uint16(2)
            Suspend = pywbem.Uint16(3)
            Terminate = pywbem.Uint16(4)
            Kill = pywbem.Uint16(5)
            Service = pywbem.Uint16(6)
            # DMTF_Reserved = 7..32767
            # Vendor_Reserved = 32768..65535

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

@cmpi_logging.trace_function
def get_verification_out_params(job):
    """
    Get the output parameters for verification job. They may not be computed
    yet. In that case compute them a update the job in YumWorker process.

    :param job: (``jobs.YumCheckPackage``)
    :rtype: (``dict``) Dictionary of output parameters with pywbem values.
    """
    if not isinstance(job, jobs.YumCheckPackage):
        raise TypeError("job must be a YumCheckPackage instance")
    if (   not job.metadata.get("output_params", [])
       and job.state == job.COMPLETED):
        from lmi.software.core import IdentityFileCheck
        failed = []
        pkg_info, pkg_check = job.result_data
        for file_name in pkg_check:
            pkg_file = pkg_check[file_name]
            file_check = IdentityFileCheck.test_file(pkg_info,
                    pkg_check.file_checksum_type, pkg_file)
            if not IdentityFileCheck.file_check_passed(file_check):
                failed.append(IdentityFileCheck.file_check2model(
                    file_check, job=job))
        metadata = {
            'output_params' : {
                'Failed' : pywbem.CIMProperty("Failed",
                    type="reference", is_array=True, value=failed)
            }
        }
        # update local instance
        job.update(metadata=metadata)
    return job.metadata.get('output_params', [])

@cmpi_logging.trace_function
def make_method_params(job, class_name, include_input, include_output):
    """
    Create a class of given name with all input or output parameters
    of the asynchronous method. Typically used to assemble
    CIM_ConcreteJob.JobInParameters or CIM_InstMethodCall.MethodParameters
    values.

    :param job: (``YumJob``) Instance of job created as a result of method
        invocation. It carries method parameters.
    :param class_name: (``str``) Name of the class to create.
    :param include_input: (``bool``) Whether input parameters should be
        included in the returned class.
    :param include_output: (``bool``) Whether output parameters should be
        included in the returned class.
    :rtype: CIMInstance of the created class.
    """
    path = util.new_instance_name(class_name)
    inst = pywbem.CIMInstance(classname=class_name, path=path)
    if include_input and "input_params" in job.metadata:
        for (name, value) in job.metadata["input_params"].items():
            inst[name] = value
    if include_output:
        if isinstance(job, jobs.YumCheckPackage):
            # make sure, that output parameters are computed
            # TODO: uncomment this, when pegasus properly handles instances
            # of unknown classes - we can not create Failed property, which
            # is an array of references in association class representing
            # result of asynchronous method
            #get_verification_out_params(job)
            pass
        if "output_params" in job.metadata:
            # overwrite any input parameter
            for (name, value) in job.metadata["output_params"].iteritems():
                inst[name] = value
        return_value = make_return_value(job)
        if return_value is not None:
            inst["__ReturnValue"] = return_value
    return inst

def make_return_value(job):
    """
    Compute return value of particular job.

    :param job: (``jobs.YumAsyncJob``) Asynchronous job.
    :rtype: (``pywbem.Uint32``) Return value of asynchronous method or ``None``
        if job has not completed yet.
    """
    from lmi.software.core import InstallationService

    if not isinstance(job, jobs.YumAsyncJob):
        raise TypeError("job must be a YumAsyncJob")

    if job.state == job.COMPLETED:
        return InstallationService. \
                Values.InstallFromURI.Job_Completed_with_No_Error
    if job.state == job.EXCEPTION:
        if issubclass(job.result_data[0], (
                errors.InvalidNevra, errors.InvalidURI,
                errors.PackageNotFound)):
            return InstallationService.Values.InstallFromURI.Invalid_Parameter
        else:
            return InstallationService.Values.InstallFromURI.Failed
    if job.state == job.TERMINATED:
        return InstallationService.Values.InstallFromURI.Unspecified_Error

    # job has not finished yet
    return None

def job_class2cim_class_name(jobcls):
    """
    Here we map classes of job objects to their corresponding CIM class
    name.

    :param jobcls: (``type``) Subclass of jobs.YumJob.
    """
    if not issubclass(jobcls, (
            jobs.YumInstallPackageFromURI,
            jobs.YumSpecificPackageJob)):
        raise ValueError("Job class \"%s\" does not have any associated"
            " CIM class." % jobcls.__name__)
    if issubclass(jobcls, (jobs.YumCheckPackage, jobs.YumCheckPackageFile)):
        return "LMI_SoftwareVerificationJob"
    return "LMI_SoftwareInstallationJob"

@cmpi_logging.trace_function
def _fill_nonkeys(job, model):
    """
    Fills into the model of instance all non-key properties.
    """
    model['Caption'] = 'Software installation job with id=%d' % job.jobid
    model['CommunicationStatus'] = Values.CommunicationStatus.Not_Available
    model['DeleteOnCompletion'] = job.delete_on_completion
    try:
        description = JOB_DESCRIPTIONS[job.__class__]
        kwargs = job.job_kwargs
        kwargs['jobid'] = job.jobid
        model['Description'] = description % kwargs
    except KeyError:
        LOG().error('no description string found for job class %s',
                job.__class__.__name__)
        model['Description'] = pywbem.CIMProperty('Description',
                type='string', value=None)
    if job.started:
        if job.finished:
            elapsed = job.finished - job.started
        else:
            elapsed = time.time() - job.started
        model['ElapsedTime'] = pywbem.CIMDateTime(timedelta(seconds=elapsed))
    else:
        model["ElapsedTime"] = pywbem.CIMProperty('ElapsedTime',
                type='datetime', value=None)
    model['ErrorCode'] = pywbem.Uint16(0 if job.state != job.EXCEPTION else 1)
    try:
        model['JobState'], model['OperationalStatus'], model['JobStatus'] = {
            jobs.YumJob.NEW        : (Values.JobState.New,
                [Values.OperationalStatus.Dormant], 'Enqueued'),
            jobs.YumJob.RUNNING    : (Values.JobState.Running,
                [Values.OperationalStatus.OK], 'Running'),
            jobs.YumJob.TERMINATED : (Values.JobState.Terminated,
                [Values.OperationalStatus.Stopped], 'Terminated'),
            jobs.YumJob.EXCEPTION  : (Values.JobState.Exception
                , [Values.OperationalStatus.Error]
                , 'Failed'),
            jobs.YumJob.COMPLETED  : (Values.JobState.Completed
                , [ Values.OperationalStatus.OK
                  , Values.OperationalStatus.Completed]
                , 'Finished successfully')
            }[job.state]
    except KeyError:
        LOG().error('unknown job state: %s', job.state)
        model['JobState'] = pywbem.CIMProperty('JobState',
                type='uint16', value=None)
        model['OperationalStatus'] = [Values.OperationalStatus.Unknown]
        model['JobStatus'] = 'Unknown'
    # TODO: uncomment this, when Pegasus propertly supports instances of
    # unknown classes
    #model["JobInParameters"] = make_method_params(
    #    job, "__JobInParameters", True, False)
    method_name = JOB_METHOD_NAMES[job.metadata["method"]]
    model["JobOutParameters"] = make_method_params(
        job, "__MethodParameters_"+method_name+"_Result", False, True)
    if 'method' in job.metadata:
        model['MethodName'] = method_name
    else:
        model["MethodName"] = pywbem.CIMProperty('MethodName',
                type='string', value=None)
    model['Name'] = job.metadata['name']
    model['LocalOrUtcTime'] = Values.LocalOrUtcTime.UTC_Time
    model['PercentComplete'] = pywbem.Uint16(
            100 if job.state == job.COMPLETED else (
            50  if job.state == job.RUNNING   else
            0))
    model['Priority'] = pywbem.Uint32(job.priority)
    if job.started:
        model['StartTime'] = pywbem.CIMDateTime(datetime.fromtimestamp(
                job.started))
    model['TimeBeforeRemoval'] = pywbem.CIMDateTime(timedelta(
            seconds=job.time_before_removal))
    model['TimeOfLastStateChange'] = pywbem.CIMDateTime(datetime.fromtimestamp(
            job.last_change))
    model['TimeSubmitted'] = pywbem.CIMDateTime(datetime.fromtimestamp(
            job.created))

@cmpi_logging.trace_function
def job2model(job, class_name=None, keys_only=True, model=None):
    """
    Makes LMI_SoftwareJob out of job object or job id.

    :param job: (``int`` | ``YumAsyncJob``) Job identifier.
        In case of integer, caller should also provide class_name of resulting
        CIM instance. Otherwise generic LMI_SoftwareJob will be returned.
    :param class_name: (``str``) Determines CIM class name of resulting
        instance. This should be given when ``job`` is an integer.
    :param model: (``CIMInstance`` | ``CIMInstanceName``) If not None,
        will be filled with properties, otherwise
        a new instance of CIMInstance or CIMObjectPath is created.
    :param keys_only: (``bool``) Says whether to fill only key properties.
        Also if ``model`` is not given, it determines, whether to make
        ``CIMInstanceName`` or ``CIMInstance``.
    :rtype: (``CIMInstance`` | ``CIMInstanceName``)
    """
    if not isinstance(job, (int, long, jobs.YumAsyncJob)):
        raise TypeError("job must be an instance of YumAsyncJob")
    if isinstance(job, jobs.YumAsyncJob) and not job.async:
        raise ValueError("job must be asynchronous")
    if not keys_only and isinstance(job, (int, long)):
        raise TypeError("job must be an instance of YumAsyncJob"
            " filling non-key properties")

    if class_name is None:
        if  model is not None:
            class_name = model.classname
        elif isinstance(job, jobs.YumJob):
            class_name = job_class2cim_class_name(job.__class__)
        else:
            class_name = "LMI_SoftwareJob"
            LOG().warn("class_name not supplied for job %s, using general"
                    " LMI_SoftwareJob as CIM class name", job)
    if model is None:
        model = util.new_instance_name(class_name)
        if not keys_only:
            model = pywbem.CIMInstance(class_name, path=model)

    jobid = job.jobid if isinstance(job, jobs.YumAsyncJob) else job
    model['InstanceID'] = 'LMI:%s:%d' % (class_name, jobid)
    if isinstance(model, pywbem.CIMInstance):
        model.path['InstanceID'] = model['InstanceID']  #pylint: disable=E1103

    if not keys_only:
        _fill_nonkeys(job, model)
    return model

@cmpi_logging.trace_function
def object_path2job(op):
    """
    @param op must contain precise InstanceID of job
    """
    if not isinstance(op, pywbem.CIMInstanceName):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "op must be an instance of CIMInstanceName")

    if (not "InstanceID" in op or not op['InstanceID']):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "Missing InstanceID key property.")
    instid = op['InstanceID']
    match = util.RE_INSTANCE_ID.match(instid)
    if not match or match.group('clsname').lower() not in {
            c.lower() for c in JOB_CLASS_NAMES}:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
            "InstanceID must start with one of {%s} prefixes."
            " And end with positive integer." % (
                ", ".join(("LMI:%s:" % cn) for cn in JOB_CLASS_NAMES)))

    instid = int(match.group('id'))
    try:
        job = YumDB.get_instance().get_job(instid)
        clsname = job_class2cim_class_name(job.__class__)
        if (   clsname.lower() != op.classname.lower()
           and op.classname.lower() != 'LMI_SoftwareJob'.lower()):
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "Classname \"%s\" does not belong to job with given id."
                " \"%s\" is the correct one." % (op.classname, clsname))
        return job
    except errors.JobNotFound:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
            'No such job "%s".' % op['InstanceID'])

@cmpi_logging.trace_function
def modify_instance(instance):
    """
    This call modifies the job's parameters according to given instance.
    """
    job = object_path2job(instance.path)
    ydb = YumDB.get_instance()
    update_kwargs = {}
    reschedule_kwargs = {}
    # all modifiable properties
    prop_name_map = {
            "name"               : "name",
            "priority"           : "priority",
            "deleteoncompletion" : "delete_on_completion",
            "timebeforeremoval"  : "time_before_removal"
    }
    metadata_props = {"name"}
    reschedule_props = {"delete_on_completion", "time_before_removal"}
    for name, prop in instance.properties.items():
        if prop is None:
            LOG().warn('property "%s" is None', name)
            continue
        name = name.lower()
        try:
            pname = prop_name_map[name]
            if pname == "priority" and job.priority != prop.value:
                LOG().info('changing priority of job %s to %d', job, prop.value)
                job = ydb.set_job_priority(job.jobid, prop.value)
            elif pname in reschedule_props:
                if getattr(job, pname) == prop.value:
                    continue
                if pname == "time_before_removal":
                    value = prop.value.timedelta.total_seconds()
                else:
                    value = prop.value
                reschedule_kwargs[pname] = value
            else:
                if pname in metadata_props:
                    if not 'metadata' in update_kwargs:
                        update_kwargs['metadata'] = {}
                    update_kwargs['metadata'][pname] = prop.value
                else:
                    update_kwargs[pname] = prop.value
        except KeyError:
            if name == 'instanceid':
                continue
            LOG().warn("skipping property %s: %s", name, prop)

    if reschedule_kwargs:
        for prop in ('delete_on_completion', 'time_before_removal'):
            if prop not in reschedule_kwargs:
                reschedule_kwargs[prop] = getattr(job, prop)
        LOG().info('rescheduling job %s to: %s', job,
            ", ".join("%s=%s"%(k, v) for k, v in reschedule_kwargs.items()))
        job = ydb.reschedule_job(job.jobid, **reschedule_kwargs)

    if update_kwargs:
        LOG().info('changing atributes of job %s to: %s', job,
                ", ".join("%s=%s"%(k, v) for k, v in update_kwargs.items()))
        job = ydb.update_job(job.jobid, **update_kwargs)

    return job2model(job, keys_only=False, model=instance)

@cmpi_logging.trace_function
def job2error(env, job):
    """
    @return instance of CIM_Error if job is in EXCEPTION state,
    None otherwise
    """
    if not isinstance(job, jobs.YumJob):
        raise TypeError("job must be isntance of YumJob")
    if job.state == job.EXCEPTION:
        errortup = job.result_data
        kwargs = {}
        if issubclass(errortup[0],
                (errors.RepositoryNotFound, errors.PackageNotFound)):
            kwargs['status_code'] = Error.Values. \
                    CIMStatusCode.CIM_ERR_NOT_FOUND
            if issubclass(errortup[0], errors.PackageNotFound):
                kwargs['status_code_description'] = "Package not found"
            else:
                kwargs['status_code_description'] = "Repository not found"
        elif issubclass(errortup[0], errors.PackageAlreadyInstalled):
            kwargs['status_code'] = Error.Values. \
                    CIMStatusCode.CIM_ERR_ALREADY_EXISTS
        kwargs['message'] = getattr(errortup[1], 'message',
                str(errortup[1]))
        value = Error.make_instance(env, **kwargs)
        return value
