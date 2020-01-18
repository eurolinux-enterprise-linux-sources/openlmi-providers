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
Core functionality of particular providers.
Each provider having functionality useful to others has a submodule
in this subpackage with the same name except for LMI_ prefix.
"""

import pywbem

from lmi.providers import cmpi_logging

@cmpi_logging.trace_function
def generate_references(
        env,
        object_name,
        model,
        filter_result_class_name,
        filter_role,
        filter_result_role,
        keys_only,
        handlers):
    """
    Function for generating references to object_name. It's supposed
    to be used directly from references method of provider classes.
    @param handlers is a list of tuples:
        [ (role, class_name, handler), ... ]
      where handler is a function, that will be called for matching
        parameters, yielding instance(s/names). It accepts arguments:
            env, object_name, model, keys_only.
    """
    if not isinstance(object_name, pywbem.CIMInstanceName):
        raise TypeError("object_name must be a CIMInstanceName")
    if not isinstance(model, pywbem.CIMInstance):
        raise TypeError("model must be a CIMInstance")

    ch = env.get_cimom_handle()
    model.path.update(dict((t[0], None) for t in handlers))

    try:
        for i, (role, clsname, handler) in enumerate(handlers):
            if (  (   filter_role
                  and filter_role.lower() != role.lower())
               or not ch.is_subclass(object_name.namespace,
                            sub=object_name.classname,
                            super=clsname)):
                continue
            other_results = list((htuple[0], htuple[1]) for htuple in (
                handlers[:i] + handlers[i+1:]))
            for res_role, res_clsname in other_results:
                if (  (  not filter_result_role
                      or filter_result_role.lower() == res_role.lower())
                   and (not filter_result_class_name or ch.is_subclass(
                                object_name.namespace,
                                sub=filter_result_class_name,
                                super=res_clsname))):
                    break
            else:
                continue
            # matches filters
            for obj in handler(env, object_name, model, keys_only):
                yield obj
    except pywbem.CIMError as exc:
        if exc.args[0] != pywbem.CIM_ERR_NOT_FOUND:
            raise

