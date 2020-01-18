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
Common utilities concerning SystemSoftwareCollection provider.
"""

import pywbem

from lmi.providers import cmpi_logging
from lmi.software import util

def get_path():
    """@return instance name with prefilled properties"""
    op = util.new_instance_name("LMI_SystemSoftwareCollection",
            InstanceID="LMI:LMI_SystemSoftwareCollection")
    return op

@cmpi_logging.trace_function
def check_path(env, collection, prop_name):
    """
    Checks instance name of SystemSoftwareCollection.
    @param prop_name name of object name; used for error descriptions
    """
    if not isinstance(collection, pywbem.CIMInstanceName):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "\"%s\" must be a CIMInstanceName" % prop_name)
    our_collection = get_path()
    ch = env.get_cimom_handle()
    if collection.namespace != our_collection.namespace:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                'Namespace of "%s" does not match "%s"' % (
                    prop_name, our_collection.namespace))
    if not ch.is_subclass(our_collection.namespace,
            sub=collection.classname,
            super=our_collection.classname):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "Class of \"%s\" must be a sublass of %s" % (
                    prop_name, our_collection.classname))
    if not 'InstanceID' in collection:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "\"%s\" is missing InstanceID key property", prop_name)
    if collection['InstanceID'] != our_collection['InstanceID']:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "InstanceID of \"%s\" does not match \"%s\"" %
                prop_name, our_collection['InstanceID'])
    return True

@cmpi_logging.trace_function
def check_path_property(env, op, prop_name):
    """
    Checks, whether prop_name property of op object path is correct.
    If not, an exception will be risen.
    """
    if not prop_name in op:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "Missing %s key property!" % prop_name)
    return check_path(env, op[prop_name], prop_name)
