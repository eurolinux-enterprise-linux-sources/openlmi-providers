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
# Authors: Michal Minar <miminar@redhat.com>
#
"""
Common utilities and base class for all software tests.
"""

import itertools
import functools
import os
import re
import shutil
import subprocess
import tempfile

from lmi.test.lmibase import LmiTestCase
import package
import repository
import reposetup

RE_REPOPKG = re.compile(r'^(?P<repo>[a-z-]+)#(?P<pkg_name>\w+.*|\*)', re.I)

def test_with_repos(*enable_repos, **repos_dict):
    """
    Decorator factory for test case methods. It enables or disables
    specified repositories for the time of method execution.

    It accepts shortened form of test repositories e.g. *stable* instead of
    *openlmi-sw-test-repo-stable*.

    Usage: ::

        @test_with_repos('stable', **{'updates' : False})
        def test_method(self):
            pass

    Repository names that shall be enabled can be passed as string arguments.
    Those that shall be disabled need to be passed as keyword arguments with
    ``False`` assigned to them.
    """

    def _decorator(func):
        """ Returns wrapped function. """
        @functools.wraps(func)
        def _wrapper(self, *args, **kwargs):
            """ This is a wrapper taking care of repository enablement. """
            to_enable = set(reposetup.full_repo_name(r) for r in enable_repos)
            to_enable.update(
                    set(reposetup.full_repo_name(r)
                for r, v in repos_dict.items() if v))
            to_enable_final = []
            to_disable = []
            for repoid in self.repodb:
                enable = (  repoid in to_enable
                         or bool(repos_dict.get(repoid, False)))
                repo = self.repodb[repoid]
                repo.refresh()
                if enable != repo.status:
                    if enable:
                        to_enable_final.append(repoid)
                    else:
                        to_disable.append(repoid)
            repository.set_repos_enabled(to_enable_final, True)
            repository.set_repos_enabled(to_disable, False)
            for repoid in itertools.chain(to_enable_final, to_disable):
                self.repodb[repoid].refresh()
            return func(self, *args, **kwargs)

        return _wrapper

    return _decorator

def test_with_packages(*install_pkgs, **enable_dict):
    """
    Decorator factory for test case methods. Returned decorator takes care of
    installation or removal of specified packages for the time of method's
    execution.

    Packages needs to be selected from testing database. They are specified in
    form: ::

        repository#package

    Where *repository* marks its full or shortened name. The same applies to
    packages ( *pkg1* instead of *openlmi-sw-test-pkg1* can be written).

    Star ``*`` is also accepted in place of package name marking any package
    present in particular repository.

    When supplied as a key-value pairs, value denote whether package (key)
    shall be installed or removed. Packages to be removed may also omit
    repository part.

    Usage: ::

        @test_with_packages('stable#pkg1', **{
            'updates#pkg2' : True,
            'pkg3' : False
        })
        def test_method(self):
            pass

    Example above shows two possible ways of specifying packages to install.
    Those are:
        * openlmi-sw-test-pkg1 from stable repository
        * openlmi-sw-test-pkg2 from updates repository
    Package openlmi-sw-test-pkg3 will be removed regardless of the source
    repository.
    """

    def _decorator(func):
        @functools.wraps(func)
        def _wrapper(self, *args, **kwargs):
            to_install = []
            to_remove = set()
            for repopkg in itertools.chain(install_pkgs, enable_dict.keys()):
                match = RE_REPOPKG.match(repopkg)
                if match:
                    repo = self.repodb[
                            reposetup.full_repo_name(match.group('repo'))]
                    pkg_name = match.group('pkg_name')
                    if pkg_name == '*':
                        pkgs = list(repo.packages)
                    else:
                        pkgs = [repo[reposetup.full_pkg_name(pkg_name)]]
                    for pkg in pkgs:
                        if not enable_dict.get(repopkg, True):
                            to_remove.add(pkg)
                        else:
                            to_install.append(pkg)
                elif enable_dict.get(repopkg, True) is False:
                    # package is given as string (with just name)
                    to_remove.add(reposetup.full_pkg_name(repopkg))
                    continue
                else:
                    raise ValueError(
                            'invalid format of repo#package string: "%s"'
                            % repopkg)
            to_remove = package.filter_installed_packages(
                    to_remove.union(set(p.name for p in to_install)))
            package.remove_pkgs(to_remove, suppress_stderr=True)
            to_install = package.filter_installed_packages(
                    to_install, installed=False)
            package.install_pkgs(to_install)
            return func(self, *args, **kwargs)

        return _wrapper

    return _decorator

class SwTestCase(LmiTestCase):
    """
    Base class for all LMI Software test classes.

    There are few important class properties to note:

        ``repodb`` : dictionary
            Is a dictionary of testing repositories. These are created upon
            set up of this class. It has following format: ::

                ( repoid, repository)

            Where repository is an instance of
            ;py:class:`repository.Repository`.
        ``other_repos`` : dictionary
            Has the same format but contains other repositories not present in
            ``repodb``. These are disabled upon class's set up and re-enabled
            on its tear down.

    These additional environment variables affects the execution:

        ``LMI_SOFTWARE_YUM_REPOS_DIR``
            Absolute path to directory used to store yum repository
            configuration files.
        ``LMI_SOFTWARE_DB_CACHE``
            File path to a serialized testing database which was cached in
            previous runs.
        ``LMI_SOFTWARE_CLEANUP_CACHE`` : defaults to '1'
            Boolean flag indicating whether, the cache shall be deleted. This
            includes temporary directory with testing repositories and their
            configuration files and serialized database file.
    """
    #: Define in subclass.
    KEYS = tuple()
    #: When running multiple test modules under same process, the test
    #: repository will be cached until last test case is destroyed. This
    #: greatly speeds up the testing.
    TEST_CASES_INSTANTIATED = 0

    def __init__(self, *args, **kwargs):
        LmiTestCase.__init__(self, *args, **kwargs)
        self.longMessage = True

    def get_repo(self, repoid):
        """
        Get repository object from its name. Prefer testin repositories.
        Accepts shortened names of testing repositories.

        :param string repoid: Repository id which may be shortened in case of
            testing one.
        :returns: Repository object.
        :rtype: :py:class:`repository.Repository`
        """
        try:
            return self.repodb[repoid]
        except KeyError:
            try:
                return self.repodb[reposetup.full_repo_name(repoid)]
            except KeyError:
                return self.other_repos[repoid]

    @classmethod
    def setUpClass(cls):
        LmiTestCase.setUpClass.im_func(cls)
        # This applies to all the commands whose output needs to be parsed.
        # Make sure we don't need to deal with localisations.
        os.environ['LANG'] = 'C'
        SwTestCase.yum_repos_dir = os.environ.get(
                'LMI_SOFTWARE_YUM_REPOS_DIR', '/etc/yum.repos.d')
        SwTestCase._cleanup_db = (
                    os.environ.get('LMI_SOFTWARE_CLEANUP_CACHE', '1')
                in  ('1', 'yes', 'on', 'true'))
        if SwTestCase.TEST_CASES_INSTANTIATED == 0:
            if os.environ.get('LMI_SOFTWARE_DB_CACHE', None):
                db_cache = os.environ['LMI_SOFTWARE_DB_CACHE']
                SwTestCase.repos_dir, SwTestCase.repodb, \
                        SwTestCase.other_repos = \
                            reposetup.load_test_database(db_cache)
            if (   not hasattr(SwTestCase, 'repos_dir')
               or not os.path.exists(SwTestCase.repos_dir)
               or any(   not os.path.exists(r.config_path)
                     for r in cls.repodb.values())):
                SwTestCase.repos_dir = tempfile.mkdtemp()
                SwTestCase.repodb, SwTestCase.other_repos = \
                        reposetup.make_object_database(SwTestCase.repos_dir)
            SwTestCase._restore_repos = []
            for repoid in repository.get_repo_list('enabled'):
                if repoid in SwTestCase.other_repos \
                        and repository.is_repo_enabled(repoid):
                    SwTestCase._restore_repos.append(repoid)
            repository.set_repos_enabled(SwTestCase._restore_repos, False)
            for repoid in SwTestCase._restore_repos:
                SwTestCase.other_repos[repoid].refresh()
        SwTestCase.TEST_CASES_INSTANTIATED += 1

    @classmethod
    def tearDownClass(cls):
        if SwTestCase.TEST_CASES_INSTANTIATED <= 1:
            to_uninstall = set()
            for repo in SwTestCase.repodb.values():
                for pkg in repo.packages:
                    to_uninstall.add(pkg.name)
                if SwTestCase._cleanup_db:
                    os.remove(repo.config_path)
            if not SwTestCase._cleanup_db:
                repository.set_repos_enabled(SwTestCase.repodb.keys(), False)
            to_uninstall = package.filter_installed_packages(to_uninstall)
            if to_uninstall:
                package.remove_pkgs(to_uninstall, suppress_stderr=True)
            repository.set_repos_enabled(cls._restore_repos, True)
            if SwTestCase._cleanup_db:
                shutil.rmtree(cls.repos_dir)
                if os.environ.get('LMI_SOFTWARE_DB_CACHE', None):
                    os.remove(os.environ['LMI_SOFTWARE_DB_CACHE'])
            cls.repodb.clear()
        SwTestCase.TEST_CASES_INSTANTIATED -= 1

