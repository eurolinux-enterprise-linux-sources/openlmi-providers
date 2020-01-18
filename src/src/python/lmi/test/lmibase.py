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
Base class and utilities for test suits written upon lmi shell.
"""

import functools
import inspect
import pywbem

from lmi.test import base
from lmi.shell import connect
from lmi.shell import LMIInstance
from lmi.shell import LMIInstanceName
from lmi.shell import LMIUtil

def enable_lmi_exceptions(method):
    """
    Function or method decorator enabling exceptions to be raised from under
    lmi shell intestines.
    """
    @functools.wraps(method)
    def _wrapper(*args, **kwargs):
        """ Enable exceptions in wrapped function. """
        original = LMIUtil.lmi_get_use_exceptions()
        LMIUtil.lmi_set_use_exceptions(True)
        try:
            retval = method(*args, **kwargs)
        finally:
            LMIUtil.lmi_set_use_exceptions(original)
        return retval

    return _wrapper

def to_cim_object(obj):
    """
    :returns: Wrapped object of from inside of shell abstractions.
    """
    if isinstance(obj, (LMIInstance, LMIInstanceName)):
        return obj.wrapped_object
    return obj

class LmiTestCase(base.BaseLmiTestCase):
    """
    Base class for all LMI test cases based on lmi shell.
    """

    #: Once you override this in subclass with a name of CIM class to be tested,
    #: you can use :py:attr:`LmiTestCase.cim_class` to get reference to a shell
    #: wrapper of this class.
    CLASS_NAME = None

    _SYSTEM_INAME = None

    @classmethod
    def setUpClass(cls):
        base.BaseLmiTestCase.setUpClass.im_func(cls)

    @property
    def conn(self):
        """
        :returns: Active connection to *CIMOM* wrapped by *lmi shell*
            abstraction.
        :rtype: :py:class:`lmi.shell.LMIConnection`
        """
        if not hasattr(self, '_shellconnection'):
            kwargs = {}
            con_argspec = inspect.getargspec(connect)
            # support older versions of lmi shell
            if 'verify_server_cert' in con_argspec.args or con_argspec.keywords:
                # newer name
                kwargs['verify_server_cert'] = False
            elif 'verify_certificate' in con_argspec.args:
                # older one
                kwargs['verify_certificate'] = False
            self._shellconnection = connect(
                    self.url, self.username, self.password, **kwargs)
        return self._shellconnection

    @property
    def ns(self):
        """
        :returns: Namespace object representing CIM ``"root/cimv2"`` namespace.
        :rtype: :py:class:`lmi.shell.LMINamespace`
        """
        return self.conn.root.cimv2

    @property
    def cim_class(self):
        """
        A convenience accessor to ``self.conn.root.cimv2.<CLASS_NAME>``.
        You need to override ``CLASS_NAME`` attribute of this class in order
        to use this.

        :returns: Lmi shell wrapper of CIM class to be tested.
        :rtype: :py:class:`lmi.shell.LMIClass`
        """
        if self.CLASS_NAME is None:
            return None
        return getattr(self.ns, self.CLASS_NAME)

    @property
    def system_iname(self):
        """
        :returns: Instance of ``CIM_ComputerSystem`` registered with *CIMOM*.
        :rtype: :py:class:`lmi.shell.LMIInstanceName`
        """
        if LmiTestCase._SYSTEM_INAME is None:
            LmiTestCase._SYSTEM_INAME = getattr(self.ns, self.system_cs_name) \
                    .first_instance_name()
        return LmiTestCase._SYSTEM_INAME.copy()

    def assertCIMIsSubclass(self, cls, base_cls):
        """
        Checks, whether cls is subclass of base_cls from CIM perspective.
        @param cls name of subclass
        @param base_cls name of base class
        """
        if not isinstance(cls, basestring):
            raise TypeError("cls must be a string")
        if not isinstance(base_cls, basestring):
            raise TypeError("base_cls must be a string")
        return self.assertTrue(pywbem.is_subclass(self.conn._client._cliconn,
            "root/cimv2", base_cls, cls))

    def assertRaisesCIM(self, cim_err_code, func, *args, **kwds):
        """
        This test passes if given function called with supplied arguments
        raises :py:class:`pywbem.CIMError` with given cim error code.
        """
        base.BaseLmiTestCase.assertRaisesCIM(
                self, cim_err_code, func, *args, **kwds)

    def assertCIMNameEqual(self, fst, snd, msg=None):
        """
        Compare two objects of :py:class:`pywbem.CIMInstanceName`. Their host
        properties are not checked.
        """
        base.BaseLmiTestCase.assertCIMNameEqual(
                self,
                to_cim_object(fst),
                to_cim_object(snd),
                msg)

    def assertCIMNameIn(self, name, candidates):
        """
        Checks that given :py:class:`pywbem.CIMInstanceName` is present in
        set of candidates. It compares all properties but ``host``.
        """
        name = to_cim_object(name)
        candidates = [to_cim_object(c) for c in candidates]
        base.BaseLmiTestCase.assertCIMNameIn(self, name, candidates)

