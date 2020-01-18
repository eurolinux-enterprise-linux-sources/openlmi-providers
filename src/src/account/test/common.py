# -*- encoding: utf-8 -*-
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
# Authors: Roman Rakus <rrakus@redhat.com>
#
"""
Base class and utilities for all OpenLMI Account tests.
"""

import os

from lmi.test import cimbase

class AccountBase(cimbase.CIMTestCase):
    """
    Base class for all LMI Account tests.
    """

    @classmethod
    def setUpClass(cls):
        cimbase.CIMTestCase.setUpClass.im_func(cls)
        cls.user_name = os.environ.get("LMI_ACCOUNT_USER")
        cls.group_name = os.environ.get("LMI_ACCOUNT_GROUP")

