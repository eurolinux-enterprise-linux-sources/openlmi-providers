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
Since yum API functions should not be called with different thread_ids
repeatedly in the same program. It's neccessary, to make these calls
in single thread. But the provider needs to be able to clean up itself,
when its not needed. That's why the yum API needs to be accessed from
separated process, that is created and terminated when needed.

This package contains all the bowels of this separate process together
with its management and communication facilities.

YumDB is a context manager supposed to be used by any provider as the
only accessor to yum api.
"""

from functools import wraps
import inspect
import threading

from lmi.base import singletonmixin
from lmi.providers import cmpi_logging
from lmi.software.yumdb import jobs
from lmi.software.yumdb.jobmanager import JobManager
from lmi.software.yumdb.packagecheck import PackageCheck
from lmi.software.yumdb.packagecheck import PackageFile
from lmi.software.yumdb.packageinfo import PackageInfo
from lmi.software.yumdb.repository import Repository

LOG = cmpi_logging.get_logger(__name__)

# *****************************************************************************
# Utilities
# *****************************************************************************
def _make_async_job(jobcls, *args, **kwargs):
    """Creates asynchronous job, filling it wih some metadata."""
    if not issubclass(jobcls, jobs.YumAsyncJob):
        raise TypeError("jobcls must be a subclass of YumAsyncJob")
    job = jobcls(*args, **kwargs)
    if job.metadata is None:
        job.metadata = {}
    job.metadata['name'] = \
            type(job).__name__[len('Yum'):] + ('-%d' % job.jobid)
    return job

# *****************************************************************************
# Decorators
# *****************************************************************************
def job_request(async=False):
    """
    Decorator factory for job entry points. They are YumDB methods.
    All of them must return either job objects or jobid for asynchronous calls.
    Job objects are processed by this decorator for caller to obtain only the
    information he needs.

    It wrapps them with logger wrapper and in case of asynchronous jobs,
    it returns just the jobid.
    """
    def _decorator(method):
        """
        Decorator that just logs the method's call and returns job's result.
        """
        logged = cmpi_logging.trace_method(method, 2)
        @wraps(method)
        def _new_func(self, *args, **kwargs):
            """Wrapper for YumDB's method."""
            return logged(self, *args, **kwargs).result_data
        return _new_func

    def _decorator_async(method):
        """
        Decorator for methods accepting async argument. In case of async=True,
        the method returns jobid. Job's result is returned otherwise.
        """
        logged = cmpi_logging.trace_method(method, 2)
        @wraps(method)
        def _new_func(self, *args, **kwargs):
            """Wrapper for YumDB's method."""
            callargs = inspect.getcallargs(method, self, *args, **kwargs)
            result = logged(self, *args, **kwargs)
            if callargs.get('async', False):
                return result
            else:
                return result.result_data
        return _new_func

    return _decorator_async if async else _decorator

class YumDB(singletonmixin.Singleton):
    """
    Context manager for accessing yum/rpm database.
    All requests are bundled into jobs -- instances of jobs.YumJob and
    sent to YumWorker for processing.

    YumWorker is a separate process handling all calls to yum api.
    Communication is done via queues (uplink and downlink).
    Uplink is used to send jobs to YumWorker and downlink for obtaining
    results.

    This is implemented in thread safe manner.

    It should be used as a context manager in case, we want to process
    multiple jobs in single transaction. The example of usage:
        with YumDB.getInstance() as ydb:
            pkgs = ydb.filter_packages(...)
            for pkg in pkgs:
                ydb.install_package(pkg)
            ...
    Yum database stays locked in whole block of code under with statement.
    """

    # this is to inform Singleton, that __init__ should be called only once
    ignoreSubsequent = True

    @cmpi_logging.trace_method
    def __init__(self):   #pylint: disable=W0231
        """
        All arguments are passed to yum.YumBase constructor.
        """
        self._access_lock = threading.RLock()
        self._jobmgr = None
        LOG().trace_info('YumDB initialized')

    # *************************************************************************
    # Private methods
    # *************************************************************************
    @property
    def jobmgr(self):
        with self._access_lock:
            if self._jobmgr is None:
                self._jobmgr = JobManager()
                self._jobmgr.start()
            return self._jobmgr

    def _do_job(self, job):
        """
        Sends the job to YumWorker process and waits for reply.
        If reply is a tuple, there was an error, while job processing.
        Incoming exception is in format:
          (exception_type, exception_value, formated_traceback_as_string)
        @return reply
        """
        LOG().trace_verbose("doing %s", job)
        reply = self.jobmgr.process(job)
        jobs.log_reply_error(job, reply)
        LOG().trace_verbose("job %s done", job.jobid)
        return reply

    # *************************************************************************
    # Special methods
    # *************************************************************************
    @cmpi_logging.trace_method
    def __enter__(self):
        """ Start a new session. """
        with self._access_lock:
            self.jobmgr.begin_session()
        return self

    @cmpi_logging.trace_method
    def __exit__(self, exc_type, exc_value, traceback):
        """ End active session. """
        with self._access_lock:
            self.jobmgr.end_session()

    # *************************************************************************
    # Public methods
    # *************************************************************************
    @cmpi_logging.trace_method
    def clean_up(self):
        """
        Shut down job manager and all the other threads and processes it has
        created.
        """
        with self._access_lock:
            if self._jobmgr is not None:
                self._do_job(jobs.YumShutDown())
                self._jobmgr.join()
                self._jobmgr = None
            else:
                LOG().warn("clean_up called with JobManager not initialized")

    # *************************************************************************
    # Jobs with simple results
    # *************************************************************************
    @job_request()
    def get_package_list(self, kind,
            allow_duplicates=False,
            sort=False,
            include_repos=None,
            exclude_repos=None):
        """
        @param kind is one of: jobs.YumGetPackageList.SUPPORTED_KINDS
        @param allow_duplicates says, whether to list all found versions
        of single package
        @return [pkg1, pkg2, ...], pkgi is instance of yumdb.PackageInfo
        """
        return self._do_job(jobs.YumGetPackageList(
            kind, allow_duplicates=allow_duplicates, sort=sort,
            include_repos=include_repos, exclude_repos=exclude_repos))

    @job_request()
    def filter_packages(self, kind,
            allow_duplicates=False,
            exact_match=True,
            sort=False,
            include_repos=None,
            exclude_repos=None,
            **filters):
        """
        Similar to get_package_list(), but applies filter on packages.
        @see yumdb.jobs.YumFilterPackages job for supported filter keys
        """
        return self._do_job(jobs.YumFilterPackages(
            kind, allow_duplicates=allow_duplicates, exact_match=exact_match,
            sort=sort, include_repos=include_repos, exclude_repos=exclude_repos,
            **filters))

    @job_request()
    def get_repository_list(self, kind):
        """
        @param kind is one of: jobs.YumGetRepositoryList.SUPPORTED_KINDS
        @param allow_duplicates says, whether to list all found versions
        of single package
        @return [pkg1, pkg2, ...], pkgi is instance of yumdb.Repository
        """
        return self._do_job(jobs.YumGetRepositoryList(kind))

    @job_request()
    def filter_repositories(self, kind, **filters):
        """
        Similar to get_repository_list(), but applies filter on packages.
        @see yumdb.jobs.YumFilterRepositories job for supported filter keys
        """
        return self._do_job(jobs.YumFilterRepositories(kind, **filters))

    @job_request()
    def set_repository_enabled(self, repoid, enable):
        """
        Enable or disable repository.
        @param enable is a boolean
        """
        return self._do_job(jobs.YumSetRepositoryEnabled(repoid, enable))

    # *************************************************************************
    # Asynchronous jobs
    # *************************************************************************
    @job_request(async=True)
    def install_package(self, pkg, async=False, force=False, **metadata):
        """
        Install package.
        @param pkg is an instance of PackageInfo obtained with
        get_package_list() or filter_packages() or a valid nevra as string.
        Package must not be installed if force is False.
        """
        return self._do_job(_make_async_job(jobs.YumInstallPackage,
            pkg, force=force, async=async, metadata=metadata))

    @job_request(async=True)
    def remove_package(self, pkg, async=False, **metadata):
        """
        @param pkg is an instance of PackageInfo obtained with
        get_package_list() or filter_packages(), which must be installed
        """
        return self._do_job(_make_async_job(jobs.YumRemovePackage,
            pkg, async=async, metadata=metadata))

    @job_request(async=True)
    def update_to_package(self, desired_pkg, async=False, **metadata):
        """
        @param desired_pkg is an instance of PackageInfo,
        which must be available
        """
        return self._do_job(_make_async_job(jobs.YumUpdateToPackage,
            desired_pkg, async=async, metadata=metadata))

    @job_request(async=True)
    def update_package(self, pkg,
            async=False,
            to_epoch=None,
            to_version=None,
            to_release=None,
            force=False,
            **metadata):
        """
        @param pkg is an instance of PackageInfo, which must be installed

        The other parameters filter candidate available packages for update.
        """
        return self._do_job(_make_async_job(jobs.YumUpdatePackage,
            pkg, async, to_epoch, to_version, to_release, force=force,
            metadata=metadata))

    @job_request(async=True)
    def check_package(self, pkg, async=False, **metadata):
        """
        Return all necessary information from package database for package
        verification.

        :param pkg: (``PackageInfo``) An instance of PackageInfo
            representing installed package or its nevra string.
        :rtype: (``PackageCheck``)
        """
        return self._do_job(_make_async_job(jobs.YumCheckPackage,
            pkg, async=async, metadata=metadata))

    @job_request(async=True)
    def check_package_file(self, pkg, file_name, async=False):
        """
        Return all necessary information from package database concerning
        on particular file of package. If ``pkg`` does not contain
        ``file_name``, ``FileNotFound`` error is raised.

        :param pkg: (``PackageInfo``) An instance of PackageInfo
            representing installed package or its nevra string.
        :rtype: (``PackageFile``)
        """
        return self._do_job(_make_async_job(jobs.YumCheckPackageFile,
            pkg, file_name, async=async))

    @job_request(async=True)
    def install_package_from_uri(self, uri,
            async=False, update_only=False, force=False, **metadata):
        """
        Install package from uri.
        @param uri is either remote url or local path.
        """
        return self._do_job(_make_async_job(jobs.YumInstallPackageFromURI,
            uri, async, update_only, force=force, metadata=metadata))

    # *************************************************************************
    # Control of asynchronous jobs
    # *************************************************************************
    @job_request()
    def get_job(self, jobid):
        """
        Return instance of ``YumJob`` with given ``jobid``.
        """
        return self._do_job(jobs.YumJobGet(jobid))

    @job_request()
    def get_job_list(self):
        """
        Return list of all asynchronous jobs.
        """
        return self._do_job(jobs.YumJobGetList())

    @job_request()
    def get_job_by_name(self, name):
        """
        Return asynchronous job filtered by its name.
        """
        return self._do_job(jobs.YumJobGetByName(name))

    @job_request()
    def set_job_priority(self, jobid, priority):
        """
        Change priority of asynchronous job. This will change
        its order in queue, if it is still enqeueued.

        Return object of job.
        """
        return self._do_job(jobs.YumJobSetPriority(jobid, priority))

    @job_request()
    def update_job(self, jobid, **kwargs):
        """
        Update metadata of job.

        :param kwargs: (``dict``) Is a dictionary of job's property names
            with mapped new values. Only keys given will be changed in
            desired job.

            **Note** that only keys, that do not affect job's priority or its
            scheduling for deletion can be changed. See :ref:`YumJobUpdate`.
        """
        return self._do_job(jobs.YumJobUpdate(jobid, **kwargs))

    @job_request()
    def reschedule_job(self, jobid,
            delete_on_completion, time_before_removal):
        """
        Change the scheduling of job for deletion.

        :param delete_on_completion: (``bool``) Says, whether the job will
            be scheduled for deletion at ``finished + time_before_removal``
            time.
        :param time_before_removal: (``int``) Number of seconds, after the job
            is finished, it will be kept alive.
        """
        return self._do_job(jobs.YumJobReschedule(jobid,
            delete_on_completion, time_before_removal))

    @job_request()
    def delete_job(self, jobid):
        """
        Delete job object. This can be called only on finished job.
        """
        return self._do_job(jobs.YumJobDelete(jobid))

    @job_request()
    def terminate_job(self, jobid):
        """
        Terminate job. This can be called only on *NEW* job.
        """
        return self._do_job(jobs.YumJobTerminate(jobid))

