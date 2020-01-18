# -*- encoding: utf-8 -*-
# Software Management Providers
#
# Copyright (C) 2012 Red Hat, Inc.  All rights reserved.
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
# Authors: Jan Safranek <jsafrane@redhat.com>
"""
Pylint checker for OpenLMI storage modules and classes.
"""

import re
from pylint.interfaces import IASTNGChecker
from pylint.checkers import BaseChecker

import pdb

class LMIStorageChecker(BaseChecker):
    """
    Add/remove checks specific for openlmi.storage
    """
    name = 'lmi_storage'
    msgs = { 'W9922': ('Dummy', "This is a dummy message.")}
    __implements__ = IASTNGChecker

    # this is important so that your checker is executed before others
    priority = -2

    def visit_module(self, node):
        """
        Suppress R0201: Method could be a function
        """
        if node.name.startswith("openlmi.storage"):
            self.linter.disable('R0201')

def register(linter):
    """required method to auto register our checker"""
    linter.register_checker(LMIStorageChecker(linter))
