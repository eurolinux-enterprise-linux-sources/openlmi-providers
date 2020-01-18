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
#
"""
This module defines session manager.
"""
import threading
import Queue as TQueue   # T as threaded
from multiprocessing import Queue

from lmi.providers import cmpi_logging
from lmi.software.util import Configuration
from lmi.software.util import get_signal_name
from lmi.software.yumdb import errors
from lmi.software.yumdb import jobs
from lmi.software.yumdb.process import YumWorker

#: Number of times the worker process will be resurrected for a completion of
#: single job. If the process dies afterwards (while still doing the same job)
#: an exception will be raised.
MAX_RESURRECTIONS = 1

LOG = cmpi_logging.get_logger(__name__)

class SessionManager(threading.Thread):
    """
    It manages *YumWorker* process that handles synchronous and asynchornous
    jobs. When this processed is killed it will be respawned, and currently
    handled job will be redone until finished. Session manager allows to
    create sessions. They cause *YumWorker* to lock the database for exclusive
    use until the session is over.

    Just one job may be processed at a time. When the job is enqueued with
    :py:meth:`process` method, no other job can be processed before its reply
    is received and picked up by the caller.

    :param reply_cond: Is a condition object used to notify any other thread
        about job's completion. This object may be used together with
        asynchronous call to :py:meth:`process` method to do some job in
        parallel and wait on thise condition afterwards until the job is
        answered by *YumWorker*.
    :type reply_cond: :py:class:`threading.Condition`
    """

    def __init__(self, reply_cond=None):
        threading.Thread.__init__(self, name="SessionManager")

        #: Recursive lock guarding concurently accessible attributes.
        self._busy_lock = threading.RLock()
        #: Condition used to notify main loop of this thread about new job
        #: available. It's also used in main loop to notify caller about
        #: reply being received and ready for picking up.
        self._busy_cond = threading.Condition(self._busy_lock)
        #: Currently processed job. ``None`` if no job is being processed.
        #: Pointer is reset to ``None`` when the reply is received.
        self._job = None

        #: Number of sessions currently active.
        self._session_level = 0

        #: Separated process accessing yum's api and handling jobs. Initialized
        #: when first needed. If this is ``False``, clean up request has been
        #: received. If ``None``, process has not been started yet or it was
        #: killed.
        self._process = None
        #: Number of resurrections for job currently processed.
        self._resurrections = 0
        #: This is filled with the answer from YumWorker. It's reset to ``None``
        #: when picked up by caller. It needs to be picked up before new job
        #: can be enqueued.
        self._reply = None

        if reply_cond is None:
            reply_cond = threading.Condition(threading.RLock())
        #: Used to wait for job to be processed and received.
        self._reply_cond = reply_cond

    @cmpi_logging.trace_method
    def _handle_reply_timeout(self, job):
        """
        This is called when timeout occurs while waiting on downlink queue for
        reply. Delay can be caused by worker process's early termination (bug).
        This handler tries to recover from such an situation.
        """
        if not self._worker.is_alive():
            if self._worker.exitcode < 0:
                msg = "worker process(pid=%d) killed by signal %s" % (
                    self._worker.pid, get_signal_name(-self._process.exitcode))
            else:
                msg = "worker process(pid=%d) is dead - exit code: %d" % (
                    self._process.pid, self._worker.exitcode)
            LOG().error("[jobid=%d] %s", job.jobid, msg)
            with self._busy_lock:
                self._resurrections += 1
                self._process = None
                LOG().error("[jobid=%d] starting new worker process", job.jobid)
                if not isinstance(job, jobs.YumLock):
                    if self._session_level > 0:
                        LOG().info('restoring session level=%d',
                                self._session_level)
                        new_session_job = jobs.YumLock()
                        self._worker.uplink.put(new_session_job)
                        reply = self._worker.downlink.get()
                        jobs.log_reply_error(new_session_job, reply)
                if self._resurrections > MAX_RESURRECTIONS:
                    LOG().warn("[jobid=%d] process has been resurrected maximum"
                            " number of times (%d times), cancelling job",
                            job.jobid, MAX_RESURRECTIONS)
                    raise errors.TransactionError(
                            "failed to complete job: %s" % (msg))
                else:
                    self._worker.uplink.put(job)
        else:
            LOG().info("[jobid=%d] process is running, waiting some more",
                    job.jobid)

    @cmpi_logging.trace_method
    def _receive_reply(self, job):
        """
        Block on downlink queue to receive expected replies from worker
        process.

        In case, that worker process terminated due to some error, restart it,
        restore session and resend the job request.
        """
        timeout = Configuration.get_instance().get_safe(
                'Jobs', 'WaitCompleteTimeout', float)
        while self._process is not False:   # process terminated
            LOG().debug("[jobid=%d] blocking on downlink queue", job.jobid)
            try:
                reply = self._worker.downlink.get(block=True, timeout=timeout)
                LOG().debug("[jobid=%d] received reply", job.jobid)
                if job.jobid != reply[0]:
                    raise errors.JobError('expected job with id=%d, got %s' % (
                        job.jobid, reply[0]))
                if self._reply:
                    raise errors.JobError('can not overwrite not yet picked'
                            ' up reply "%s" with "%s"' % (self._reply, reply))
                self._resurrections = 0
                return reply
            except TQueue.Empty:
                LOG().warn("[jobid=%d] wait for job reply timeout"
                        "(%d seconds) occured", job.jobid, timeout)
                self._handle_reply_timeout(job)
        raise errors.TerminatingError("can not complete job %s,"
                " terminating ..." % job)

    @cmpi_logging.trace_method
    def _send_and_receive(self, job):
        """
        Sends a request to server and blocks until job is processed by
        *YumWorker* and reply is received. It then notifies the called waiting
        either in the :py:meth:`process` method or just on
        :py:attr:`_reply_cond`.
        """
        with self._busy_lock:
            self._worker.uplink.put(job)
            reply = self._receive_reply(job)
            with self._reply_cond:
                self._reply = reply
                self._busy_cond.notifyAll()
                self._reply_cond.notifyAll()
                return reply

    @property
    def _worker(self):
        """
        *YumWorker* process accessor. It's created upon first need.

        :returns: Process object.
        :rtype: :py:class:`lmi.software.yumdb.process.YumWorker`
        """
        if self._process is False:
            raise errors.TerminatingError("provider is terminating")
        if self._process is None:
            LOG().trace_info("starting YumWorker")
            uplink = Queue()
            downlink = Queue()
            self._process = YumWorker(uplink, downlink)
            self._process.start()
            LOG().trace_info("YumWorker started with pid=%s", self._process.pid)
        return self._process

    def peak(self):
        """
        :returns: Received reply, if any. ``None`` is returned otherwise.
            This keeps reply with this object. Reply has always following
            format: ::

                (jobid, result_code, result_data)

        :rtype: tuple
        """
        with self._reply_cond:
            return self._reply

    @property
    def got_reply(self):
        """
        :returns: ``True`` if there is reply waiting to be picked up.
        :rtype: boolean
        """
        reply = self.peak()
        return reply is not None

    @property
    def is_busy(self):
        """
        :returns: Whether there is a job being processed.
        :type: boolean
        """
        if self._busy_lock.acquire(False):
            ret = self._job is not None
            self._busy_lock.release()
            return ret
        return False

    @cmpi_logging.trace_method
    def pop_reply(self, no_wait=False):
        """
        Wait for reply to be received, pick it up and return it.

        :param boolean no_wait: Whether to raise an exception if
            the reply is not yet available. If ``False``, wait for its
            arrival if not yet received.
        :returns: Received reply in form ``(jobid, result_code, result_data)``.
        :rtype: tuple
        """
        reply = None
        if self._busy_lock.acquire(not no_wait):
            try:
                with self._reply_cond:
                    reply = self._reply
                    self._reply = None
                if not no_wait:
                    while reply is None:
                        self._busy_cond.wait()
                        with self._reply_cond:
                            reply = self._reply
                            self._reply = None
            finally:
                self._busy_lock.release()

        if reply is None:   # this is possible only if no_wait=True
            raise TQueue.Empty("reply has not been received yet")

        return reply

    @cmpi_logging.trace_method
    def process(self, job, sync=True):
        """
        Enqueue job, and optionally wait to reply, pick it up and return it.

        :param boolean sync: Whether to wait for reply or leave immediately
            after the job is enqueued.
        :returns: Received reply if the *sync* is ``True``.
        :rtype: boolean
        """
        with self._busy_lock:
            # enqueueing job
            while self.is_busy or self._reply:
                self._busy_cond.wait()
            self._job = job
            self._busy_cond.notifyAll()
            if sync:
                # waiting for reply
                reply = self.pop_reply()
                assert reply[0] == job.jobid
                LOG().trace_info('gor reply from YumWorker: %s', reply)
                return reply

    @property
    def session_level(self):
        """
        Number of sessions currently active.
        """
        with self._busy_lock:
            return self._session_level

    @cmpi_logging.trace_method
    def begin_session(self):
        """
        Start a new session. This locks up yum database, if there has been
        not active session yet.
        """
        with self._busy_lock:
            if self._session_level == 0:
                self.process(jobs.YumLock())
                LOG().trace_info('new session started')
            self._session_level += 1
            LOG().trace_info('nested to session level=%d', self._session_level)

    @cmpi_logging.trace_method
    def end_session(self):
        """
        Stop session. This unlocks yum database, if there are any active
        sessions.
        """
        with self._busy_lock:
            if self._session_level == 1:
                self.process(jobs.YumUnlock())
                LOG().trace_info('session ended')
            LOG().trace_info('emerged from session level=%d',
                    self._session_level)
            self._session_level = max(self._session_level - 1, 0)

    def run(self):
        """
        Main loop of session manager. It resends all enqueued jobs to YumWorker,
        accepts replies and notifies callers.
        """
        LOG().info('%s started', self.name)
        while True:
            with self._busy_lock:
                while not self._job:
                    self._busy_cond.wait()
                job = self._job
                LOG().debug('sending job %s to YumWorker', job)
                if not isinstance(job, jobs.YumShutDown):
                    self._send_and_receive(job)
                    self._job = None
                else:   # last job - terminate Yum Worker
                    if self._process:
                        self._process.uplink.put(job)
                        self._process.join()
                        LOG().info('YumWorker terminated')
                    # prohibit next instantiation
                    self._process = False
                    with self._reply_cond:
                        self._reply = (
                                job.jobid, jobs.YumJob.RESULT_SUCCESS, None)
                        self._busy_cond.notifyAll()
                        self._reply_cond.notifyAll()
                    self._job = None
                    break
        LOG().info('%s terminated', self.name)
