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
CIM values for enumeration types of CIM_InstMethodCall indication class.
"""

import pywbem

from lmi.providers import cmpi_logging
from lmi.software import util
from lmi.software.core import Job
from lmi.software.yumdb import jobs

class Values(object):
    class ReturnValueType(object):
        boolean = pywbem.Uint16(2)
        string = pywbem.Uint16(3)
        char16 = pywbem.Uint16(4)
        uint8 = pywbem.Uint16(5)
        sint8 = pywbem.Uint16(6)
        uint16 = pywbem.Uint16(7)
        sint16 = pywbem.Uint16(8)
        uint32 = pywbem.Uint16(9)
        sint32 = pywbem.Uint16(10)
        uint64 = pywbem.Uint16(11)
        sint64 = pywbem.Uint16(12)
        datetime = pywbem.Uint16(13)
        real32 = pywbem.Uint16(14)
        real64 = pywbem.Uint16(15)
        reference = pywbem.Uint16(16)
        # DMTF_Reserved = ..
        _reverse_map = {
                2: 'boolean',
                3: 'string',
                4: 'char16',
                5: 'uint8',
                6: 'sint8',
                7: 'uint16',
                8: 'sint16',
                9: 'uint32',
                10: 'sint32',
                11: 'uint64',
                12: 'sint64',
                13: 'datetime',
                14: 'real32',
                15: 'real64',
                16: 'reference'
        }

    class PerceivedSeverity(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Information = pywbem.Uint16(2)
        Degraded_Warning = pywbem.Uint16(3)
        Minor = pywbem.Uint16(4)
        Major = pywbem.Uint16(5)
        Critical = pywbem.Uint16(6)
        Fatal_NonRecoverable = pywbem.Uint16(7)
        # DMTF_Reserved = ..
        _reverse_map = {
                0: 'Unknown',
                1: 'Other',
                2: 'Information',
                3: 'Degraded/Warning',
                4: 'Minor',
                5: 'Major',
                6: 'Critical',
                7: 'Fatal/NonRecoverable'
        }

@cmpi_logging.trace_function
def job2model(env, job, pre=True):
    """
    Create post or pre indication instance used by clients to subscribe
    to job's state changes.

    :param job: (``YumJob``) Instance of job created as a result of method
        invocation.
    :param pre: (``bool``) says, whether to make pre or post indication
        instance.
    :rtype CIMInstance of CIM_InstMethodCall.
    """
    if not isinstance(job, jobs.YumJob):
        raise TypeError("job must be a YumJob")
    if not pre and job.state == job.NEW or job.state == job.RUNNING:
        raise ValueError("job must be finished to make a post indication"
            " instance")
    path = util.new_instance_name("CIM_InstMethodCall")
    inst = pywbem.CIMInstance(classname="CIM_InstMethodCall", path=path)
    src_instance = Job.job2model(job, keys_only=False)
    inst['SourceInstance'] = pywbem.CIMProperty("SourceInstance",
            type="instance", value=src_instance)
    inst['SourceInstanceModelPath'] = \
            str(src_instance.path)    #pylint: disable=E1103
    method_name = Job.JOB_METHOD_NAMES[job.metadata["method"]]
    inst['MethodName'] = method_name
    # TODO: uncomment when Pegasus can correctly handle instances
    # of unregistered classes
    #inst['MethodParameters'] = Job.make_method_params(
    #        job, '__MethodParameters', True, not pre)
    # TODO: until then, use this workaround
    if not pre:
        inst["MethodParameters"] = Job.make_method_params(
                job, '__MethodParameters_' + method_name, False, True)

    inst['PreCall'] = pre

    if not pre:
        inst["Error"] = pywbem.CIMProperty("Error", type="instance",
                is_array=True, value=[])
        error = Job.job2error(env, job)
        if error is not None:
            inst["Error"].append(error)
        inst["ReturnValueType"] = Values.ReturnValueType.uint32
        return_value = Job.make_return_value(job)
        if return_value is not None:
            inst["ReturnValue"] = str(return_value)

    return inst

