#!/usr/bin/env python
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
# Authors: Jan Grec <jgrec@redhat.com>
#
"""
Unit tests for ``LMI_SoftwareInstallationJob`` provider.
"""

import datetime
import re
from multiprocessing import Process, Queue
import time

from lmi.shell import LMIInstanceName
from lmi.shell import LMIUtil
from lmi.test import wbem
from lmi.test import unittest
from lmi.test.lmibase import enable_lmi_exceptions

import package
import swbase
import util

INSTALL_OPTIONS_FORCE     = 3
INSTALL_OPTIONS_INSTALL   = 4
INSTALL_OPTIONS_UPDATE    = 5
INSTALL_OPTIONS_REPAIR    = 6
INSTALL_OPTIONS_UNINSTALL = 9

JOB_STATE_NEW           =  2
JOB_STATE_STARTING      =  3
JOB_STATE_RUNNING       =  4
JOB_STATE_SUSPENDED     =  5
JOB_STATE_SHUTTING_DOWN =  6
JOB_STATE_COMPLETED     =  7
JOB_STATE_TERMINATED    =  8
JOB_STATE_KILLED        =  9
JOB_STATE_EXCEPTION     = 10
JOB_STATE_SERVICE       = 11
JOB_STATE_QUERY_PENDING = 12

LOCAL_TIME = 1
UTC_TIME   = 2

CIM_STATUS_CODE_OK                                      =  0
CIM_STATUS_CODE_ERR_FAILED                              =  1
CIM_STATUS_CODE_ERR_ACCESS_DENIED                       =  2
CIM_STATUS_CODE_ERR_INVALID_NAMESPACE                   =  3
CIM_STATUS_CODE_ERR_INVALID_PARAMETER                   =  4
CIM_STATUS_CODE_ERR_INVALID_CLASS                       =  5
CIM_STATUS_CODE_ERR_NOT_FOUND                           =  6
CIM_STATUS_CODE_ERR_NOT_SUPPORTED                       =  7
CIM_STATUS_CODE_ERR_CLASS_HAS_CHILDREN                  =  8
CIM_STATUS_CODE_ERR_CLASS_HAS_INSTANCES                 =  9
CIM_STATUS_CODE_ERR_INVALID_SUPERCLASS                  = 10
CIM_STATUS_CODE_ERR_ALREADY_EXISTS                      = 11
CIM_STATUS_CODE_ERR_NO_SUCH_PROPERTY                    = 12
CIM_STATUS_CODE_ERR_TYPE_MISMATCH                       = 13
CIM_STATUS_CODE_ERR_QUERY_LANGUAGE_NOT_SUPPORTED        = 14
CIM_STATUS_CODE_ERR_INVALID_QUERY                       = 15
CIM_STATUS_CODE_ERR_METHOD_NOT_AVAILABLE                = 16
CIM_STATUS_CODE_ERR_METHOD_NOT_FOUND                    = 17
CIM_STATUS_CODE_ERR_UNEXPECTED_RESPONSE                 = 18
CIM_STATUS_CODE_ERR_INVALID_RESPONSE_DESTINATION        = 19
CIM_STATUS_CODE_ERR_NAMESPACE_NOT_EMPTY                 = 20
CIM_STATUS_CODE_ERR_INVALID_ENUMERATION_CONTEXT         = 21
CIM_STATUS_CODE_ERR_INVALID_OPERATION_TIMEOUT           = 22
CIM_STATUS_CODE_ERR_PULL_HAS_BEEN_ABANDONED             = 23
CIM_STATUS_CODE_ERR_PULL_CANNOT_BE_ABANDONED            = 24
CIM_STATUS_CODE_ERR_FILTERED_ENUMERATION_NOT_SUPPORTED  = 25
CIM_STATUS_CODE_ERR_CONTINUATION_ON_ERROR_NOT_SUPPORTED = 26
CIM_STATUS_CODE_ERR_SERVER_LIMITS_EXCEEDED              = 27
CIM_STATUS_CODE_ERR_SERVER_IS_SHUTTING_DOWN             = 28
CIM_STATUS_CODE_ERR_QUERY_FEATURE_NOT_SUPPORTED         = 29

COMMUNICATION_STATUS_UNKNOWN            = 0
COMMUNICATION_STATUS_NOT_AVAILABLE      = 1
COMMUNICATION_STATUS_COMMUNICATION_OK   = 2
COMMUNICATION_STATUS_LOST_COMMUNICATION = 3
COMMUNICATION_STATUS_NO_CONTACT         = 4

ERROR_CODE_UNKNOWN                              =  0
ERROR_CODE_OOM                                  =  1
ERROR_CODE_NO_NETWORK                           =  2
ERROR_CODE_NOT_SUPPORTED                        =  3
ERROR_CODE_INTERNAL_ERROR                       =  4
ERROR_CODE_GPG_FAILURE                          =  5
ERROR_CODE_PACKAGE_ID_INVALID                   =  6
ERROR_CODE_PACKAGE_NOT_INSTALLED                =  7
ERROR_CODE_PACKAGE_NOT_FOUND                    =  8
ERROR_CODE_PACKAGE_ALREADY_INSTALLED            =  9
ERROR_CODE_PACKAGE_DOWNLOAD_FAILED              = 10
ERROR_CODE_GROUP_NOT_FOUND                      = 11
ERROR_CODE_GROUP_LIST_INVALID                   = 12
ERROR_CODE_DEP_RESOLUTION_FAILED                = 13
ERROR_CODE_FILTER_INVALID                       = 14
ERROR_CODE_CREATE_THREAD_FAILED                 = 15
ERROR_CODE_TRANSACTION_ERROR                    = 16
ERROR_CODE_TRANSACTION_CANCELLED                = 17
ERROR_CODE_NO_CACHE                             = 18
ERROR_CODE_REPO_NOT_FOUND                       = 19
ERROR_CODE_CANNOT_REMOVE_SYSTEM_PACKAGE         = 20
ERROR_CODE_PROCESS_KILL                         = 21
ERROR_CODE_FAILED_INITIALIZATION                = 22
ERROR_CODE_FAILED_FINALISE                      = 23
ERROR_CODE_FAILED_CONFIG_PARSING                = 24
ERROR_CODE_CANNOT_CANCEL                        = 25
ERROR_CODE_CANNOT_GET_LOCK                      = 26
ERROR_CODE_NO_PACKAGES_TO_UPDATE                = 27
ERROR_CODE_CANNOT_WRITE_REPO_CONFIG             = 28
ERROR_CODE_LOCAL_INSTALL_FAILED                 = 29
ERROR_CODE_BAD_GPG_SIGNATURE                    = 30
ERROR_CODE_MISSING_GPG_SIGNATURE                = 31
ERROR_CODE_CANNOT_INSTALL_SOURCE_PACKAGE        = 32
ERROR_CODE_REPO_CONFIGURATION_ERROR             = 33
ERROR_CODE_NO_LICENSE_AGREEMENT                 = 34
ERROR_CODE_FILE_CONFLICTS                       = 35
ERROR_CODE_PACKAGE_CONFLICTS                    = 36
ERROR_CODE_REPO_NOT_AVAILABLE                   = 37
ERROR_CODE_INVALID_PACKAGE_FILE                 = 38
ERROR_CODE_PACKAGE_INSTALL_BLOCKED              = 39
ERROR_CODE_PACKAGE_CORRUPT                      = 40
ERROR_CODE_ALL_PACKAGES_ALREADY_INSTALLED       = 41
ERROR_CODE_FILE_NOT_FOUND                       = 42
ERROR_CODE_NO_MORE_MIRRORS_TO_TRY               = 43
ERROR_CODE_NO_DISTRO_UPGRADE_DATA               = 44
ERROR_CODE_INCOMPATIBLE_ARCHITECTURE            = 45
ERROR_CODE_NO_SPACE_ON_DEVICE                   = 46
ERROR_CODE_MEDIA_CHANGE_REQUIRED                = 47
ERROR_CODE_NOT_AUTHORIZED                       = 48
ERROR_CODE_UPDATE_NOT_FOUND                     = 49
ERROR_CODE_CANNOT_INSTALL_REPO_UNSIGNED         = 50
ERROR_CODE_CANNOT_UPDATE_REPO_UNSIGNED          = 51
ERROR_CODE_CANNOT_GET_FILELIST                  = 52
ERROR_CODE_CANNOT_GET_REQUIRES                  = 53
ERROR_CODE_CANNOT_DISABLE_REPOSITORY            = 54
ERROR_CODE_RESTRICTED_DOWNLOAD                  = 55
ERROR_CODE_PACKAGE_FAILED_TO_CONFIGURE          = 56
ERROR_CODE_PACKAGE_FAILED_TO_BUILD              = 57
ERROR_CODE_PACKAGE_FAILED_TO_INSTALL            = 58
ERROR_CODE_PACKAGE_FAILED_TO_REMOVE             = 59
ERROR_CODE_UPDATE_FAILED_DUE_TO_RUNNING_PROCESS = 60
ERROR_CODE_PACKAGE_DATABASE_CHANGED             = 61
ERROR_CODE_PROVIDE_TYPE_NOT_SUPPORTED           = 62
ERROR_CODE_INSTALL_ROOT_INVALID                 = 63
ERROR_CODE_CANNOT_FETCH_SOURCES                 = 64
ERROR_CODE_CANCELLED_PRIORITY                   = 65
ERROR_CODE_UNFINISHED_TRANSACTION               = 66
ERROR_CODE_LOCK_REQUIRED                        = 67

ERROR_SOURCE_FORMAT_UNKNOWN       = 0
ERROR_SOURCE_FORMAT_OTHER         = 1
ERROR_SOURCE_FORMAT_CIMOBJECTPATH = 2

ERROR_TYPE_UNKNOWN                     =  0
ERROR_TYPE_OTHER                       =  1
ERROR_TYPE_COMMUNICATIONS_ERROR        =  2
ERROR_TYPE_QUALITY_OF_SERVICE_ERROR    =  3
ERROR_TYPE_SOFTWARE_ERROR              =  4
ERROR_TYPE_HARDWARE_ERROR              =  5
ERROR_TYPE_ENVIROMENTAL_ERROR          =  6
ERROR_TYPE_SECURITY_ERROR              =  7
ERROR_TYPE_OVERSUBSCRIPTION_ERROR      =  8
ERROR_TYPE_UNAVAILABLE_RESOURCE_ERROR  =  9
ERROR_TYPE_UNSUPPORTED_OPERATION_ERROR = 10

GET_ERRORS_SUCCESS = 0

OPERATING_STATUS_UNKNOWN       =  0
OPERATING_STATUS_NOT_AVAILABLE =  1
OPERATING_STATUS_SERVICING     =  2
OPERATING_STATUS_STARTING      =  3
OPERATING_STATUS_STOPPING      =  4
OPERATING_STATUS_STOPPED       =  5
OPERATING_STATUS_ABORTED       =  6
OPERATING_STATUS_DORMANT       =  7
OPERATING_STATUS_COMPLETED     =  8
OPERATING_STATUS_MIGRATING     =  9
OPERATING_STATUS_EMIGRATING    = 10
OPERATING_STATUS_IMMIGRATING   = 11
OPERATING_STATUS_SNAPSHOTTING  = 12
OPERATING_STATUS_SHUTTING_DOWN = 13
OPERATING_STATUS_IN_TEST       = 14
OPERATING_STATUS_TRANSITIONING = 15
OPERATING_STATUS_IN_SERVICE    = 16

OPERATIONAL_STATUS_UNKNOWN                    =  0
OPERATIONAL_STATUS_OTHER                      =  1
OPERATIONAL_STATUS_OK                         =  2
OPERATIONAL_STATUS_DEGRADED                   =  3
OPERATIONAL_STATUS_STRESSED                   =  4
OPERATIONAL_STATUS_PREDICTIVE_FAILURE         =  5
OPERATIONAL_STATUS_ERROR                      =  6
OPERATIONAL_STATUS_NON_RECOVERABLE_ERROR      =  7
OPERATIONAL_STATUS_STARTING                   =  8
OPERATIONAL_STATUS_STOPPING                   =  9
OPERATIONAL_STATUS_STOPPED                    = 10
OPERATIONAL_STATUS_IN_SERVICE                 = 11
OPERATIONAL_STATUS_NO_CONTACT                 = 12
OPERATIONAL_STATUS_LOST_COMMUNICATION         = 13
OPERATIONAL_STATUS_ABORTED                    = 14
OPERATIONAL_STATUS_DORMANT                    = 15
OPERATIONAL_STATUS_SUPPORTING_ENTITY_IN_ERROR = 16
OPERATIONAL_STATUS_COMPLETED                  = 17
OPERATIONAL_STATUS_POWER_MODE                 = 18
OPERATIONAL_STATUS_RELOCATING                 = 19

PRIMARY_STATUS_UNKNOWN  = 0
PRIMARY_STATUS_OK       = 1
PRIMARY_STATUS_DEGRADED = 2
PRIMARY_STATUS_ERROR    = 3

REQUEST_STATE_CHANGE_START     = 2
REQUEST_STATE_CHANGE_SUSPEND   = 3
REQUEST_STATE_CHANGE_TERMINATE = 4
REQUEST_STATE_CHANGE_KILL      = 5
REQUEST_STATE_CHANGE_SERVICE   = 6

# return values
REQUEST_STATE_CHANGE_COMPLETED_WITH_NO_ERROR = 0

RETURN_VALUE_TYPE_UINT32 = 9

JOB_COMPLETED_WITH_NO_ERROR = 0
UNSPECIFIED_ERROR = 2
INVALID_PARAMETER = 5
METHOD_PARAMETERS_CHECKED_JOB_STARTED = 4096

JOB_STATE_2_STATUS = {
        JOB_STATE_NEW        : 'Enqueued',
        JOB_STATE_RUNNING    : 'Running',
        JOB_STATE_TERMINATED : 'Terminated',
        JOB_STATE_EXCEPTION  : 'Failed',
        JOB_STATE_COMPLETED  : 'Completed successfully'
}

JOB_STATE_2_OPERATIONAL_STATUS = {
        JOB_STATE_NEW        : [OPERATIONAL_STATUS_DORMANT],
        JOB_STATE_RUNNING    : [OPERATIONAL_STATUS_OK],
        JOB_STATE_TERMINATED : [OPERATIONAL_STATUS_STOPPED],
        JOB_STATE_EXCEPTION  : [OPERATIONAL_STATUS_ERROR],
        JOB_STATE_COMPLETED  : [OPERATIONAL_STATUS_OK,
            OPERATIONAL_STATUS_COMPLETED]
}

class TestSoftwareInstallationJob(swbase.SwTestCase):
    """
    Basic cim operations test on ``LMI_SoftwareInstallatioJob``.
    """

    CLASS_NAME = "LMI_SoftwareInstallationJob"
    KEYS = ("InstanceID", )
    RE_INSTANCE_ID = re.compile(r'^LMI:%s:(?P<number>\d+)$' % CLASS_NAME, re.I)

    def make_op(self, jobnum):
        """
        :returns: Object path of ``LMI_SoftwareInstallationJob``
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        return self.cim_class.new_instance_name({
            "InstanceID" : 'LMI:%s:%d' % (self.CLASS_NAME, int(jobnum))
        })

    @property
    def service_op(self):
        """
        :returns: Object path of ``LMI_SoftwareInstallationJob``
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        if not hasattr(self, '_service_op'):
            self._service_op = self.ns.LMI_SoftwareInstallationService \
                    .first_instance()
        return self._service_op

    @enable_lmi_exceptions
    def _check_cim_error(self, job_inst, error_inst,
            expected_status_code=None,
            expected_error_type=None,
            pkgkit_running=True):
        """
        Run some asserts on instance of ``CIM_Error``.

        :param job_inst: An instance of ``LMI_SoftwareInstallationJob``
        :param error_inst: An instance of corresponding ``CIM_Error`` obtained
            either from ``GetError()`` method of job or from its post call
            indication.
        :param int expected_status_code: Expected value of ``CIMStatusCode``
            property.
        :param int expected_error_type: Expected value of ``ErrorType`` property.
        :param bool pkgkit_running: Whether the PackageKit is running during
            currently executing test.
        """
        if isinstance(error_inst, wbem.CIMInstance):
            error_inst = LMIUtil.lmi_wrap_cim_instance(self.conn, error_inst,
                    error_inst.classname, self.ns.name)
        self.assertEqual(error_inst.classname, "CIM_Error")
        self.assertTrue(isinstance(error_inst.CIMStatusCode, (int, long)))
        self.assertNotEqual(error_inst.CIMStatusCode, CIM_STATUS_CODE_OK)
        if expected_status_code is not None:
            self.assertEqual(error_inst.CIMStatusCode, expected_status_code)
        self.assertObjectPathStringEqual(error_inst.ErrorSource,
                str(job_inst.path.wrapped_object))
        self.assertEqual(error_inst.ErrorSourceFormat,
                ERROR_SOURCE_FORMAT_CIMOBJECTPATH)
        self.assertTrue(isinstance(error_inst.ErrorType, (int, long)))
        if expected_error_type is not None:
            self.assertEqual(error_inst.ErrorType, expected['error_type'])
        elif not pkgkit_running:
            self.assertEqual(error_inst.ErrorType, ERROR_TYPE_COMMUNICATIONS_ERROR)
        self.assertTrue(len(error_inst.Message) > 0)

    @enable_lmi_exceptions
    def _check_inst_method_call(self, job_inst, inst,
            input_parameters,
            pre=True,
            expected_result=None,
            expected_status_code=None,
            expected_error_type=None,
            pkgkit_running=True):
        """
        Run some asserts on instance of ``CIM_InstMethodCall``.

        :param job_inst: An instance of ``LMI_SoftwareInstallationJob``
        :param inst: An instance of corresponding ``CIM_InstMethodCall`` obtained
            from associated job method result.
        :param dict input_parameters: Dictionary of input parameters passed to
            asynchronous method that created the job.
        :param expected_result: Expected value of ``ReturnValue`` property.
            This can be of any type which is serializable to string.
        :param int expected_status_code: Expected value of ``CIMStatusCode``
            property.
        :param int expected_error_type: Expected value of ``ErrorType`` property.
        :param bool pkgkit_running: Whether the PackageKit is running during
            currently executing test.
        """
        if isinstance(inst, wbem.CIMInstance):
            inst = LMIUtil.lmi_wrap_cim_instance(self.conn, inst,
                    inst.classname, self.ns.name)
        self.assertEqual(inst.classname, 'CIM_InstMethodCall')
        self.assertEqual(inst.SourceInstance.classname, self.CLASS_NAME)
        src_inst = LMIUtil.lmi_wrap_cim_instance(self.conn, inst.SourceInstance,
                self.CLASS_NAME, self.ns.name)
        self.assertEqual(src_inst.InstanceID, job_inst.InstanceID)
        if job_inst.JobState != JOB_STATE_NEW:
            self.assertEqual(src_inst.StartTime, job_inst.StartTime)
        if not job_inst.JobState in (JOB_STATE_NEW, JOB_STATE_RUNNING):
            self.assertEqual(src_inst.Description, job_inst.Description)
            self.assertEqual(src_inst.ElementName, job_inst.ElementName)
            self.assertEqual(src_inst.ElapsedTime, job_inst.ElapsedTime)
            self.assertEqual(src_inst.TimeOfLastStateChange,
                    job_inst.TimeOfLastStateChange)
        self.assertEqual(src_inst.Caption, job_inst.Caption)
        self.assertEqual(src_inst.TimeSubmitted, job_inst.TimeSubmitted)
        self.assertObjectPathStringEqual(inst.SourceInstanceModelPath,
                str(job_inst.wrapped_object.path))
        self.assertEqual(inst.MethodName, job_inst.MethodName)
        self.assertEqual(inst.ReturnValueType, RETURN_VALUE_TYPE_UINT32)
        if not inst.PreCall and job_inst.JobState == JOB_STATE_COMPLETED:
            self.assertTrue(isinstance(inst.ReturnValue, basestring))
            if expected_result is not None:
                self.assertEqual(inst.ReturnValue, str(expected_result))
            else:
                self.assertTrue(re.match(r'^\d+$', inst.ReturnValue) is not None)
        else:
            self.assertEqual(inst.ReturnValue, None)
        self.assertEqual(inst.PreCall, pre)
        self.assertTrue(isinstance(inst.MethodParameters,
            wbem.CIMInstance))
        params_inst = LMIUtil.lmi_wrap_cim_instance(self.conn,
                inst.MethodParameters, inst.MethodParameters.classname,
                self.ns.name)
        props = set(["Source", "Target", "Collection", "InstallOptions",
                     "InstallOptionValues"])
        if pre or job_inst.JobState != JOB_STATE_COMPLETED:
            self.assertEqual(params_inst.classname,
                    '__MethodParameters_' + job_inst.MethodName)
        else:
            self.assertEqual(params_inst.classname,
                    '__MethodParameters_' + job_inst.MethodName + '_Result')
            props.add("__ReturnValue")
            self.assertNotEqual(getattr(params_inst, "__ReturnValue"), None)
            if expected_result is not None:
                self.assertEqual(getattr(params_inst, "__ReturnValue"),
                        expected_result)
        self.assertEqual(set(params_inst.properties()), set(props))
        for param, value in input_parameters.items():
            inst_val = getattr(params_inst, param)
            if isinstance(value, (list, tuple)):
                self.assertTrue(isinstance(inst_val, list))
                self.assertEqual(set(inst_val), set(value))
            elif isinstance(value, LMIInstanceName):
                self.assertCIMNameEqual(inst_val, value)
            else:
                self.assertEqual(inst_val, value)
        for param in params_inst.properties():
            if not param in input_parameters and param != "__ReturnValue":
                self.assertTrue(getattr(params_inst, param) in ([], None))
        if not pre:
            if job_inst.JobState == JOB_STATE_COMPLETED:
                self.assertEqual(inst.Error, [])
            elif job_inst.JobState == JOB_STATE_EXCEPTION:
                self.assertEqual(inst.ReturnValue, None)
                self.assertEqual(len(inst.Error), 1)
                self._check_cim_error(job_inst, inst.Error[0],
                        expected_status_code=expected_status_code,
                        expected_error_type=expected_error_type,
                        pkgkit_running=pkgkit_running)
            else:
                self.assertEqual(job_inst.JobState, JOB_STATE_TERMINATED)
                self.assertEqual(inst.ReturnValue, None)
                self.assertEqual(inst.Error, [])
        else:
            self.assertTrue(inst.Error in (None, []))
            self.assertEqual(inst.ReturnValue, None)

    @enable_lmi_exceptions
    def _check_job_method_result(self, job_inst, result_inst,
            input_parameters,
            expected_result=None,
            expected_status_code=None,
            expected_error_type=None,
            pkgkit_running=True):
        """
        Run some asserts on instance of ``LMI_SoftwareMethodResult``.

        :param job_inst: An instance of ``LMI_SoftwareInstallationJob``.
        :param result_inst: An instance of associated
            ``LMI_SoftwareMethodResult`` result.
        :param dict input_parameters: Dictionary of input parameters passed to
            asynchronous method that created the job.
        :param expected_result: Expected value of ``ReturnValue`` property.
            This can be of any type which is serializable to string.
        :param int expected_status_code: Expected value of ``CIMStatusCode``
            property.
        :param int expected_error_type: Expected value of ``ErrorType`` property.
        :param bool pkgkit_running: Whether the PackageKit is running during
            currently executing test.
        """
        self.assertEqual(result_inst.classname, 'LMI_SoftwareMethodResult')
        jobnum = int(self.RE_INSTANCE_ID.match(
            job_inst.InstanceID).group('number'))
        self.assertEqual(result_inst.InstanceID,
                "LMI:LMI_SoftwareMethodResult:%d" % jobnum)
        self.assertEqual(result_inst.path.classname, result_inst.classname)
        self.assertEqual(
                dict((k, v) for k, v in
                        result_inst.path.key_properties_dict().items()),
                {'InstanceID' : result_inst.InstanceID})
        self.assertTrue(len(result_inst.Caption) > 0)
        self.assertTrue(len(result_inst.Description) > 0)
        self.assertEqual(result_inst.ElementName, "MethodResult-%d" % int(jobnum))
        self._check_inst_method_call(job_inst,
                result_inst.PreCallIndication,
                input_parameters,
                True,
                expected_result=expected_result,
                expected_status_code=expected_status_code,
                expected_error_type=expected_error_type,
                pkgkit_running=pkgkit_running)
        if not job_inst.JobState in (JOB_STATE_NEW, JOB_STATE_RUNNING):
            self._check_inst_method_call(job_inst,
                    result_inst.PostCallIndication,
                    input_parameters,
                    False,
                    expected_result=expected_result,
                    expected_status_code=expected_status_code,
                    expected_error_type=expected_error_type,
                    pkgkit_running=pkgkit_running)
        elif result_inst.PostCallIndication is not None:
            # PostCallIndication may only be filled when job is finished.
            # We need to check, whether the job has completed since last GetInstance().
            job_inst.refresh()
            self.assertFalse(job_inst.JobState in (JOB_STATE_NEW, JOB_STATE_RUNNING))
            self._check_inst_method_call(job_inst,
                    result_inst.PostCallIndication,
                    input_parameters,
                    False,
                    expected_result=expected_result,
                    expected_status_code=expected_status_code,
                    expected_error_type=expected_error_type,
                    pkgkit_running=pkgkit_running)

    @enable_lmi_exceptions
    def _check_job_parameters(self, job_inst, input_parameters):
        """
        Run some asserts on ``LMI_SoftwareJob.JobInParameters`` and
        ``LMI_SoftwareJob.JobOutParameters`` properties of given job.

        :param job_inst: An instance of ``LMI_SoftwareInstallationJob``
        :param dict input_parameters: Dictionary of input parameters passed to
            asynchronous method that created the job.
        """
        if job_inst.MethodName is None:
            self.assertEqual(job_inst.JobInParameters, None)
            self.assertEqual(job_inst.JobOutParameters, None)
        elif job_inst.JobState != JOB_STATE_COMPLETED:
            self.assertNotEqual(job_inst.JobInParameters, None)
            self.assertEqual(job_inst.JobOutParameters, None)
        else:
            self.assertNotEqual(job_inst.JobInParameters, None)
            self.assertNotEqual(job_inst.JobOutParameters, None)

        if job_inst.JobInParameters is not None:
            inst = LMIUtil.lmi_wrap_cim_instance(self.conn,
                    job_inst.JobInParameters,
                    job_inst.JobInparameters.classname, self.ns.name)
            self.assertEqual(inst.classname,
                    '__MethodParameters_' + job_inst.MethodName)
            if job_inst.MethodName == "InstallFromSoftwareIdentity":
                self.assertEqual(set(inst.properties()),
                        set(["Source", "Target", "Collection", "InstallOptions",
                            "InstallOptionValues"]))
            for param, value in input_parameters.items():
                inst_val = getattr(inst, param)
                if isinstance(value, (list, tuple)):
                    self.assertTrue(isinstance(inst_val, list))
                    self.assertEqual(set(inst_val), set(value))
                elif isinstance(value, LMIInstanceName):
                    self.assertCIMNameEqual(inst_val, value)
                else:
                    self.assertEqual(inst_val, value)
            for param in inst.properties():
                if not param in input_parameters:
                    self.assertIn(getattr(inst, param), (None, []),
                            "Input parameter %s should be unset for job %s." %
                            (param, job_inst.InstanceId))

        if job_inst.JobOutParameters is not None:
            inst = LMIUtil.lmi_wrap_cim_instance(self.conn,
                    job_inst.JobOutParameters, 
                    job_inst.JobOutParameters.classname, self.ns.name)
            self.assertEqual(inst.classname,
                    '__MethodParameters_' + job_inst.MethodName + "_Result")
            self.assertEqual(set(inst.properties()),
                    (set(job_inst.JobInParameters.properties.keys()).union(
                        set(["__ReturnValue"]))))
            for param, value in inst.properties_dict().items():
                if param == "__ReturnValue":
                    self.assertTrue(isinstance(value, (int, long)))
                else:
                    self.assertTrue(value in ([], None))

    @enable_lmi_exceptions
    def _check_job_instance(self,
            op, inst,
            input_parameters,
            expected_states=None,
            expected_result=None,
            expected_status_code=None,
            expected_error_type=None,
            pkgkit_running=True,
            affected_packages=None,
            affected_allow_subset=False,
            **kwargs):
        """
        Run some asserts on instance of ``LMI_SoftwareInstallationJob``.

        :param inst: An instance of ``LMI_SoftwareInstallationJob``.
        :param dict input_parameters: Dictionary of input parameters passed to
            asynchronous method that created the job.
        :param set expected_states: Set of expected states of job. ``JobState``
            property of given job must be one of them.
        :param expected_result: Expected value of ``ReturnValue`` property.
            This can be of any type which is serializable to string.
        :param int expected_status_code: Expected value of ``CIMStatusCode``
            property.
        :param int expected_error_type: Expected value of ``ErrorType`` property.
        :param bool pkgkit_running: Whether the PackageKit is running during
            currently executing test.
        :param affected_packages: 
        """
        if isinstance(expected_states, (int, long)):
            expected_states = (expected_states, )
        self.assertNotEqual(inst, None)
        self.assertCIMNameEqual(inst.path, op)
        for key_property in op.key_properties():
                self.assertEqual(getattr(inst, key_property),
                        getattr(op, key_property))
        for key, value in kwargs.items():
            self.assertTrue(key in inst.properties())
            self.assertEqual(getattr(inst, key), value)

        self.assertTrue(len(inst.Caption) > 0)
        self.assertTrue(len(inst.Description) > 0)
        if self.backend == 'yum':
            self.assertEqual(inst.CommunicationStatus,
                    COMMUNICATION_STATUS_NOT_AVAILABLE)
        elif pkgkit_running:
            self.assertEqual(inst.CommunicationStatus,
                    COMMUNICATION_STATUS_COMMUNICATION_OK)
        else:
            self.assertEqual(inst.CommunicationStatus,
                    COMMUNICATION_STATUS_LOST_COMMUNICATION)
        self.assertTrue(isinstance(inst.ElapsedTime, wbem.CIMDateTime))
        self.assertTrue(inst.ElapsedTime.is_interval)
        if inst.JobState == JOB_STATE_COMPLETED:
            self.assertEqual(inst.ErrorCode, 0)
        else:
            if inst.JobState == JOB_STATE_EXCEPTION:
                self.assertNotEqual(inst.ErrorCode, 0)
            else:
                self.assertEqual(inst.ErrorCode, 0)
        if expected_states is not None:
            self.assertTrue(inst.JobState in expected_states,
                    "JobState %d is not one of expected %s for job \"%s\"" %
                    (inst.JobState, str(expected_states), inst.InstanceID))
        self.assertTrue(isinstance(inst.JobStatus, basestring))
        self.assertEqual(JOB_STATE_2_STATUS[inst.JobState], inst.JobStatus)
        self.assertEqual(inst.LocalOrUtcTime, UTC_TIME)
        self.assertTrue(isinstance(inst.OperationalStatus, list))
        self.assertEqual(set(inst.OperationalStatus),
                set(JOB_STATE_2_OPERATIONAL_STATUS[inst.JobState]))
        self.assertTrue(isinstance(inst.TimeSubmitted, wbem.CIMDateTime))
        self.assertFalse(inst.TimeSubmitted.is_interval)
        self.assertTrue(isinstance(inst.TimeOfLastStateChange, wbem.CIMDateTime))
        self.assertFalse(inst.TimeOfLastStateChange.is_interval)
        if inst.JobState == JOB_STATE_NEW:
            self.assertEqual(inst.TimeSubmitted, inst.TimeOfLastStateChange)
        else:
            self.assertTrue(inst.TimeSubmitted <= inst.TimeOfLastStateChange)
        if inst.JobState == JOB_STATE_NEW:
            self.assertEqual(inst.StartTime, None)
            self.assertEqual(util.timedelta_get_total_seconds(
                inst.ElapsedTime.timedelta), 0)
        else:
            self.assertTrue(isinstance(inst.StartTime, wbem.CIMDateTime))
            self.assertFalse(inst.StartTime.is_interval)
            self.assertTrue(inst.TimeSubmitted <= inst.StartTime)
            if inst.JobState != JOB_STATE_RUNNING:
                # finished job
                self.assertAlmostEqual(
                        util.timedelta_get_total_seconds(
                              inst.TimeOfLastStateChange.datetime
                            - inst.StartTime.datetime),
                        util.timedelta_get_total_seconds(
                            inst.ElapsedTime.timedelta), delta=0.5)
            else:   # JobState == RUNNING
                self.assertAlmostEqual(
                        util.timedelta_get_total_seconds(
                              wbem.CIMDateTime.now().datetime
                            - inst.StartTime.datetime),
                        util.timedelta_get_total_seconds(
                            inst.ElapsedTime.timedelta), delta=5)
        rval, outparms, _ = inst.GetErrors(RefreshInstance=False)
        self.assertEqual(rval, GET_ERRORS_SUCCESS)
        self.assertEqual(len(outparms), 1)
        if inst.JobState == JOB_STATE_EXCEPTION:
            self.assertEqual(len(outparms['Errors']), 1,
                    "Expected non-empty list as an output parameter Errors"
                    " of GetErrors() on job %s (state=%d)."
                    % (inst.InstanceID, inst.JobState))
            self._check_cim_error(inst, outparms['Errors'][0],
                    expected_status_code=expected_status_code,
                    expected_error_type = (  ERROR_TYPE_COMMUNICATIONS_ERROR
                                      if not pkgkit_running else None)
                        if expected_error_type is None else expected_error_type,
                    pkgkit_running=pkgkit_running)
        else:
            if len(outparms["Errors"]) == 1:
                # job has been modified since last refresh
                inst.refresh()
                self.assertEqual(inst.JobState, JOB_STATE_EXCEPTION)
                self._check_cim_error(inst, outparms['Errors'][0],
                        expected_status_code=expected_status_code,
                        expected_error_type = (  ERROR_TYPE_COMMUNICATIONS_ERROR
                                          if not pkgkit_running else None)
                            if expected_error_type is None else expected_error_type,
                        pkgkit_running=pkgkit_running)
            else:
                self.assertEqual(outparms['Errors'], [],
                        "Expected empty list as an output parameter Errors"
                        " of GetErrors() on job %s (state=%d)."
                        % (inst.InstanceID, inst.JobState))

        # implemented properties must be filled
        self.assertTrue(isinstance(inst.DeleteOnCompletion, bool))
        if inst.JobState != JOB_STATE_NEW:
            self.assertTrue(isinstance(inst.MethodName, basestring))
        else:
            self.assertTrue(
                       inst.MethodName is None
                    or isinstance(inst.MethodName, basestring))
        self._check_job_parameters(inst, input_parameters)
        self.assertTrue(isinstance(inst.PercentComplete, (int, long)))
        self.assertTrue(isinstance(inst.Priority, (int, long)))

        results = inst.associators(
            AssocClass='LMI_AssociatedSoftwareJobMethodResult',
            ResultClass='LMI_SoftwareMethodResult',
            Role='Job',
            ResultRole='JobParameters')
        if inst.MethodName is not None:
            # MethodResult instance is associated with a job right after its
            # MethodName property is set
            self.assertEqual(len(results), 1)
        else:
            self.assertTrue(len(results) <= 1)
        for result in results:
            self._check_job_method_result(inst, result,
                    input_parameters,
                    expected_result=expected_result,
                    expected_status_code=expected_status_code,
                    expected_error_type=expected_error_type)

        results = inst.associator_names(AssocClass='LMI_AffectedSoftwareJobElement')
        self.assertCIMNameIn(self.system_iname, results)
        for i, result in enumerate(results):
            if result.classname == self.system_cs_name:
                del results[i]
                break
        self.assertCIMNameIn(self.ns.LMI_SystemSoftwareCollection.new_instance_name({
            "InstanceID" : "LMI:LMI_SystemSoftwareCollection"
            }), results)
        for i, result in enumerate(results):
            if result.classname == "LMI_SystemSoftwareCollection":
                del results[i]
                break
        pkg_ids = set()
        for result in results:
            self.assertCIMIsSubclass(result.classname, "LMI_SoftwareIdentity")
            self.assertFalse(result.InstanceID in pkg_ids)
            pkg_ids.add(result.InstanceID)
            if affected_packages is not None:
                self.assertCIMNameIn(result, affected_packages)
        if inst.JobState == JOB_STATE_COMPLETED:
            self.assertTrue(len(pkg_ids) > 0)
            if affected_packages and not affected_allow_subset:
                for pkg in affected_packages:
                    self.assertCIMNameIn(pkg, results)

    @enable_lmi_exceptions
    def _test_get_instance(self, jobnum, input_parameters, **kwargs):
        objpath = self.make_op(jobnum)
        inst = objpath.to_instance()
        self._check_job_instance(objpath, inst, input_parameters, **kwargs)
        return inst

    @enable_lmi_exceptions
    def _test_check_method_out_params(self, input_parameters, oparms, **kwargs):
        self.assertEqual(len(oparms), 1)
        self.assertTrue("Job" in oparms)
        self.assertEqual(set(oparms['Job'].key_properties()), set(("InstanceID", )))
        match = self.RE_INSTANCE_ID.match(oparms["Job"].InstanceID)
        self.assertNotEqual(match, None,
                "Failed to parse instance id \"%s\" of job."
                % oparms["Job"].InstanceID)
        return self._test_get_instance(match.group('number'),
                input_parameters, **kwargs)

    @enable_lmi_exceptions
    def _do_test_single_pkg_install(self, pkg,
            queue=None,
            affected_packages=None,
            affected_allow_subset=False):
        self.assertFalse(package.is_pkg_installed(pkg))
        if queue is not None:
            # we are running in separate process, we need our own connection
            if hasattr(self, '_shellconnection'):
                delattr(self, '_shellconnection')
            if hasattr(self, '_service_op'):
                delattr(self, '_service_op')

        input_parameters = {
                "Source"         : util.make_pkg_op(self.ns, pkg),
                "InstallOptions" : [INSTALL_OPTIONS_INSTALL],
                "Target"         : self.system_iname
        }
        rval, oparms, _ = self.service_op.InstallFromSoftwareIdentity(
                **input_parameters)
        self.assertEqual(rval, METHOD_PARAMETERS_CHECKED_JOB_STARTED)
        inst = self._test_check_method_out_params(
                input_parameters, oparms,
                expected_states=(JOB_STATE_NEW, JOB_STATE_RUNNING),
                affected_packages=affected_packages,
                affected_allow_subset=affected_allow_subset)
        if queue is not None:
            queue.put((pkg.name, inst.InstanceID))

        while inst.JobState in (JOB_STATE_NEW, JOB_STATE_RUNNING):
            inst.refresh()
            self._check_job_instance(inst.path, inst, input_parameters)
            time.sleep(0.1)

        self._check_job_instance(inst.path, inst,
                input_parameters,
                expected_states=set((JOB_STATE_COMPLETED, )),
                expected_result=JOB_COMPLETED_WITH_NO_ERROR,
                expected_status_code=CIM_STATUS_CODE_OK,
                affected_packages=affected_packages,
                affected_allow_subset=affected_allow_subset)

        if queue is not None:
            queue.put((pkg.name, True))

    @swbase.test_with_repos('stable', **{
            'updates'         : False,
            'updates-testing' : False,
            'misc'            : False
    })
    @swbase.test_with_packages(**{ 'pkg1' : False })
    def test_install_package(self):
        """
        Try to install single package.
        """
        pkg = self.get_repo('stable')['pkg1']
        self._do_test_single_pkg_install(pkg,
                affected_packages=[util.make_pkg_op(self.ns, pkg)])

    @swbase.run_for_backends('package-kit')
    @swbase.test_with_repos('stable', **{
            'updates'         : False,
            'updates-testing' : False,
            'misc'            : False
    })
    @swbase.test_with_packages(**{
            'pkg1' : False,
            'pkg2' : False,
            'pkg3' : False
    })
    def test_parallel_install(self):
        """
        Try to install multiple packages in parallel.
        """
        pkgs = [ self.get_repo('stable')['pkg1']
               , self.get_repo('stable')['pkg2']
               , self.get_repo('stable')['pkg3']
               ]
        processes = {}
        job_insts = {}
        successful_installations = 0
        queue = Queue()

        for pkg in pkgs:
            process = Process(target=self._do_test_single_pkg_install,
                    args=(pkg, queue))
            process.start()
            processes[pkg] = process

        while len(processes):
            for job_inst in job_insts.values():
                job_inst.refresh()
            while not queue.empty():
                pkg_name, value = queue.get()
                pkg = self.get_repo('stable')[pkg_name]
                self.assertTrue(pkg in pkgs)
                if isinstance(value, bool):
                    if value is True:
                        self.assertTrue(package.is_pkg_installed(pkg))
                        successful_installations += 1
                else:
                    self.assertFalse(pkg in job_insts)
                    jobnum = self.RE_INSTANCE_ID.match(value).group('number')
                    job_insts[pkg] = self.make_op(jobnum).to_instance()
            dead_thread_pkg = []
            for pkg, process in processes.items():
                if not process.is_alive():
                    dead_thread_pkg.append(pkg)
                    process.join()
            for pkg in dead_thread_pkg:
                del processes[pkg]
            if len(job_insts) > 1:
                # check that there is just one job running
                running = []
                for job_inst in job_insts.values():
                    if job_inst.JobState == JOB_STATE_RUNNING:
                        running.append(job_inst)
                    if len(running) > 1:
                        # jobs may have changed their states since the first
                        # one has been refreshed
                        finished = []
                        for rinst in running:
                            rinst.refresh()
                            if rinst.JobState != JOB_STATE_RUNNING:
                                finished.append(rinst)
                        for finst in finished:
                            running.remove(finst)
                        self.assertTrue(len(running) <= 1)
            time.sleep(0.1)
        while not queue.empty():
            pkg, value = queue.get()
            self.assertFalse(pkg in job_insts)
            self.assertTrue(pkg in pkgs)
            job_insts[pkg] = value
            if isinstance(value, bool) and value is True:
                successful_installations += 1
        self.assertEqual(len(job_insts), len(pkgs))
        intervals = []
        # check that each job was completed before another one started
        for job_inst in job_insts.values():
            for start, end in intervals:
                self.assertTrue(  job_inst.StartTime >= end
                               or job_inst.TimeOfLastStateChange <= start)
            intervals.append((job_inst.StartTime, job_inst.TimeOfLastStateChange))
        self.assertEqual(len(pkgs), successful_installations)

    @swbase.test_with_repos('stable', **{
            'updates'         : False,
            'updates-testing' : False,
            'misc'            : False
    })
    @swbase.test_with_packages('stable#pkg1')
    def test_install_already_installed_package(self):
        """
        Try to install already installed package.
        """
        pkg = self.get_repo('stable')['pkg1']
        affected_packages = (util.make_pkg_op(self.ns, pkg), )
        input_parameters = {
                "Source"         : util.make_pkg_op(self.ns, pkg),
                "InstallOptions" : [INSTALL_OPTIONS_INSTALL],
                "Target"         : self.system_iname
        }
        rval, oparms, _ = self.service_op.InstallFromSoftwareIdentity(
                **input_parameters)
        self.assertEqual(rval, METHOD_PARAMETERS_CHECKED_JOB_STARTED)
        inst = self._test_check_method_out_params(
                input_parameters, oparms,
                expected_states=(JOB_STATE_NEW, JOB_STATE_RUNNING),
                affected_packages=affected_packages)

        while inst.JobState in (JOB_STATE_NEW, JOB_STATE_RUNNING):
            inst.refresh()
            self._check_job_instance(inst.path, inst, input_parameters,
                    affected_packages=affected_packages)
            time.sleep(0.1)

        self._check_job_instance(inst.path, inst, input_parameters,
            expected_states=(JOB_STATE_EXCEPTION, ),
            expected_status_code=CIM_STATUS_CODE_ERR_ALREADY_EXISTS)

    @swbase.test_with_repos(**{
            'stable' : False,
            'updates' : False,
            'updates-testing' : False,
            'misc' : False})
    @swbase.test_with_packages(**{'pkg1' : False})
    def test_install_unavailable_package(self):
        """
        Check whether the error is correctly set for failed job.
        """
        pkg = self.get_repo('stable')['pkg1']
        affected_packages = (util.make_pkg_op(self.ns, pkg), )
        input_parameters = {
                "Source"         : util.make_pkg_op(self.ns, pkg),
                "InstallOptions" : [INSTALL_OPTIONS_INSTALL],
                "Target"         : self.system_iname
        }
        rval, oparms, _ = self.service_op.InstallFromSoftwareIdentity(
                **input_parameters)
        self.assertEqual(rval, METHOD_PARAMETERS_CHECKED_JOB_STARTED)

        inst = self._test_check_method_out_params(
                input_parameters, oparms,
                expected_states=(JOB_STATE_NEW, JOB_STATE_RUNNING),
                affected_packages=affected_packages,
                affected_allow_subset=True)

        while inst.JobState in (JOB_STATE_NEW, JOB_STATE_RUNNING):
            inst.refresh()
            self._check_job_instance(inst.path, inst, input_parameters,
                affected_packages=affected_packages,
                affected_allow_subset=True)
            time.sleep(0.1)

        self._check_job_instance(inst.path, inst, input_parameters,
            expected_states=JOB_STATE_EXCEPTION,
            expected_status_code=CIM_STATUS_CODE_ERR_NOT_FOUND,
            affected_packages=affected_packages,
            affected_allow_subset=True)

    @swbase.test_with_repos('stable', **{
            'updates'         : False,
            'updates-testing' : False,
            'misc'            : False
    })
    @swbase.test_with_packages(**{'pkg1' : False})
    def test_terminate_job(self):
        """
        Try to terminate running job.
        """
        pkg = self.get_repo('stable')['pkg1']
        affected_packages = (util.make_pkg_op(self.ns, pkg), )
        input_parameters = {
                "Source"         : util.make_pkg_op(self.ns, pkg),
                "InstallOptions" : [INSTALL_OPTIONS_INSTALL],
                "Target"         : self.system_iname
        }
        rval, oparms, _ = self.service_op.InstallFromSoftwareIdentity(
                **input_parameters)
        self.assertEqual(rval, METHOD_PARAMETERS_CHECKED_JOB_STARTED)
        self.assertTrue("Job" in oparms)

        inst = oparms['Job'].to_instance()
        self.assertNotEqual(inst, None)
        rval, oparms, errstr = inst.RequestStateChange(
                RequestedState=REQUEST_STATE_CHANGE_TERMINATE)

        if rval == REQUEST_STATE_CHANGE_COMPLETED_WITH_NO_ERROR:
            expected_states = (JOB_STATE_TERMINATED, )
            expected_status_code = CIM_STATUS_CODE_ERR_FAILED
        else:
            # job is completed with no error
            self.assertEqual(rval, -1)
            self.assertGreater(len(errstr), 0)
            expected_states = (JOB_STATE_COMPLETED, )
            expected_status_code = CIM_STATUS_CODE_OK

        while inst.JobState in (JOB_STATE_NEW, JOB_STATE_RUNNING):
            inst.refresh()
            self._check_job_instance(inst.path, inst, input_parameters,
                    affected_packages=affected_packages,
                    affected_allow_subset=True)
            time.sleep(0.1)

        self._check_job_instance(inst.path, inst,
            input_parameters,
            expected_states=expected_states,
            expected_status_code=expected_status_code,
            affected_packages=(util.make_pkg_op(self.ns, pkg), ))

    @enable_lmi_exceptions
    @swbase.test_with_repos('stable', 'updates', **{
            'updates-testing' : False,
            'misc'            : False
    })
    @swbase.test_with_packages('stable#pkg2')
    def test_terminate_completed_job(self):
        """
        Try to terminate completed package update job.
        """
        pkg = self.get_repo('stable')['pkg2']
        new = self.get_repo('updates')['pkg2']
        affected_packages = [
                util.make_pkg_op(self.ns, pkg),
                util.make_pkg_op(self.ns, new)]
        input_parameters = {
                "Source"         : affected_packages[0],
                "InstallOptions" : [INSTALL_OPTIONS_UPDATE],
                "Target"         : self.system_iname
        }
        rval, oparms, _ = self.service_op.InstallFromSoftwareIdentity(
                **input_parameters)
        inst = self._test_check_method_out_params(
                input_parameters, oparms,
                expected_states=(JOB_STATE_NEW, JOB_STATE_RUNNING),
                affected_packages=affected_packages,
                affected_allow_subset=True)

        while inst.JobState in (JOB_STATE_NEW, JOB_STATE_RUNNING):
            inst.refresh()
            self._check_job_instance(inst.path, inst, input_parameters,
                    affected_packages=affected_packages,
                    affected_allow_subset=True)
            time.sleep(0.1)

        self._check_job_instance(inst.path, inst,
            input_parameters,
            expected_states=JOB_STATE_COMPLETED,
            affected_packages=affected_packages)
        self.assertTrue(package.is_pkg_installed(new))

        self.assertRaisesCIM(wbem.CIM_ERR_FAILED,
            inst.RequestStateChange,
            RequestedState=REQUEST_STATE_CHANGE_TERMINATE)

        self._check_job_instance(inst.path, inst,
            input_parameters,
            expected_states=JOB_STATE_COMPLETED,
            affected_packages=affected_packages)

    @enable_lmi_exceptions
    @swbase.test_with_repos('updates-testing', 'misc', **{
        'stable'  : False,
        'updates' : False
    })
    @swbase.test_with_packages(**{
        'pkg1'    : False,
        'pkg2'    : False,
        'depend1' : False
    })
    def test_install_package_with_dependencies(self):
        """
        Check whether the job is associated with all packages installed."
        """
        dependent = self.get_repo('misc')['depend1']
        dependencies = [
            self.get_repo('updates-testing')['pkg1'],
            self.get_repo('updates-testing')['pkg2']]
        affected_packages = [util.make_pkg_op(self.ns, dependent), ]
        input_parameters = {
                "Source"         : affected_packages[0],
                "InstallOptions" : [INSTALL_OPTIONS_INSTALL],
                "Target"         : self.system_iname
        }
        rval, oparms, _ = self.service_op.InstallFromSoftwareIdentity(
                **input_parameters)

        self.assertEqual(rval, METHOD_PARAMETERS_CHECKED_JOB_STARTED)
        inst = self._test_check_method_out_params(
                input_parameters, oparms,
                expected_states=(JOB_STATE_NEW, JOB_STATE_RUNNING),
                affected_packages=affected_packages)

        while inst.JobState in (JOB_STATE_NEW, JOB_STATE_RUNNING):
            inst.refresh()
            self._check_job_instance(inst.path, inst, input_parameters)
            time.sleep(0.1)

        if self.backend != 'yum':
            # yum provider doesn't detect transaction's dependencies
            affected_packages.extend([
                util.make_pkg_op(self.ns, dep) for dep in dependencies])

        self._check_job_instance(inst.path, inst,
            input_parameters,
            expected_states=JOB_STATE_COMPLETED,
            affected_packages=affected_packages)

        for pkg in [dependent] + dependencies:
            self.assertTrue(package.is_pkg_installed(pkg))

        # let's remove it otherwise other tests will fail in removing
        # pkg1 and pkg2 due to installed depend1
        package.remove_pkgs(dependent)

    @swbase.test_with_repos('stable')
    @swbase.test_with_packages('stable#pkg1')
    def test_delete_job(self):
        """
        Try to delete package removal job.
        """
        pkg = self.get_repo('stable')['pkg1']
        affected_packages = [util.make_pkg_op(self.ns, pkg)]

        input_parameters = {
                "Source"         : util.make_pkg_op(self.ns, pkg),
                "InstallOptions" : [INSTALL_OPTIONS_UNINSTALL],
                "Target"         : self.system_iname
        }
        rval, oparms, _ = self.service_op.InstallFromSoftwareIdentity(
                **input_parameters)
        self.assertEqual(rval, METHOD_PARAMETERS_CHECKED_JOB_STARTED)
        inst = self._test_check_method_out_params(
                input_parameters, oparms,
                expected_states=(JOB_STATE_NEW, JOB_STATE_RUNNING),
                affected_packages=affected_packages)

        while inst.JobState in (JOB_STATE_NEW, JOB_STATE_RUNNING):
            inst.refresh()
            self._check_job_instance(inst.path, inst,
                    input_parameters,
                    affected_packages=affected_packages)
            time.sleep(0.1)

        self._check_job_instance(inst.path, inst,
                input_parameters,
                expected_states=set((JOB_STATE_COMPLETED, )),
                expected_result=JOB_COMPLETED_WITH_NO_ERROR,
                expected_status_code=CIM_STATUS_CODE_OK,
                affected_packages=affected_packages)

        results = inst.associator_names(
            AssocClass='LMI_AssociatedSoftwareJobMethodResult',
            ResultClass='LMI_SoftwareMethodResult',
            Role='Job',
            ResultRole='JobParameters')
        method_result = results[0]
        self.assertNotEqual(method_result.to_instance(), None)

        op = inst.path
        inst.delete()

        self.assertEqual(op.to_instance(), None)
        self.assertEqual(method_result.to_instance(), None)

def suite():
    """For unittest loaders."""
    return unittest.TestLoader().loadTestsFromTestCase(
            TestSoftwareInstallationJob)

if __name__ == '__main__':
    unittest.main()
