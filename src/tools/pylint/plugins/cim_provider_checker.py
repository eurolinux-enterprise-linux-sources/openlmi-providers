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
Pylint checker for CIM provider modules and classes.
"""

import re
from logilab.astng import node_classes # pylint: disable=W0403
from logilab.astng import scoped_nodes # pylint: disable=W0403
from pylint.interfaces import IASTNGChecker
from pylint.checkers import BaseChecker

_RE_PROVIDER_CLASS_NAME = re.compile(
    r'^(?P<prefix>[A-Z][a-zA-Z0-9]*)_(?P<name>[A-Z][a-zA-Z]*)$')
_RE_PROVIDER_MODULE_NAME = re.compile(
    r'^((?P<parent>.*)\.)?(?P<basename>(?P<prefix>[A-Z][a-zA-Z0-9]*)'
    r'_(?P<name>[A-Z][a-zA-Z]*))$')
_RE_COMMON_MODULE_NAME = re.compile(
    r'^([a-z_][a-z0-9_]*)|([A-Z][a-zA-Z0-9]+)$')

def recursively_clean_cls_values(linter, node):
    """
    Values class under provider defines property values, under
    nested classes that are popular target of pylint messages. Since
    these are generated and known to any developper, we don't want
    to be bothered by these warnings.

    Supress them for any nested class recursively.
    """
    # class has too few public methods
    linter.disable('R0903', scope='module', line=node.fromlineno)
    # class has not docstring
    linter.disable('C0111', scope='module', line=node.fromlineno)
    for child in node.get_children():
        if isinstance(child, scoped_nodes.Class):
            recursively_clean_cls_values(linter, child)

def supress_cim_provider_messages(linter, node):
    """
    Supress some warnings for CIMProvider2 subclass.
    @param node is a subclass of CIMProvider2
    """
    assert isinstance(node, scoped_nodes.Class)
    # class has too many public methods
    linter.disable('R0904', scope='module', line=node.fromlineno)
    if '__init__' in node:
        # __init__ not called for base class
        linter.disable('W0231', scope='module', line=node['__init__'].fromlineno)
    if (  'Values' in node
       and isinstance(node['Values'], scoped_nodes.Class)):
        recursively_clean_cls_values(linter, node['Values'])

    generated_methods = (
        'get_instance',
        'enum_instances',
        'set_instance',
        'delete_instance')
    # implementation of generated methods may not use all arguments
    for child in node.get_children():
        if (  not isinstance(child, scoped_nodes.Function)
           or (   child.name not in generated_methods
              and not child.name.startswith('cim_method_'))):
            continue
        # provider method could be a function
        linter.disable('R0201', scope='module', line=child.fromlineno)
        for arg in child.args.get_children():
            linter.disable('W0613', scope='module', line=arg.fromlineno)

class CIMProviderChecker(BaseChecker):
    """
    Checks for compliance to naming conventions for python cim providers.
    """

    __implements__ = IASTNGChecker

    name = 'cim_provider'
    msgs = {
        'C9905': ('Invalid provider module name %s',
                  "Module containing cim provider(s) should be named as "
                  "<prefix>_<name>. Where both <prefix> and <name> are "
                  "CamelCased strings."),
        'C9906': ('Prefixes of module and class does not match: %s != %s',
                  "Module prefix has to match provider class prefix."),
        'W9908': ("get_providers in module %s is not a function",
                  "get_providers should be a callable function."),
        'W9909': ("Missing provider name \"%s\" in providers dictionary.",
                  "Function get_providers returns providers association"
                  " dictionary, that must include all module's provider"
                  " classes."),
        'C9910': ("Prefix different from LMI: %s",
                  "All OpenLMI CIM classes have LMI_ ORGID prefix."),
        'C9911': ("Invalid module name: %s",
                  "All modules, that does not declare cim provider class"
                  " should be named according to this regexp:"
                  " '^(([a-z_][a-z0-9_]*)|([A-Z][a-zA-Z0-9]+))$'."),
        }

    # this is important so that your checker is executed before others
    priority = -2

    def visit_class(self, node):
        """
        Check every class, which inherits from
          pywbem.cim_provider2.CIMProvider2.
        """
        if "CIMProvider2" in [a.name for a in node.ancestors()]:
            supress_cim_provider_messages(self.linter, node)
            parent = node.parent
            while not isinstance(parent, scoped_nodes.Module):
                parent = parent.parent
            clsm = _RE_PROVIDER_CLASS_NAME.match(node.name)
            modm = _RE_PROVIDER_MODULE_NAME.match(parent.name)
            if clsm and not modm:
                self.add_message('C9905', node=node, args=parent.name)
            if clsm and modm and clsm.group('prefix') != modm.group('prefix'):
                self.add_message('C9906', node=node,
                        args=(modm.group('prefix'), clsm.group('prefix')))
            if clsm and clsm.group('prefix') != 'LMI':
                self.add_message('C9910', node=node, args=clsm.group('prefix'))
            if 'get_providers' in parent.keys():
                getprovs = parent['get_providers']
                if not isinstance(getprovs, scoped_nodes.Function):
                    self.add_message('W9908', node=getprovs, args=parent.name)
                ret = getprovs.last_child()
                if not isinstance(ret, node_classes.Return):
                    return
                dictionary = ret.get_children().next()
                if not isinstance(dictionary, node_classes.Dict):
                    return
                if not node.name in [i[0].value for i in dictionary.items]:
                    self.add_message('W9909', node=getprovs, args=node.name)

        elif node.name == "Values" and len([a for a in node.ancestors()]) == 1:
            recursively_clean_cls_values(self.linter, node)

    def visit_module(self, node):
        """
        Check for invalid module name.
        """
        modm = _RE_PROVIDER_MODULE_NAME.match(node.name)
        if modm:
            if not _RE_COMMON_MODULE_NAME.match(node.name):
                for child in node.get_children():
                    if (   isinstance(child, scoped_nodes.Class)
                       and 'CIMProvider2' in [
                           a.name for a in child.ancestors()]):
                        break
                else:
                    self.add_message('C9911', node=node, args=node.name)

def register(linter):
    """required method to auto register our checker"""
    linter.register_checker(CIMProviderChecker(linter))

