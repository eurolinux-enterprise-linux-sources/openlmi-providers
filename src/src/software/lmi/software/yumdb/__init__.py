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

import errno
from functools import wraps
import inspect
import os
import re
import time
from multiprocessing import Process, Queue #pylint: disable=W0404
from pywbem.cim_provider2 import CIMProvider2
import Queue as TQueue   # T as threaded
import threading
import yum

from lmi.base import singletonmixin
from lmi.providers import cmpi_logging
from lmi.providers.IndicationManager import IndicationManager
from lmi.software.util import Configuration
from lmi.software.util import get_signal_name
from lmi.software.yumdb import jobs
from lmi.software.yumdb.packagecheck import PackageCheck
from lmi.software.yumdb.packagecheck import PackageFile
from lmi.software.yumdb.packageinfo import PackageInfo
from lmi.software.yumdb.process import YumWorker
from lmi.software.yumdb.repository import Repository

LOG = cmpi_logging.get_logger(__name__)

# *****************************************************************************
# Utilities
# *****************************************************************************
def log_reply_error(job, reply):
    """
    Raises an exception in case of error occured in worker process
    while processing job.
    """
    if isinstance(reply, (int, long)):
        # asynchronous job
        return
    if not isinstance(reply, jobs.YumJob):
        raise TypeError('expected instance of jobs.YumJob for reply, not "%s"'
                % reply.__class__.__name__)
    if reply.result == jobs.YumJob.RESULT_ERROR:
        LOG().error("%s failed with error %s: %s",
                job, reply.result_data[0].__name__, str(reply.result_data[1]))
        LOG().trace_warn("%s exception traceback:\n%s%s: %s",
                job, "".join(reply.result_data[2]),
                reply.result_data[0].__name__, str(reply.result_data[1]))
        reply.result_data[1].tb_printed = True
        raise reply.result_data[1]
    elif reply.result == jobs.YumJob.RESULT_TERMINATED:
        LOG().warn('%s terminated', job)
    else:
        LOG().debug('%s completed with success', job)

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

    # This serves to all code base as a global variable used to check,
    # whether YumDB instance is running under cimom broker or under worker
    # process. This is important for code used in callback functions passed
    # to worker responsible for creating instances of ConcreteJob. This code
    # must avoid using calls to YumDB while running under worker. This
    #
    # Worker process must set this to False before starting its event handling
    # loop.
    RUNNING_UNDER_CIMOM_PROCESS = True

    @cmpi_logging.trace_method
    def __init__(self, **kwargs):   #pylint: disable=W0231
        """
        All arguments are passed to yum.YumBase constructor.
        """
        self._process = None
        if kwargs is None:
            kwargs = {}
        self._yum_kwargs = kwargs

        self._session_lock = threading.RLock()
        self._session_level = 0

        # used to guard access to _expected list and _process
        self._reply_lock = threading.Lock()
        # used to wait for job to be processed and received
        self._reply_cond = threading.Condition(self._reply_lock)
        # ids of all expected jobs -- those to be processed by YumWorker
        self._expected = []
        # {job_id : reply, ... }
        self._replies = {}
        LOG().trace_info('YumDB initialized')

    # *************************************************************************
    # Private methods
    # *************************************************************************
    @cmpi_logging.trace_method
    def _handle_reply_timeout(self, job):
        """
        This is called when timeout occurs while waiting on downlink queue for
        reply. Delay can be caused by worker process's early termination (bug).
        This handler tries to recover from such an situation.
        """
        if not self._worker.is_alive():
            if self._worker.exitcode < 0:
                LOG().error("[jobid=%d] worker"
                    " process(pid=%d) killed by signal %s", job.jobid,
                    self._worker.pid, get_signal_name(-self._process.exitcode))
            else:
                LOG().error("[jobid=%d] worker"
                    " process(pid=%d) is dead - exit code: %d",
                    job.jobid, self._process.pid, self._worker.exitcode)
            with self._reply_lock:
                self._process = None
                LOG().error("[jobid=%d] starting new worker process", job.jobid)
                self._expected = []
                if not isinstance(job, jobs.YumBeginSession):
                    with self._session_lock:
                        if self._session_level > 0:
                            LOG().info('restoring session level=%d',
                                    self._session_level)
                            new_session_job = jobs.YumBeginSession()
                            self._worker.uplink.put(new_session_job)
                            reply = self._worker.downlink.get()
                            log_reply_error(new_session_job, reply)
                self._worker.uplink.put(job)
                self._expected.append(job.jobid)
                # other waiting processes need to resend their requests
                self._reply_cond.notifyAll()
        else:
            LOG().info("[jobid=%d] process is running, waiting some more",
                    job.jobid)

    @cmpi_logging.trace_method
    def _receive_reply(self, job):
        """
        Block on downlink queue to receive expected replies from worker
        process. Only one thread can be executing this code at any time.

        Only one thread can block on downlink channel to obtain reply. If
        it's reply for him, he takes it and leaves, otherwise he adds it to
        _replies dictionary and notifies other threads. This thread is the
        one, whose job appears as first in _expected list.

        In case, that worker process terminated due to some error. Restart it
        and resend all the job requests again.
        """
        timeout = Configuration.get_instance().get_safe(
                'Jobs', 'WaitCompleteTimeout', float)
        while True:
            LOG().debug("[jobid=%d] blocking on downlink queue", job.jobid)
            try:
                jobout = self._worker.downlink.get(block=True, timeout=timeout)
                if jobout.jobid == job.jobid:
                    LOG().debug("[jobid=%d] received desired reply", job.jobid)
                    with self._reply_lock:
                        self._expected.remove(job.jobid)
                        self._reply_cond.notifyAll()
                        return jobout
                else:
                    LOG().info("[jobid=%d] received reply for another thread"
                            " (jobid=%d)", job.jobid, jobout.jobid)
                    with self._reply_lock:
                        self._replies[jobout.jobid] = jobout
                        self._reply_cond.notifyAll()
            except TQueue.Empty:
                LOG().warn("[jobid=%d] wait for job reply timeout"
                        "(%d seconds) occured", job.jobid, timeout)
                self._handle_reply_timeout(job)

    @cmpi_logging.trace_method
    def _send_and_receive(self, job):
        """
        Sends a request to server and blocks until job is processed by
        YumWorker and reply is received.

        Only one thread can block on downlink channel to obtain reply. This
        thread is the one, whose job appears as first in _expected list. Server
        processes input jobs sequentially. That's why it's safe to presume,
        that jobs are received in the same order as they were send. Thanks to
        that we don't have to care about receiving replies for the other
        waiting threads.

        @return result of job
        """
        with self._reply_lock:
            self._worker.uplink.put(job)
            if getattr(job, 'async', False) is True:
                return job.jobid
            self._expected.append(job.jobid)
            while True:
                if job.jobid in self._replies:
                    LOG().debug("[jobid=%d] desired reply already received",
                            job.jobid)
                    try:
                        self._expected.remove(job.jobid)
                    except ValueError:
                        LOG().warn("[jobid=%d] reply not in expected list",
                                job.jobid)
                    self._reply_cond.notifyAll()
                    return self._replies.pop(job.jobid)
                elif job.jobid not in self._expected:
                    # process terminated, resending job
                    LOG().warn("[jobid=%d] job removed from expected list,"
                            " sending request again", job.jobid)
                    self._worker.uplink.put(job)
                    self._expected.append(job.jobid)
                elif job.jobid == self._expected[0]:
                    # now it's our turn to block on downlink
                    break
                else:   # another thread blocks on downlink -> let's sleep
                    LOG().debug("[jobid=%d] another %d threads expecting reply,"
                        " suspending...", job.jobid, len(self._expected) - 1)
                    self._reply_cond.wait()
                    LOG().debug("[jobid=%d] received reply, waking up",
                            job.jobid)
        return self._receive_reply(job)

    def _do_job(self, job):
        """
        Sends the job to YumWorker process and waits for reply.
        If reply is a tuple, there was an error, while job processing.
        Incoming exception is in format:
          (exception_type, exception_value, formated_traceback_as_string)
        @return reply
        """
        LOG().trace_verbose("doing %s", job)
        reply = self._send_and_receive(job)
        log_reply_error(job, reply)
        LOG().trace_verbose("job %s done", job.jobid)
        return reply

    @property
    def _worker(self):
        """
        YumWorker process accessor. It's created upon first need.
        """
        if self._process is None:
            LOG().trace_info("starting YumWorker")
            uplink = Queue()
            downlink = Queue()
            self._process = YumWorker(uplink, downlink,
                    indication_manager=IndicationManager.get_instance(),
                    yum_kwargs=self._yum_kwargs)
            self._process.start()
            LOG().trace_info("YumWorker started with pid=%s", self._process.pid)
        return self._process

    # *************************************************************************
    # Special methods
    # *************************************************************************
    @cmpi_logging.trace_method
    def __enter__(self):
        with self._session_lock:
            if self._session_level == 0:
                self._do_job(jobs.YumBeginSession())
                LOG().trace_info('new session started')
            self._session_level += 1
            LOG().trace_info('nested to session level=%d', self._session_level)
            return self

    @cmpi_logging.trace_method
    def __exit__(self, exc_type, exc_value, traceback):
        with self._session_lock:
            if self._session_level == 1:
                self._do_job(jobs.YumEndSession())
                LOG().trace_info('session ended')
            LOG().trace_info('emerged from session level=%d',
                    self._session_level)
            self._session_level = max(self._session_level - 1, 0)

    # *************************************************************************
    # Public methods
    # *************************************************************************
    @cmpi_logging.trace_method
    def clean_up(self):
        """
        Shut down the YumWorker process.
        """
        with self._reply_lock:
            if self._process is not None:
                LOG().info('terminating YumWorker')
                self._process.uplink.put(None)  # terminating command
                self._process.join()
                LOG().info('YumWorker terminated')
                self._process = None
            else:
                LOG().warn("clean_up called, when process not initialized!")

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

