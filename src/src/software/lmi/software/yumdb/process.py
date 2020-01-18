# -*- encoding: utf-8 -*-
# Software Management Providers
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
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Authors: Michal Minar <miminar@redhat.com>
#
"""
Module holding the code of separate process accessing the YUM API.
"""

import errno
from functools import wraps
from itertools import chain
import logging
from multiprocessing import Process
import os
import Queue as TQueue   # T as threaded
import re
import sys
import time
import traceback
from urlgrabber.grabber import default_grabber
import weakref
import yum

from lmi.providers import cmpi_logging
from lmi.software import util
from lmi.software.yumdb import errors
from lmi.software.yumdb import jobs
from lmi.software.yumdb import packageinfo
from lmi.software.yumdb import packagecheck
from lmi.software.yumdb import repository
from lmi.software.yumdb.jobmanager import JobManager
from lmi.software.yumdb.util import setup_logging

# Global variable which gets its value after the start of YumWorker process.
LOG = None

# *****************************************************************************
# Utilities
#  ****************************************************************************
def _get_package_filter_function(filters, exact_match=True):
    """
    :param filters: (``dict``) Dictionary with keys of package property
        names and values of their desired values.
    :param exact_match: (``bool``) Whether the ``name`` should be checked
        for exact match or for presence in package's name or summary
        strings.
    :rtype: (``function``) A function used to filter list of packages.
    """
    if not isinstance(filters, dict):
        raise TypeError("filters must be a dictionary")

    filters = dict((k, value) for k, value in filters.items()
                              if value is not None)

    match = None
    if "nevra" in filters:
        match = util.RE_NEVRA.match(filters["nevra"])
    elif "envra" in filters:
        match = util.RE_ENVRA.match(filters["envra"])
    if match is not None:
        for attr in ("name", "epoch", "version", "release", "arch"):
            match_attr = attr
            filters[attr] = match.group(match_attr)
        filters.pop('nevra', None)
        filters.pop('envra', None)
    elif "evra" in filters:
        for prop_name in ("epoch", "version", "release", "epoch"):
            filters.pop(prop_name, None)

    filter_list = []
    # properties are sorted by their filtering ability
    # (the most unprobable property, that can match, comes first)
    for prop_name in ("evra", "name", "version", "epoch",
            "release", "repoid", "arch"):
        if not prop_name in filters:
            continue
        if not exact_match and 'name' in filters:
            continue
        filter_list.append((prop_name, filters.pop(prop_name)))
    if not exact_match and 'name' in filters:
        re_name = re.compile(re.escape(filters['name']))
        def _cmp_props(pkg):
            """ :rtype: (``bool``) Does pkg matche properies filter? """
            if re_name.search(pkg.name) or re_name.search(pkg.summary):
                return all(getattr(pkg, p) == v for p, v in filter_list)
            return False
    else:
        def _cmp_props(pkg):
            """ :rtype: (``bool``) Does pkg matche properies filter? """
            return all(getattr(pkg, p) == v for p, v in filter_list)
    return _cmp_props

class RepoFilterSetter(object):
    """
    A context manager, that will set a repository filter lasting
    as long as the object itself.
    """
    def  __init__(self, yum_base, include_repos=None, exclude_repos=None):
        if not isinstance(yum_base, yum.YumBase):
            raise TypeError("yum_base must be a YumBase instance")
        self._yum_base = yum_base
        self._include = include_repos
        self._exclude = exclude_repos
        # after __enter__ this will be dictionary containing (
        # repoid, enabled) pairs
        self._prev_states = None

    def __enter__(self):
        self._prev_states = {   r.id: r.enabled
                            for r in self._yum_base.repos.repos.values()}
        if isinstance(self._exclude, (list, tuple, set)):
            exclude = ",".join(self._exclude)
        else:
            exclude = self._exclude
        # set of repositories, that were affected
        repos = set()
        if exclude:
            repos.update(self._yum_base.repos.disableRepo(exclude))
            LOG.info('disabling repositories: [%s]', ", ".join(repos))
        if isinstance(self._include, (list, tuple, set)):
            include = ",".join(self._include)
        else:
            include = self._include
        if include:
            affected = self._yum_base.repos.enableRepo(include)
            LOG.info('enabling repositories: [%s]', ", ".join(affected))
            repos.update(affected)
        for repoid, prev_enabled in self._prev_states.items():
            if (  repoid not in repos
               or (  bool(prev_enabled)
                  is bool(self._yum_base.repos.getRepo(repoid).enabled))):
                # keep only manipulated repositories
                del self._prev_states[repoid]
        if len(self._prev_states):
            for repoid in (r for r, v in self._prev_states.items() if v):
                self._yum_base.pkgSack.sacks.pop(repoid, None)
            self._yum_base.repos.populateSack()
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        # restore previous repository states
        if len(self._prev_states):
            LOG.info('restoring repositories: [%s]',
                    ", ".join(self._prev_states.keys()))
            for repoid, enabled in self._prev_states.items():
                repo = self._yum_base.repos.getRepo(repoid)
                if enabled:
                    repo.enable()
                else:
                    repo.disable()
            for repoid in (r for r, v in self._prev_states.items() if not v):
                self._yum_base.pkgSack.sacks.pop(repoid, None)
            self._yum_base.repos.populateSack()

# *****************************************************************************
# Decorators
# *****************************************************************************
def _needs_database(method):
    """
    Decorator for YumWorker job handlers, that need to access the yum database.
    It ensures, that database is initialized and locks it in case, that
    no session is active.
    """
    logged = cmpi_logging.trace_method(method, frame_level=2)
    @wraps(method)
    def _wrapper(self, *args, **kwargs):
        """
        Wrapper for the job handler method.
        """
        created_session = False
        self._init_database()               #pylint: disable=W0212
        if self._session_level == 0:        #pylint: disable=W0212
            self._session_level = 1         #pylint: disable=W0212
            created_session = True
            self._lock_database()           #pylint: disable=W0212
        try:
            result = logged(self, *args, **kwargs)
            return result
        finally:
            if created_session is True:     #pylint: disable=W0212
                self._session_level = 0     #pylint: disable=W0212
                self._unlock_database()     #pylint: disable=W0212
    return _wrapper

# *****************************************************************************
# Classes
# *****************************************************************************
class YumWorker(Process):
    """
    The main process, that works with YUM API. It has two queues, one
    for input jobs and second for results.

    Jobs are dispatched by their class names to particular handler method.

    It spawns a second thread for managing asynchronous jobs and queue
    of incoming jobs. It's an instance of JobManager.
    """

    def __init__(self,
            queue_in,
            queue_out,
            indication_manager,
            yum_kwargs=None):
        Process.__init__(self, name="YumWorker")
        self._jobmgr = JobManager(queue_in, queue_out, indication_manager)
        self._session_level = 0
        self._session_ended = False

        if yum_kwargs is None:
            yum_kwargs = {}

        self._yum_kwargs = yum_kwargs
        self._yum_base = None

        self._pkg_cache = None
        # contains (repoid, time_stamp_of_config_file)
        # plus (/repos/dir, ...) for each repo config directory
        self._repodir_mtimes = {}

    # *************************************************************************
    # Private methods
    # *************************************************************************
    @cmpi_logging.trace_method
    def _init_database(self):
        """
        Initializes yum base object, when it does no exists.
        And updates the cache (when out of date).
        """
        if self._yum_base is None:
            LOG.info("creating YumBase with kwargs=(%s)",
                ", ".join((   "%s=%s"%(k, str(v))
                          for k, v in self._yum_kwargs.items())))
            self._yum_base = yum.YumBase(**self._yum_kwargs)

    @cmpi_logging.trace_method
    def _free_database(self):
        """
        Release the yum base object to safe memory.
        """
        LOG.info("freing database")
        self._pkg_cache.clear()
        self._yum_base = None

    @cmpi_logging.trace_method
    def _lock_database(self):
        """
        Only one process is allowed to work with package database at given time.
        That's why we lock it.

        Try to lock it in loop, until success.
        """
        while True:
            try:
                LOG.info("trying to lock database - session level %d",
                        self._session_level)
                self._yum_base.doLock()
                LOG.info("successfully locked up")
                break
            except yum.Errors.LockError as exc:
                LOG.warn("failed to lock")
                if exc.errno in (errno.EPERM, errno.EACCES, errno.ENOSPC):
                    LOG.error("can't create lock file")
                    raise errors.DatabaseLockError("Can't create lock file.")
            timeout = util.Configuration.get_instance().get_safe(
                    'Yum', 'LockWaitInterval', float)
            LOG.info("trying to lock again after %.1f seconds", timeout)
            time.sleep(timeout)

    @cmpi_logging.trace_method
    def _unlock_database(self):
        """
        The opposite to _lock_database() method.
        """
        if self._yum_base is not None:
            LOG.info("unlocking database")
            self._yum_base.closeRpmDB()
            self._yum_base.doUnlock()

    @cmpi_logging.trace_method
    def _get_job(self):
        """
        Get job from JobManager thread.
        If no job comes for long time, free database to save memory.
        """
        while True:
            if self._session_ended and self._session_level == 0:
                try:
                    timeout = util.Configuration.get_instance().get_safe(
                            'Yum', 'FreeDatabaseTimeout', float)
                    return self._jobmgr.get_job(timeout=timeout)
                except TQueue.Empty:
                    self._free_database()
                    self._session_ended = False
            else:
                return self._jobmgr.get_job()

    @cmpi_logging.trace_method
    def _transform_packages(self, packages,
            cache_packages=True,
            flush_cache=True):
        """
        Return instances of PackageInfo for each package in packages.
        Cache all the packages.
        @param packages list of YumAvailablePackage instances
        @param cache_packages whether to update cache with packages
        @param flush_cache whether to clear the cache before adding input
        packages; makes sense only with cachee_packages=True
        """
        if cache_packages is True and flush_cache is True:
            LOG.debug("flushing package cache")
            self._pkg_cache.clear()
        res = []
        for orig in packages:
            pkg = packageinfo.make_package_from_db(orig)
            if cache_packages is True:
                self._pkg_cache[pkg.objid] = orig
            res.append(pkg)
        return res

    @cmpi_logging.trace_method
    def _cache_packages(self, packages, flush_cache=True, transform=False):
        """
        Store packages in cache and return them.
        @param flush_cache whether to clear the cache before adding new
        packages
        @param transform whether to return packages as PackageInfos
        @return either list of original packages or PackageInfo instances
        """
        if transform is True:
            return self._transform_packages(packages, flush_cache=flush_cache)
        if flush_cache is True:
            LOG.debug("flushing package cache")
            self._pkg_cache.clear()
        for pkg in packages:
            self._pkg_cache[id(pkg)] = pkg
        return packages

    @cmpi_logging.trace_method
    def _lookup_package(self, pkg):
        """
        Lookup the original package in cache.
        If it was garbage collected already, make new query to find it.
        @return instance of YumAvailablePackage
        """
        if not isinstance(pkg, packageinfo.PackageInfo):
            raise TypeError("pkg must be instance of PackageInfo")
        LOG.debug("looking up yum package %s with id=%d",
            pkg, pkg.objid)
        try:
            result = self._pkg_cache[pkg.objid]
            LOG.debug("lookup successful")
        except KeyError:
            LOG.warn("lookup of package %s with id=%d failed, trying"
                " to query database", pkg, pkg.objid)
            result = self._handle_filter_packages(
                    'installed' if pkg.installed else 'available',
                    allow_duplicates=False,
                    sort=False,
                    transform=False,
                    **pkg.key_props)
            if len(result) < 1:
                LOG.warn("package %s not found", pkg)
                raise errors.PackageNotFound(
                        "package %s could not be found" % pkg)
            result = result[0]
        return result

    @cmpi_logging.trace_method
    def _clear_repository_cache(self):
        """
        Clears the repository cache and their configuration directory
        last modification times.
        """
        if self._yum_base is not None:
            for repoid in self._yum_base.repos.repos.keys():
                self._yum_base.repos.delete(repoid)
            del self._yum_base.repos
            del self._yum_base.pkgSack
        self._repodir_mtimes.clear()

    @cmpi_logging.trace_method
    def _check_repository_configs(self):
        """
        Checks whether repository information is up to date with configuration
        files by comparing timestamps. If not, repository cache will be
        released.
        """
        dirty = False
        if self._repodir_mtimes:
            for repodir in self._yum_base.conf.reposdir:
                if (  os.path.exists(repodir)
                   and (   not repodir in self._repodir_mtimes
                       or  ( os.stat(repodir).st_mtime
                           > self._repodir_mtimes[repodir]))):
                    LOG.info("repository config dir %s changed", repodir)
                    dirty = True
                    break
            if not dirty:
                for repo in self._yum_base.repos.repos.values():
                    filename = repo.repofile
                    if (  not os.path.exists(filename)
                       or (  int(os.stat(filename).st_mtime)
                          > repo.repo_config_age)):
                        LOG.info('config file of repository "%s" changed',
                                repo.id)
                        dirty = True
                        break
        if dirty is True:
            LOG.info("repository cache is dirty, cleaning up ...")
            self._clear_repository_cache()
            self._yum_base.getReposFromConfig()
        if dirty is True or not self._repodir_mtimes:
            self._update_repodir_mtimes()

    @cmpi_logging.trace_method
    def _update_repodir_mtimes(self):
        """
        Updates the last modification times of repo configuration directories.
        """
        assert self._yum_base is not None
        for repodir in self._yum_base.conf.reposdir:
            if os.path.exists(repodir):
                self._repodir_mtimes[repodir] = os.stat(repodir).st_mtime

    @cmpi_logging.trace_method
    def _do_work(self, job):
        """
        Dispatcher of incoming jobs. Job is passed to the right handler
        depending on its class.
        """
        if not isinstance(job, jobs.YumJob):
            raise TypeError("job must be instance of YumJob")
        try:
            handler = {
                jobs.YumGetPackageList   : self._handle_get_package_list,
                jobs.YumFilterPackages   : self._handle_filter_packages,
                jobs.YumInstallPackage   : self._handle_install_package,
                jobs.YumRemovePackage    : self._handle_remove_package,
                jobs.YumUpdateToPackage  : self._handle_update_to_package,
                jobs.YumUpdatePackage    : self._handle_update_package,
                jobs.YumBeginSession     : self._handle_begin_session,
                jobs.YumEndSession       : self._handle_end_session,
                jobs.YumCheckPackage     : self._handle_check_package,
                jobs.YumCheckPackageFile : self._handle_check_package_file,
                jobs.YumInstallPackageFromURI : \
                        self._handle_install_package_from_uri,
                jobs.YumGetRepositoryList : \
                        self._handle_get_repository_list,
                jobs.YumFilterRepositories : self._handle_filter_repositories,
                jobs.YumSetRepositoryEnabled : \
                        self._handle_set_repository_enabled
            }[job.__class__]
            LOG.info("processing job %s(id=%d)",
                job.__class__.__name__, job.jobid)
        except KeyError:
            LOG.error("No handler for job \"%s\"", job.__class__.__name__)
            raise errors.UnknownJob("No handler for job \"%s\"." %
                    job.__class__.__name__)
        return handler(**job.job_kwargs)

    @cmpi_logging.trace_method
    def _run_transaction(self, name):
        """
        Builds and runs the yum transaction and checks for errors.
        @param name of transaction used only in error description on failure
        """
        LOG.info("building transaction %s", name)
        (code, msgs) = self._yum_base.buildTransaction()
        if code == 1:
            LOG.error("building transaction %s failed: %s",
                name, "\n".join(msgs))
            raise errors.TransactionBuildFailed(
                "Failed to build \"%s\" transaction: %s" % (
                    name, "\n".join(msgs)))
        LOG.info("processing transaction %s", name)
        self._yum_base.processTransaction()
        self._yum_base.closeRpmDB()

    @cmpi_logging.trace_method
    def _main_loop(self):
        """
        This is a main loop called from run(). Jobs are handled here.
        It accepts a job from input queue, handles it,
        sends the result to output queue and marks the job as done.

        It is terminated, when None is received from input queue.
        """
        while True:
            job = self._get_job()
            if job is not None:             # not a terminate command
                result = jobs.YumJob.RESULT_SUCCESS
                try:
                    data = self._do_work(job)
                except Exception:    #pylint: disable=W0703
                    result = jobs.YumJob.RESULT_ERROR
                    # (type, value, traceback)
                    data = sys.exc_info()
                    # traceback is not pickable - replace it with formatted
                    # text
                    data = (data[0], data[1], traceback.format_tb(data[2]))
                    LOG.exception("job %s failed", job)
                self._jobmgr.finish_job(job, result, data)
            if job is None:
                LOG.info("waiting for %s to finish", self._jobmgr.name)
                self._jobmgr.join()
                break

    @cmpi_logging.trace_method
    def _handle_begin_session(self):
        """
        Handler for session begin job.
        """
        self._session_level += 1
        LOG.info("beginning session level %s", self._session_level)
        if self._session_level == 1:
            self._init_database()
            self._lock_database()

    @cmpi_logging.trace_method
    def _handle_end_session(self):
        """
        Handler for session end job.
        """
        LOG.info("ending session level %d", self._session_level)
        self._session_level = max(self._session_level - 1, 0)
        if self._session_level == 0:
            self._unlock_database()
        self._session_ended = True

    @_needs_database
    def _handle_get_package_list(self, kind, allow_duplicates, sort,
            include_repos=None, exclude_repos=None, transform=True):
        """
        Handler for listing packages job.
        @param transform says, whether to return just a package abstractions
        or original ones
        @return [pkg1, pkg2, ...]
        """
        if kind == 'avail_notinst':
            what = 'available'
        elif kind == 'available':
            what = 'all'
        elif kind == 'avail_reinst':
            what = 'all'
        else:
            what = kind
        with RepoFilterSetter(self._yum_base, include_repos, exclude_repos):
            LOG.debug("calling YumBase.doPackageLists(%s, showdups=%s)",
                    what, allow_duplicates)
            pkglist = self._yum_base.doPackageLists(what,
                    showdups=allow_duplicates)
        if kind == 'all':
            result = pkglist.available + pkglist.installed
        elif kind == 'available':
            result = pkglist.available + pkglist.reinstall_available
        elif kind == 'avail_reinst':
            result = pkglist.reinstall_available
        else:   # get installed or available
            result = getattr(pkglist, what)
        if sort is True:
            result.sort()
        LOG.debug("returning %s packages", len(result))
        return self._cache_packages(result, transform=transform)

    @_needs_database
    def _handle_filter_packages(self, kind, allow_duplicates, sort,
            exact_match=True, include_repos=None, exclude_repos=None,
            transform=True, **filters):
        """
        Handler for filtering packages job.
        @return [pkg1, pkg2, ...]
        """
        if kind == 'all' \
               and filters.get('repoid', None) \
               and not include_repos and not exclude_repos:
            # If repoid is given, we are interested only in available
            # packages, not installed. With kind == 'all' we are not able to
            # check repoid of installed package (which may be available).
            kind = 'available'
            exclude_repos = '*'
            include_repos = filters.pop('repoid')
        pkglist = self._handle_get_package_list(kind, allow_duplicates, False,
                include_repos=include_repos, exclude_repos=exclude_repos,
                transform=False)
        matches = _get_package_filter_function(filters, exact_match)
        result = [p for p in pkglist if matches(p)]
        if sort is True:
            result.sort()
        LOG.debug("%d packages matching", len(result))
        if transform is True:
            # caching has been already done by _handle_get_package_list()
            result = self._transform_packages(result, cache_packages=False)
        return result

    @_needs_database
    def _handle_install_package(self, pkg, force=False):
        """
        Handler for package installation job.
        @return installed package instance
        """
        if isinstance(pkg, basestring):
            pkgs = self._handle_filter_packages(
                    'available' if force else 'avail_notinst',
                    allow_duplicates=False, sort=True,
                    transform=False, nevra=pkg)
            if len(pkgs) < 1:
                raise errors.PackageNotFound('No available package matches'
                        ' nevra "%s".' % pkg)
            elif len(pkgs) > 1:
                LOG.warn('multiple packages matches nevra "%s": [%s]',
                        pkg, ", ".join(p.nevra for p in pkgs))
            pkg_desired = pkgs[-1]
        else:
            pkg_desired = self._lookup_package(pkg)
        if isinstance(pkg_desired, yum.rpmsack.RPMInstalledPackage):
            if force is False:
                raise errors.PackageAlreadyInstalled(pkg)
            action = "reinstall"
        else:
            action = "install"
        getattr(self._yum_base, action)(pkg_desired)
        self._run_transaction(action)
        installed = self._handle_filter_packages("installed", False, False,
                nevra=util.pkg2nevra(pkg_desired, with_epoch="ALWAYS"))
        if len(installed) < 1:
            raise errors.TransactionExecutionFailed(
                    "Failed to install desired package %s." % pkg)
        return installed[0]

    @_needs_database
    def _handle_remove_package(self, pkg):
        """
        Handler for package removal job.
        """
        if isinstance(pkg, basestring):
            pkgs = self._handle_filter_packages('installed',
                    allow_duplicates=False, sort=False,
                    transform=False, nevra=pkg)
            if len(pkgs) < 1:
                raise errors.PackageNotFound('No available package matches'
                        ' nevra "%s".' % pkg)
            pkg = pkgs[-1]
        else:
            pkg = self._lookup_package(pkg)
        self._yum_base.remove(pkg)
        self._run_transaction("remove")

    @_needs_database
    def _handle_update_to_package(self, pkg):
        """
        Handler for specific package update job.
        @return package corresponding to pkg after update
        """
        if isinstance(pkg, basestring):
            pkgs = self._handle_filter_packages('available',
                    allow_duplicates=False, sort=False,
                    transform=False, nevra=pkg)
            if len(pkgs) < 1:
                raise errors.PackageNotFound('No available package matches'
                        ' nevra "%s".' % pkg)
            pkg_desired = pkgs[-1]
        else:
            pkg_desired = self._lookup_package(pkg)
        self._yum_base.update(update_to=True,
                name=pkg_desired.name,
                epoch=pkg_desired.epoch,
                version=pkg_desired.version,
                release=pkg_desired.release,
                arch=pkg_desired.arch)
        self._run_transaction("update")
        installed = self._handle_filter_packages("installed", False, False,
                **pkg.key_props)
        if len(installed) < 1:
            raise errors.TransactionExecutionFailed(
                    "Failed to update to desired package %s." % pkg)
        return installed[0]

    @_needs_database
    def _handle_update_package(self, pkg, to_epoch, to_version, to_release,
            force=False):
        """
        Handler for package update job.
        @return updated package instance
        """
        if isinstance(pkg, basestring):
            pkgs = self._handle_filter_packages('installed',
                    allow_duplicates=False, sort=False,
                    transform=False, nevra=pkg)
            if len(pkgs) < 1:
                raise errors.PackageNotFound('No available package matches'
                        ' nevra "%s".' % pkg)
            pkg = pkgs[-1]
        else:
            pkg = self._lookup_package(pkg)
        if not isinstance(pkg, yum.rpmsack.RPMInstalledPackage):
            raise errors.PackageNotInstalled(pkg)
        kwargs = { "name" : pkg.name, "arch" : pkg.arch }
        if any(v is not None for v in (to_epoch, to_version, to_release)):
            kwargs["update_to"] = True
        if to_epoch:
            kwargs["to_epoch"] = to_epoch
        if to_version:
            kwargs["to_version"] = to_version
        if to_release:
            kwargs["to_release"] = to_release
        self._yum_base.update(**kwargs)
        self._run_transaction("update")
        kwargs = dict(  (k[3:] if k.startswith("to_") else k, v)
                     for k, v in kwargs.items())
        installed = self._handle_filter_packages(
                "installed", False, False, **kwargs)
        if len(installed) < 1:
            raise errors.TransactionExecutionFailed(
                    "Failed to update package %s." % pkg)
        return installed[0]

    @_needs_database
    def _handle_check_package(self, pkg, file_name=None):
        """
        @return PackageCheck instance for requested package
        """
        if isinstance(pkg, basestring):
            pkgs = self._handle_filter_packages('installed',
                    allow_duplicates=False, sort=False,
                    transform=False, nevra=pkg)
            if len(pkgs) < 1:
                raise errors.PackageNotFound('No available package matches'
                        ' nevra "%s".' % pkg)
            rpm = pkgs[-1]
            pkg = self._transform_packages((rpm, ), cache_packages=False)[0]
        else:
            rpm = self._lookup_package(pkg)
        if not isinstance(rpm, yum.rpmsack.RPMInstalledPackage):
            raise errors.PackageNotInstalled(rpm)
        vpkg = yum.packages._RPMVerifyPackage(rpm, rpm.hdr.fiFromHeader(),
                packagecheck.pkg_checksum_type(rpm), [], True)
        return (pkg, packagecheck.make_package_check_from_db(vpkg,
                file_name=file_name))

    @_needs_database
    def _handle_check_package_file(self, pkg, file_name):
        """
        @return PackageCheck instance for requested package containing
            just one PackageFile instance for given ``file_name``.
        """
        return self._handle_check_package(pkg, file_name)

    @_needs_database
    def _handle_install_package_from_uri(self, uri,
            update_only=False, force=False):
        """
        @return installed PackageInfo instance
        """
        try:
            pkg = yum.packages.YumUrlPackage(self._yum_base,
                    ts=self._yum_base.rpmdb.readOnlyTS(), url=uri,
                    ua=default_grabber.opts.user_agent)
        except yum.Errors.MiscError as exc:
            raise errors.PackageOpenError(uri, str(exc))
        installed = self._handle_filter_packages("installed", False, False,
                nevra=util.pkg2nevra(pkg, with_epoch="ALWAYS"))
        if installed and force is False:
            raise errors.PackageAlreadyInstalled(pkg)
        kwargs = { 'po' : pkg }
        if installed:
            action = 'reinstallLocal'
        else:
            action = 'installLocal'
            kwargs = { 'updateonly' : update_only }
        getattr(self._yum_base, action)(uri, **kwargs)
        self._run_transaction('installLocal')
        installed = self._handle_filter_packages("installed", False, False,
                nevra=util.pkg2nevra(pkg, with_epoch="ALWAYS"))
        if len(installed) < 1:
            raise errors.TransactionExecutionFailed(
                    "Failed to install desired package %s." % pkg)
        return installed[0]

    @_needs_database
    def _handle_get_repository_list(self, kind, transform=True):
        """
        @return list of yumdb.Repository instances
        """
        self._check_repository_configs()
        if kind == 'enabled':
            repos = sorted(self._yum_base.repos.listEnabled())
        else:
            repos = self._yum_base.repos.repos.values()
            if kind == 'disabled':
                repos = [repo for repo in repos if not repo.enabled]
            repos.sort()
        if transform:
            repos = [repository.make_repository_from_db(r) for r in repos]
        LOG.debug("returning %d repositories from %s",
                len(repos), kind)
        return repos

    @_needs_database
    def _handle_filter_repositories(self, kind, **filters):
        """
        @return list of yumdb.Repository instances -- filtered
        """
        filters = dict((k, v) for k, v in filters.items() if v is not None)
        if 'repoid' in filters:
            self._check_repository_configs()
            try:
                repo = repository.make_repository_from_db(
                            self._yum_base.repos.getRepo(filters["repoid"]))
                if (  (kind == "enabled"  and not repo.enabled)
                   or (kind == "disabled" and repo.enabled)):
                    LOG.warn(
                            'no such repository with id="%s"matching filters',
                            filters['repoid'])
                    return []
                LOG.debug(
                        "exactly one repository matching filters found")
                return [repo]
            except (KeyError, yum.Errors.RepoError):
                LOG.warn('repository with id="%s" could not be found',
                        filters['repoid'])
                raise errors.RepositoryNotFound(filters['repoid'])

        repos = self._handle_get_repository_list(kind, transform=False)
        result = []
        for repo in repos:
            # do the filtering and safe transformed repo into result
            for prop, value in filters.items():
                if repository.get_prop_from_yum_repo(repo, prop) != value:
                    # did not pass the filter
                    break
            else: # all properties passed
                result.append(repository.make_repository_from_db(repo))
        LOG.debug("found %d repositories matching", len(result))
        return result

    @_needs_database
    def _handle_set_repository_enabled(self, repo, enable):
        """
        @return previous enabled state
        """
        self._check_repository_configs()
        if isinstance(repo, repository.Repository):
            repoid = repo.repoid
        else:
            repoid = repo
        try:
            repo = self._yum_base.repos.getRepo(repoid)
        except (KeyError, yum.Errors.RepoError):
            raise errors.RepositoryNotFound(repoid)
        res = repo.enabled
        try:
            if enable ^ res:
                if enable is True:
                    LOG.info("enabling repository %s" % repo)
                    repo.enable()
                else:
                    LOG.info("disabling repository %s" % repo)
                    repo.disable()
                try:
                    yum.config.writeRawRepoFile(repo, only=["enabled"])
                except Exception as exc:
                    raise errors.RepositoryChangeError(
                            'failed to modify repository "%s": %s - %s' % (
                                repo, exc.__class__.__name__, str(exc)))
            else:
                LOG.info("no change for repo %s", repo)
        except yum.Errors.RepoError as exc:
            raise errors.RepositoryChangeError(
                    'failed to modify repository "%s": %s' % (repo, str(exc)))
        return res

    # *************************************************************************
    # Public properties
    # *************************************************************************
    @property
    def uplink(self):
        """
        @return input queue for jobs
        """
        return self._jobmgr.queue_in

    @property
    def downlink(self):
        """
        @return output queue for job results
        """
        return self._jobmgr.queue_out

    # *************************************************************************
    # Public methods
    # *************************************************************************
    def run(self):
        """
        Process's entry point. After initial setup it calls _main_loop().
        """
        setup_logging()
        global LOG
        LOG = logging.getLogger(__name__)
        LOG.info("running as pid=%d", self.pid)
        self._jobmgr.start()
        LOG.info("started %s as thread %s",
                self._jobmgr.name, self._jobmgr.ident)
        self._pkg_cache = weakref.WeakValueDictionary()

        # This allows the code, that can be run both from broker and
        # YumWorker, to check, whether it's called by this process.
        from lmi.software.yumdb import YumDB
        YumDB.RUNNING_UNDER_CIMOM_PROCESS = False

        self._main_loop()
        LOG.info("terminating")

