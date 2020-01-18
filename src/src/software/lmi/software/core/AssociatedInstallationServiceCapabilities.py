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
Just a common functionality related to associated class
LMI_AssociatedInstallationServiceCapabilities.
"""

import pywbem

class Values(object):
    class Characteristics(object):
        Default = pywbem.Uint16(2)
        Current = pywbem.Uint16(3)
        # DMTF_Reserved = ..
        # Vendor_Specific = 32768..65535
        _reverse_map = {
                2: 'Default',
                3: 'Current'
        }
