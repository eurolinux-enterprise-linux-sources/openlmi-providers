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
#
# Authors: Michal Minar <miminar@redhat.com>
"""
Pylint plugin supressing error concerning the use of cmpi_logging
facilities.
"""
from logilab.astng import scoped_nodes
from logilab.astng import MANAGER
from logilab.astng.builder import ASTNGBuilder
from pylint.interfaces import IASTNGChecker
from pylint.checkers import BaseChecker

def logging_transform(module):
    """
    Make the logging.RootLogger look, like it has following methods:
        * trace_warn
        * trace_info
        * trace_verbose
    These are actually defined in its subclass CMPILogger.
    But pylint thinks, that RootLogger is used all the time.
    """
    if module.name != 'logging' or 'RootLogger' not in module:
        return
    fake = ASTNGBuilder(MANAGER).string_build('''

def trace_warn(self, msg, *args, **kwargs):
    print(self, msg, args, kwargs)

def trace_info(self, msg, *args, **kwargs):
    print(self, msg, args, kwargs)

def trace_verbose(self, msg, *args, **kwargs):
    print(self, msg, args, kwargs)

''')
    for level in ('warn', 'info', 'verbose'):
        method_name = 'trace_' + level
        for child in module.locals['RootLogger']:
            if not isinstance(child, scoped_nodes.Class):
                continue
            child.locals[method_name] = fake.locals[method_name]

class CMPILoggingChecker(BaseChecker):
    """
    Checker that only disables some error messages of our custom
    cmpi_logging facilities.
    """

    __implements__ = IASTNGChecker

    name = 'cmpi_logging'
    # When we do have some real warning messages/reports,
    # remote W9921. We need it just to get registered.
    msgs = { 'W9921': ('Dummy', "This is a dummy message.")}

    def visit_class(self, node):
        """
        This supresses some warnings for CMPILogger class.
        """
        if (   isinstance(node.parent, scoped_nodes.Module)
           and node.parent.name.split('.')[-1] == "cmpi_logging"
           and node.name == "CMPILogger"):
            # supress warning about too many methods defined
            self.linter.disable('R0904', scope='module', line=node.lineno)

def register(linter):
    """
    Called when loaded by pylint --load-plugins.
    Let's register our tranformation function and checker here.
    """
    MANAGER.register_transformer(logging_transform)
    linter.register_checker(CMPILoggingChecker(linter))
