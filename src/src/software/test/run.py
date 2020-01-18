#!/usr/bin/python
# -*- Coding:utf-8 -*-
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
Preferably a ``suite()`` function should be defined there as well.
They must be named according to shell regexp: ``"test_*.py"``.
"""

import optparse
import copy
import getpass
import inspect
import os
import sys
import tempfile

from lmi.test import unittest
from lmi.test import wbem

import reposetup
import swbase
import util

class NotFound(Exception):
    """Raised when test of particular name could no be found."""
    def __init__(self, name):
        Exception.__init__(self, "Test \"%s\" could not be found!" % name)

def parse_cmd_line():
    """
    Use ArgumentParser to parse command line arguments.

    :returns: ``(parsed arguments object, arguments for unittest.main())``.
    :rtype: tuple
    """
    parser = optparse.OptionParser(
            add_help_option=False,     # handle help message ourselves
            description="Test OpenLMI Software providers. Arguments"
            " for unittest main function can be added after \"--\""
            " switch.")
    parser.add_option("--url",
            default=util.get_env('url'),
            help="Network port to use for tests")
    parser.add_option("-u", "--user",
            default=util.get_env('username'),
            help="User for broker authentication.")
    parser.add_option("-p", "--password",
            default=util.get_env('password'),
            help="User's password.")

    parser.add_option('-b', '--backend',
	    default=util.get_env('backend'),
	    choices=('yum', 'package-kit'),
	    help="What backend provider uses.")

    parser.add_option("--run-dangerous", action="store_true",
            default=util.get_env('run_dangerous'),
            help="Run also tests dangerous for this machine"
                 " (tests, that manipulate with software database)."
                 " Overrides environment variable LMI_RUN_DANGEROUS.")
    parser.add_option('--no-dangerous', action="store_false",
            dest="run_dangerous",
            default=util.get_env('run_dangerous'),
            help="Disable dangerous tests.")

    parser.add_option("--run-tedious", action="store_true",
            default=util.get_env('run_tedious'),
            help="Run also tedious (long running) for this machine."
                 " Overrides environment variable LMI_RUN_TEDIOUS.")
    parser.add_option('--no-tedious', action="store_false",
            dest="run_tedious",
            default=util.get_env('run_tedious'),
            help="Disable tedious tests.")

    parser.add_option("--cleanup-cache", action="store_true",
            default=util.get_env('cleanup_cache'),
            help="Clean up all temporary files created for testing purposes."
            " If LMI_SOFTWARE_DB_CACHE is set and this is set to False, next"
            " run will be much faster because testing database won't need to"
            " be generated again. Overrides environment variable"
            " LMI_SOFTWARE_CLEANUP_CACHE.")
    parser.add_option('--no-cleanup', action="store_false",
            dest="cleanup_cache",
            default=util.get_env('cleanup_cache'),
            help="Do not delete database cache file and created repositories."
            " This speeds up testing because testing repositories don't need"
            " to be recreated upon next run. This applies only when"
            " LMI_SOFTWARE_DB_CACHE is set.")

    parser.add_option('-l', '--list-tests', action="store_true",
            help="List all possible test names.")
    parser.add_option('-h', '--help', action="store_true",
            help="Show help message.")

    argv = copy.copy(sys.argv)
    rest = []
    if '--' in argv:
        index = argv.index('--')
        argv = argv[:index]
        rest = sys.argv[index + 1:]
    args, unknown_args = parser.parse_args(argv)
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
    the user for credentials.
    """
    user = args.user
    password = args.password
    try:
        wbem.WBEMConnection(args.url, (user, password))
    except wbem.ConnectionError as exc:
        user = args.user
        sys.stderr.write("Authentication error\n")
        if not user:
            user = raw_input("User: ")
        password = getpass.getpass()

def prepare_environment(args):
    """
    Set the environment for test scripts.

    :returns: Whether the database cache needs to be deleted.
    :rtype: boolean
    """
    os.environ['LMI_CIMOM_URL'] = args.url
    os.environ['LMI_CIMOM_USERNAME'] = args.user
    os.environ['LMI_CIMOM_PASSWORD'] = args.password
    os.environ['LMI_RUN_DANGEROUS'] = (
            '1' if args.run_dangerous else '0')
    os.environ["LMI_RUN_TEDIOUS"] = (
            '1' if args.run_tedious else '0')
    db_cache = util.get_env('db_cache')
    needs_cleanup = args.cleanup_cache or not bool(db_cache)
    if not db_cache or not os.path.exists(db_cache):
        repos_dir = tempfile.mkdtemp()
        repodb, other_repos = reposetup.make_object_database(repos_dir)
        db_cache = reposetup.save_test_database(
                repos_dir, repodb, other_repos, db_cache)
    os.environ["LMI_SOFTWARE_BACKEND"] = args.backend
    os.environ["LMI_SOFTWARE_DB_CACHE"] = db_cache
    os.environ['LMI_SOFTWARE_CLEANUP_CACHE'] = (
            '1' if needs_cleanup else '0')
    return needs_cleanup

def load_tests(loader, standard_tests, pattern):
    """
    Helper function for ``unittest.main()`` test loader.

    :returns: :py:class:`unittest.TestSuite` instance.
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

        :param string name: Name of test to find.
        :returns: Desired :py:class:`TestCase` or test function.
        """
        if (   isinstance(node, unittest.TestSuite)
           and node.countTestCases() > 0
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

    :param node: A test suite.
    :type node: :py:class:`unittest.TestSuite`
    :returns: ``[ test_name, ... ]``
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
    args, ut_args = parse_cmd_line()
    if args.list_tests:
        list_tests()
    try_connection(args)
    cleanup = prepare_environment(args)
    swbase.SwTestCase.setUpClass()
    try:
        test_program = unittest.main(__name__, argv=ut_args,
                testLoader=LMITestLoader(), exit=False)
    finally:
        swbase.SwTestCase.tearDownClass()
    sys.exit(0 if test_program.result.wasSuccessful() else 1)

if __name__ == '__main__':
    main()
