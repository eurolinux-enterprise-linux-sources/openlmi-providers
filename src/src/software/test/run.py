#!/usr/bin/python
# -*- Coding:utf-8 -*-
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
# Lesser General Public License for more details. #
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Authors: Radek Novacek <rnovacek@redhat.com>
# Authors: Michal Minar <miminar@redhat.com>
"""
Runs all defined tests.

Test modules are expected to be in directory with this script and
should contain subclasses of unittest.TestCase, that will be loaded.
Preferably a suite() function should be defined there as well.
They must be named according to shell regexp: "test_*.py".
"""

import argparse
import copy
import getpass
import inspect
import pywbem
import os
import shutil
import sys
import tempfile
import unittest

import rpmcache

# this is a global variable, that can be modified by environment or program
# argument
CACHE_DIR = ''

class NotFound(Exception):
    """Raised when test of particular name could no be found."""
    def __init__(self, name):
        Exception.__init__(self, "Test \"%s\" could not be found!" % name)

def parse_cmd_line():
    """
    Use ArgumentParser to parse command line arguments.
    @return (parsed arguments object, arguments for unittest.main())
    """
    parser = argparse.ArgumentParser(
            add_help=False,     # handle help message ourselves
            description="Test OpenLMI Software providers. Arguments"
            " for unittest main function can be added after \"--\""
            " switch.")
    parser.add_argument("--url",
            default=os.environ.get("LMI_CIMOM_URL", "http://localhost:5988"),
            help="Network port to use for tests")
    parser.add_argument("-u", "--user",
            default=os.environ.get("LMI_CIMOM_USERNAME", ''),
            help="User for broker authentication.")
    parser.add_argument("-p", "--password",
            default=os.environ.get("LMI_CIMOM_PASSWORD", ''),
            help="User's password.")

    dangerous_group = parser.add_mutually_exclusive_group()
    dangerous_group.add_argument("--run-dangerous", action="store_true",
            default=(os.environ.get('LMI_RUN_DANGEROUS', '0') == '1'),
            help="Run also tests dangerous for this machine"
                 " (tests, that manipulate with software database)."
                 " Overrides environment variable LMI_RUN_DANGEROUS.")
    dangerous_group.add_argument('--no-dangerous', action="store_false",
            dest="run_dangerous",
            default=os.environ.get('LMI_RUN_DANGEROUS', '0') == '1',
            help="Disable dangerous tests.")

    tedious_group = parser.add_mutually_exclusive_group()
    tedious_group.add_argument("--run-tedious", action="store_true",
            default=(os.environ.get('LMI_RUN_TEDIOUS', '0') == '1'),
            help="Run also tedious (long running) for this machine."
                 " Overrides environment variable LMI_RUN_TEDIOUS.")
    tedious_group.add_argument('--no-tedious', action="store_false",
            dest="run_tedious",
            default=os.environ.get('LMI_RUN_TEDIOUS', '0') == '1',
            help="Disable tedious tests.")

    cache_group = parser.add_mutually_exclusive_group()
    cache_group.add_argument("-c", "--use-cache", action="store_true",
            default=(os.environ.get('LMI_SOFTWARE_USE_CACHE', '0') == '1'),
            help="Use a cache directory to download rpm packages for"
            " testing purposes. It greatly speeds up testing."
            " Also a database file \"${LMI_SOFTWARE_CACHE_DIR}/%s\""
            " will be created to store information"
            " for this system. Overrides environment variable"
            " LMI_SOFTWARE_USE_CACHE." % rpmcache.DB_BACKUP_FILE)
    cache_group.add_argument("--no-cache", action="store_false",
            dest="use_cache",
            default=(os.environ.get('LMI_SOFTWARE_USE_CACHE', '0') == '1'),
            help="Do not cache rpm packages for speeding up tests."
            " Overrides environment variable LMI_SOFTWARE_USE_CACHE.")
    parser.add_argument('--cache-dir',
            default=os.environ.get('LMI_SOFTWARE_CACHE_DIR', ''),
            help="Use particular directory, instead of temporary one, to store"
            " rpm packages for testing purposes. Overrides environment"
            " variable LMI_SOFTWARE_CACHE_DIR.")
    parser.add_argument('--test-repos',
            default=os.environ.get('LMI_SOFTWARE_REPOSITORIES', ''),
            help="Use this option to specify list of repositories, that"
            " alone should be used for testing. Overrides environment"
            " variable LMI_SOFTWARE_REPOSITORIES.")
    parser.add_argument('--test-packages',
            default=os.environ.get('LMI_SOFTWARE_PACKAGES', ''),
            help="Specify packages for dangerous tests. If empty"
            " and cache is enabled, some will be picked up by algorithm")
    parser.add_argument('--force-update', action="store_true",
            help="Force update of package database. Otherwise an old"
            " one will be used (if any exists).")
    parser.add_argument('-l', '--list-tests', action="store_true",
            help="List all possible test names.")
    parser.add_argument('-h', '--help', action="store_true",
            help="Show help message.")

    argv = copy.copy(sys.argv)
    rest = []
    if '--' in argv:
        index = argv.index('--')
        argv = argv[:index]
        rest = sys.argv[index + 1:]
    args, unknown_args = parser.parse_known_args(argv)
    if args.help:
        parser.print_help()
        print
        print "*"*79
        print " Unit test options (parsed after \"--\" switch)."
        print "*"*79
        print
        unittest.main(argv=[argv[0], "--help"])
    return (args, unknown_args + rest)

def try_connection(args):
    """
    Try to connect to cim broker. If authentication fails, ask
    the user for credentials in loop.
    @return (user, password)
    """
    user = args.user
    password = args.password
    while False:
        try:
            pywbem.WBEMConnection(args.url, (user, password))
            break
        except pywbem.cim_http.AuthError as exc:
            user = args.user
            sys.stderr.write("Authentication error\n")
            if exc.args[0] == pywbem.CIM_ERR_ACCESS_DENIED:
                if not user:
                    user = raw_input("User: ")
                password = getpass.getpass()

def prepare_environment(args):
    """
    Set the environment for test scripts.
    """
    os.environ['LMI_CIMOM_URL'] = args.url
    os.environ['LMI_CIMOM_USERNAME'] = args.user
    os.environ['LMI_CIMOM_PASSWORD'] = args.password
    os.environ['LMI_RUN_DANGEROUS'] = (
            '1' if args.run_dangerous else '0')
    os.environ["LMI_RUN_TEDIOUS"] = (
            '1' if args.run_tedious else '0')
    os.environ['LMI_SOFTWARE_USE_CACHE'] = '1' if args.use_cache else '0'
    if args.use_cache:
        os.environ['LMI_SOFTWARE_CACHE_DIR'] = CACHE_DIR
    os.environ['LMI_SOFTWARE_REPOSITORIES'] = args.test_repos
    os.environ['LMI_SOFTWARE_PACKAGES'] = args.test_packages

def load_tests(loader, standard_tests, pattern):
    """
    Helper function for unittest.main() test loader.
    @return TestSuite instance
    """
    this_dir = os.path.dirname(__file__)
    if standard_tests:
        suite = standard_tests
    else:
        suite = unittest.TestSuite()
    discover_kwargs = dict(start_dir=this_dir)
    if pattern is not None:
        discover_kwargs['pattern'] = pattern
    tests = loader.discover(**discover_kwargs)
    suite.addTests(tests)
    return suite

class LMITestLoader(unittest.TestLoader):
    """
    Customized test loader to make invoking particular tests a lot easier.
    """

    @staticmethod
    def find_in_test_nodes(node, name):
        """
        Traverses suite tree nodes to find a test named name.
        @param name is a name of test to find
        @return desired TestCase or test function
        """
        if (   isinstance(node, unittest.TestSuite)
           and isinstance(next(iter(node)), unittest.TestCase)
           and next(iter(node)).__class__.__name__ == name):
            return node
        for subnode in node:
            if isinstance(subnode, unittest.TestSuite):
                subnode = LMITestLoader.find_in_test_nodes(subnode, name)
                if subnode is not None:
                    return subnode
            elif isinstance(subnode, unittest.TestCase):
                if (  name == subnode.__class__.__name__
                   or name == subnode._testMethodName):
                    return subnode
            elif inspect.isfunction(subnode) and name == subnode.__name__:
                return subnode

    def loadTestsFromName(self, name, module=None):     #pylint: disable=C0103
        """
        Override of parent method to make test cases accessible to by their
        names from command line.
        """
        suite = load_tests(self, None, 'test_*')
        parts = name.split('.')
        node = suite
        for part in parts:
            node = LMITestLoader.find_in_test_nodes(node, part)
            if node is None:
                raise NotFound(name)
        return node

def unwind_test_suite_tree(node):
    """
    Make a list of test names out of TestSuite.
    @param node is a suite
    @return [ test_name, ... ]
    """
    result = []
    for subnode in node:
        if isinstance(subnode, unittest.TestSuite):
            result.extend(unwind_test_suite_tree(subnode))
        elif isinstance(subnode, unittest.TestCase):
            result.append( subnode.__class__.__name__
                         + '.'+subnode._testMethodName)
        elif inspect.isfunction(subnode):
            result.append(subnode.__name__)
    return result

def list_tests():
    """
    Prints all available tests to stdout and exits.
    """
    suite = load_tests(LMITestLoader(), None, None)
    test_names = unwind_test_suite_tree(suite)
    print "\n".join(test_names)
    sys.exit(0)

def main():
    """
    Main functionality of script.
    """
    global CACHE_DIR
    args, ut_args = parse_cmd_line()
    if args.list_tests:
        list_tests()
    if args.use_cache and not args.cache_dir:
        CACHE_DIR = tempfile.mkdtemp(suffix="software_database")
    elif args.use_cache:
        CACHE_DIR = args.cache_dir
        if not os.path.exists(args.cache_dir):
            os.makedirs(args.cache_dir)
    try_connection(args)
    if args.test_repos:
        repolist = args.test_repos.split(',')
    else:
        repolist = []
    rpmcache.get_pkg_database(
            args.force_update,
            args.use_cache,
            dangerous=args.run_dangerous,
            cache_dir=CACHE_DIR,
            repolist=repolist)
    prepare_environment(args)
    test_program = unittest.main(__name__, argv=ut_args,
            testLoader=LMITestLoader(), exit=False)
    if args.use_cache and not args.cache_dir:
        shutil.rmtree(CACHE_DIR)
    sys.exit(test_program.result.wasSuccessful())

if __name__ == '__main__':
    main()
