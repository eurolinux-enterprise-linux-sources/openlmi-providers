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
LMI test utilities.
"""

import os
import pywbem
import socket
import unittest

def is_this_system(system_name):
    """
    :returns: Whether the given *system_name* matches the hostname of currently
    running system.
    :rtype: boolean
    """
    return (  socket.gethostbyaddr(system_name)[0]
           == socket.gethostbyaddr(socket.gethostname())[0])

def get_environvar(variable, default='', convert=str):
    """
    Get the value of environment variable.

    :param string variable: Name of environment variable.
    :param default: Any value that should be returned when the variable is not
        set. If None, the conversion won't be done.
    :param callable convert: Function transforming value to something else.
    :returns: Converted value of the environment variable.
    """
    val = os.environ.get(variable, default)
    if convert is bool:
        return val.lower() in ('true', 'yes', 'on', '1')
    if val is None:
        return None
    return convert(val)

def mark_dangerous(method):
    """
    Decorator for methods of :py:class:`unittest.TestCase` subclasses that
    skips dangerous tests if an environment variable says so.
    ``LMI_RUN_DANGEROUS`` is the environment variabled read.

    These tests will be skipped by default.
    """
    if get_environvar('LMI_RUN_DANGEROUS', '0', bool):
        return method
    else:
        return unittest.skip("This test is marked as dangerous.")(method)

def mark_tedious(method):
    """
    Decorator for methods of :py:class:`unittest.TestCase` subclasses that
    skips tedious tests. Those running for very long time and usually need a
    lot of memory. They are run by default. Environment variable
    ``LMI_RUN_TEDIOUS`` can be used to skip them.
    """
    if get_environvar('LMI_RUN_TEDIOUS', '1', bool):
        return method
    else:
        return unittest.skip("This test is marked as tedious.")(method)

def check_inames_equal(fst, snd):
    """
    Compare two objects of :py:class:`pywbem.CIMInstanceName`. Their ``host``
    property is not checked. Be benevolent when checking names system
    creation class names.

    :returns: ``True`` if both instance names are equal.
    :rtype: boolean
    """
    if not isinstance(fst, pywbem.CIMInstanceName):
        raise TypeError("fst argument must be a pywbem.CIMInstanceName, not %s"
                % repr(fst))
    if not isinstance(snd, pywbem.CIMInstanceName):
        raise TypeError("snd argument must be a pywbem.CIMInstanceName, not %s"
                % repr(snd))
    if fst.classname != snd.classname or fst.namespace != snd.namespace:
        return False

    snd_keys = { k: v for k, v in snd.keybindings.iteritems() }
    for key, value in fst.keybindings.iteritems():
        if key not in snd_keys:
            return False
        snd_value = snd_keys.pop(key)
        if (   isinstance(value, pywbem.CIMInstanceName)
           and isinstance(snd_value, pywbem.CIMInstanceName)):
            if not check_inames_equal(value, snd_value):
                return False

        # accept also aliases in the Name attribute of ComputerSystem
        elif (  (   fst.classname.endswith('_ComputerSystem')
                and key.lower() == 'name')
             or (   key.lower() == 'systemname'
                and 'SystemCreationClassName' in fst)):
            if (  value != snd_value
               and (  not is_this_system(value)
                   or not is_this_system(snd_value))):
                return False

        elif (   fst.classname.endswith('_ComputerSystem')
             and key.lower() == 'creationclassname'):
            if (   value != snd_value
               and 'CIM_ComputerSystem' not in [
                   p['CreationClassName'] for p in (fst, snd)]):
                return False

        elif value != snd_value:
            return False

    if snd_keys:    # second path has more key properties than first one
        return False

    return True

