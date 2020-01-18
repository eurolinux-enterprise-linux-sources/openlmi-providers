# -*- encoding: utf-8 -*-
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
#
"""Pylint plugin to check OpenLMI unittest modules."""

from logilab.astng import scoped_nodes
from pylint.interfaces import IASTNGChecker
from pylint.checkers import BaseChecker
import unittest

def is_test_case_method(node):
    """
    Check, whether node method is defined by TestCase base class.
    """
    if (   isinstance(node, scoped_nodes.Function)
       and node.name in unittest.TestCase.__dict__):
        return True
    return False

def is_test_case_subclass(node):
    """
    Check, whether node is a subclass of TestCase base class.
    """
    if not isinstance(node, scoped_nodes.Class):
        return False
    for ancestor in node.ancestors():
        if (   ancestor.name == 'TestCase'
           and ancestor.getattr('__module__')[0].value.startswith('unittest')):
            return True
    return False

class TestCaseChecker(BaseChecker):
    """
    Checker for OpenLMI unittest modules.
    Right now it just suppresses unwanted messages.
    """

    __implements__ = IASTNGChecker

    name = 'unittest_case'
    # When we do have some real warning messages/reports,
    # remote W9920. We need it just to get registered.
    msgs = { 'W9920': ('Dummy', "This is a dummy message.")}

    priority = -1

    def visit_class(self, node):
        """
        Suppress Naming and 'Attribute creation out of __init__'
        errors for every TestCase subclass.
        """
        if not is_test_case_subclass(node):
            return
        if 'setUp' in node:
            self.linter.disable('W0201',
                    scope='module', line=node['setUp'].lineno)
        for child in node.get_children():
            if not is_test_case_method(child):
                continue
            self.linter.disable('C0103',
                    scope='module', line=child.lineno)

def register(linter):
    """required method to auto register our checker"""
    linter.register_checker(TestCaseChecker(linter))

