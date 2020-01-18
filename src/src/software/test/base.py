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
import os
import tempfile
from subprocess import check_output

from lmi.test.lmibase import LmiTestCase
import repository
import rpmcache

def get_pkg_files(pkg):
    """
    Tries to make a the heterogenous and smallest set of test files from
    package.
    @param pkg must be installed package
    @return list of few files installed by pkg
    """
    cmd = ['rpm', '-ql', pkg.name]
    output = check_output(cmd)
    configs = set()
    docs = set()
    dirs = set()
    files = set()
    symlinks = set()
    for fpath in output.splitlines():   #pylint: disable=E1103
        if (   len(dirs) == 0
           and not os.path.islink(fpath)
           and os.path.isdir(fpath)):
            dirs.add(fpath)
        elif len(symlinks) == 0 and os.path.islink(fpath):
            symlinks.add(fpath)
        elif not os.path.islink(fpath) and os.path.isfile(fpath):
            if len(configs) == 0 and rpmcache.has_pkg_config_file(pkg, fpath):
                configs.add(fpath)
            elif len(docs) == 0 and rpmcache.has_pkg_doc_file(pkg, fpath):
                docs.add(fpath)
            elif len(files) == 0:
                files.add(fpath)
    out = list(configs) + list(docs) + list(dirs) + list(symlinks)
    if len(files) > 0 and len(docs) == 0 and len(symlinks) == 0:
        out += list(files)
    return out

class SoftwareBaseTestCase(LmiTestCase): #pylint: disable=R0904
    """
    Base class for all LMI Software test classes.
    """

    CLASS_NAME = "Define in subclass"
    KEYS = tuple()

    # will be filled when first needed
    # it's a dictionary with items (pkg_name, [file_path1, ...])
    PKGDB_FILES = None
    REPODB = None

    @classmethod
    def get_pkgdb_files(cls):
        """
        @return dictionary { pkg_name: ["file_path1, ...] }
        """
        if cls.PKGDB_FILES is not None:
            return cls.PKGDB_FILES
        SoftwareBaseTestCase.PKGDB_FILES = res = dict(
                (pkg.name, get_pkg_files(pkg)) for pkg in itertools.chain(
                    cls.safe_pkgs, cls.dangerous_pkgs))
        return res

    @classmethod
    def get_repodb(cls):
        """
        @return list of Repository instances
        """
        if cls.REPODB is not None:
            return cls.REPODB
        SoftwareBaseTestCase.REPODB = res = repository.get_repo_database()
        return res

    @classmethod
    def needs_pkgdb(cls):
        """subclass may override this, if it does not need PKGDB database"""
        return True

    @classmethod
    def needs_pkgdb_files(cls):
        """subclass may override this, if it needs PKGDB_FILES database"""
        return False

    @classmethod
    def needs_repodb(cls):
        """subclass may override this, if it needs REPODB database"""
        return False

    def __init__(self, *args, **kwargs):
        LmiTestCase.__init__(self, *args, **kwargs)
        self.longMessage = True     #pylint: disable=C0103

    def install_pkg(self, pkg, newer=True, repolist=None):
        """
        Use this method instead of function in rpmcache in tests.
        """
        if repolist is None:
            repolist = self.test_repos
        return rpmcache.install_pkg(pkg, newer, repolist)

    def ensure_pkg_installed(self, pkg, newer=True, repolist=None):
        """
        Use this method instead of function in rpmcache in tests.
        """
        if repolist is None:
            repolist = self.test_repos
        return rpmcache.ensure_pkg_installed(pkg, newer, repolist)

    @classmethod
    def setUpClass(cls):    #pylint: disable=C0103
        LmiTestCase.setUpClass.im_func(cls)

        # TODO: make dangerous tests work reliably
        # this is just a temporary solution
        cls.run_dangerous = False

        cls.test_repos = os.environ.get(
                'LMI_SOFTWARE_TEST_REPOS', '').split(',')
        use_cache = os.environ.get('LMI_SOFTWARE_USE_CACHE', '0') == '1'
        cls.cache_dir = None
        if use_cache:
            cls.cache_dir = os.environ.get('LMI_SOFTWARE_CACHE_DIR', None)
            if cls.cache_dir is None:
                cls.cache_dir = tempfile.mkdtemp(suffix="software_database")
            if cls.cache_dir:
                cls.prev_dir = os.getcwd()
                if not os.path.exists(cls.cache_dir):
                    os.makedirs(cls.cache_dir)
                # rpm packages are expected to be in CWD
                os.chdir(cls.cache_dir)
        if not hasattr(cls, 'safe_pkgs') or not hasattr(cls, 'dangerous_pkgs'):
            if cls.needs_pkgdb():
                safe, dangerous  = rpmcache.get_pkg_database(
                        use_cache=use_cache,
                        dangerous=cls.run_dangerous,
                        repolist=cls.test_repos,
                        cache_dir=cls.cache_dir if use_cache else None)
                SoftwareBaseTestCase.safe_pkgs = safe
                SoftwareBaseTestCase.dangerous_pkgs = dangerous
            else:
                cls.safe_pkgs = []
                cls.dangerous_pkgs = []
        if cls.needs_pkgdb_files() and not hasattr(cls, 'pkgdb_files'):
            for pkg in cls.dangerous_pkgs:
                if not rpmcache.is_pkg_installed(pkg.name):
                    rpmcache.install_pkg(pkg, repolist=cls.test_repos)
            SoftwareBaseTestCase.pkgdb_files = cls.get_pkgdb_files()
        if cls.needs_repodb() and not hasattr(cls, 'repodb'):
            SoftwareBaseTestCase.repodb = cls.get_repodb()

    @classmethod
    def tearDownClass(cls):     #pylint: disable=C0103
        if hasattr(cls, "repodb") and cls.repodb:
            # set the enabled states to original values
            for repo in cls.repodb:
                if repository.is_repo_enabled(repo) != repo.status:
                    repository.set_repo_enabled(repo, repo.status)
        if cls.run_dangerous:
            for pkg in cls.dangerous_pkgs:
                if rpmcache.is_pkg_installed(pkg.name):
                    rpmcache.remove_pkg(pkg.name)
        if hasattr(cls, "prev_dir"):
            os.chdir(cls.prev_dir)

