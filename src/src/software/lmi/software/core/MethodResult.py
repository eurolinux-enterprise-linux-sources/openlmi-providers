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
Just a common functionality related to class LMI_SoftwareMethodResult.
"""

import pywbem

from lmi.providers import cmpi_logging
from lmi.software import util
from lmi.software.core import InstMethodCall
from lmi.software.core import Job
from lmi.software.yumdb import jobs, errors, YumDB

@cmpi_logging.trace_function
def object_path2jobid(op):
    """
    @param op must contain precise InstanceID of job
    """
    if not isinstance(op, pywbem.CIMInstanceName):
        raise TypeError("op must be a CIMInstanceName")
    if not op["InstanceID"].lower().startswith('lmi:lmi_softwaremethodresult:'):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "Missing 'LMI:LMI_SoftwareMethodResult:' prefix in InstanceID.")
    instid = op['InstanceID'][len('LMI:LMI_SoftwareMethodResult:'):]
    try:
        instid = int(instid)
    except ValueError:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "Failed to parse InstanceID.")
    return instid

@cmpi_logging.trace_function
def object_path2job(op):
    """
    @param op must contain precise InstanceID of job
    """
    instid = object_path2jobid(op)
    try:
        return YumDB.get_instance().get_job(instid)
    except errors.JobNotFound:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND, "No such job.")

@cmpi_logging.trace_function
def job2model(env, job, keys_only=True, model=None):
    """
    This accepts job's id and produces corresponding result model.
    """
    if not isinstance(job, jobs.YumJob):
        raise TypeError("job must be an instance of YumJob")
    if model is None:
        model = util.new_instance_name("LMI_SoftwareMethodResult")
        if not keys_only:
            model = pywbem.CIMInstance("LMI_SoftwareMethodResult", path=model)
    model['InstanceID'] = "LMI:LMI_SoftwareMethodResult:"+str(job.jobid)
    if not keys_only:
        model.path['InstanceID'] = model['InstanceID']  #pylint: disable=E1103
        method_name = Job.JOB_METHOD_NAMES[job.metadata['method']]
        model['Caption'] = 'Result of method %s' % method_name
        model['Description'] = (
                'Result of asynchronous job number %d created upon invocation'
                " of %s's %s method." % (job.jobid,
                "LMI_SoftwareInstallationService", method_name))
        model['ElementName'] = 'MethodResult-%d' % job.jobid
        if job.state not in (job.NEW, job.RUNNING):
            model['PostCallIndication'] = pywbem.CIMProperty(
                    "PostCallIndication", type="instance",
                    value=InstMethodCall.job2model(env, job, pre=False))
        model['PreCallIndication'] = pywbem.CIMProperty("PreCallIndication",
                type="instance",
                value=InstMethodCall.job2model(env, job))
    return model

