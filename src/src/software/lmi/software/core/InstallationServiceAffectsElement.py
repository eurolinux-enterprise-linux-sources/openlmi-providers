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
Just a common functionality related to class
LMI_SoftwareInstallationServiceAffectsElement.
"""

import pywbem

from lmi.providers import cmpi_logging
from lmi.providers import ComputerSystem
from lmi.software.core import Identity

class Values(object):
    class ElementEffects(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        Exclusive_Use = pywbem.Uint16(2)
        Performance_Impact = pywbem.Uint16(3)
        Element_Integrity = pywbem.Uint16(4)
        Manages = pywbem.Uint16(5)
        Consumes = pywbem.Uint16(6)
        Enhances_Integrity = pywbem.Uint16(7)
        Degrades_Integrity = pywbem.Uint16(8)
        Enhances_Performance = pywbem.Uint16(9)
        Degrades_Performance = pywbem.Uint16(10)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..0xFFFF
        _reverse_map = {
                0  : 'Unknown',
                1  : 'Other',
                2  : 'Exclusive Use',
                3  : 'Performance Impact',
                4  : 'Element Integrity',
                5  : 'Manages',
                6  : 'Consumes',
                7  : 'Enhances Integrity',
                8  : 'Degrades Integrity',
                9  : 'Enhances Performance',
                10 : 'Degrades Performance'
        }

@cmpi_logging.trace_function
def fill_model_computer_system(env, model, keys_only=True):
    """
    Fills model's AffectedElement and all non-key properties.
    """
    model["AffectedElement"] = ComputerSystem.get_path(env)
    if not keys_only:
        model["ElementEffects"] = [
                Values.ElementEffects.Enhances_Integrity,
                Values.ElementEffects.Degrades_Integrity,
                Values.ElementEffects.Other,
                Values.ElementEffects.Other]
        model["OtherElementEffectsDescriptions"] = [
                "Enhances Integrity",
                "Degrades Integrity",
                "Enhances Functionality",
                "Degrades Functionality"]
    return model

@cmpi_logging.trace_function
def fill_model_identity(model, pkg, keys_only=True, identity_model=None):
    """
    Fills model's AffectedElement and all non-key properties.
    """
    model["AffectedElement"] = Identity.pkg2model(pkg, model=identity_model)
    if not keys_only:
        model["ElementEffects"] = [Values.ElementEffects.Other]
        model["OtherElementEffectsDescriptions"] = ["Allows to install"]
    return model
