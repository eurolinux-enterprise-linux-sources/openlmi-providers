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
This is a module for ``JobManager`` which is a standalone thread in provider
process. It keeps a cache of asynchronous jobs and handles input queue of jobs
that are about to be handled by
:py:class:`~lmi.software.yumdb.process.YumWorker` process.

Before using ``JobManager``, module's variable ``JOB_TO_MODEL`` should be set
to a callable taking :py:class:`~lmi.software.yumdb.jobs.YumJob` instance and
returning its matching CIM abstraction instance.
"""
from functools import wraps
import heapq
import inspect
import sys
import threading
import time
import traceback

from lmi.providers import cmpi_logging
from lmi.providers.IndicationManager import IndicationManager
from lmi.providers.JobManager import JobManager as JM
from lmi.software.yumdb import errors
from lmi.software.yumdb import jobs
from lmi.software.yumdb.sessionmanager import SessionManager
from lmi.software.util import Configuration

# This is a callable, which must be initialized before JobManager is used.
# It should be a pointer to function, which takes a job and returns
# corresponding CIM instance. It's used for sending indications.
JOB_TO_MODEL = lambda job: None

LOG = cmpi_logging.get_logger(__name__)

# *****************************************************************************
# Decorators
# *****************************************************************************
def job_handler(job_from_target=True):
    """
    Decorator for JobManager methods serving as handlers for control jobs.

    If job_from_target is True, 'job' MUST be the first parameter of decorated
    method!

    Decorator locks the :py:attr:`JobManager._job_lock` of manager's instance.
    """
    def _wrapper_jft(method):
        """
        It consumes "job" keyword argument (which is job's id) and makes
        it an instance of YumJob. The method is then called with the instance
        of YumJob instead of job id.
        """
        logged = cmpi_logging.trace_method(method, frame_level=2)

        @wraps(method)
        def _new_func(self, *args, **kwargs):
            """Wrapper around method."""
            if 'target' in kwargs:
                target = kwargs.pop('target')
            elif 'job' in kwargs:
                target = kwargs.pop('job')
            else:
                # 'job' is not in kwargs, it _must_ be in args then
                assert len(args) > 0
                target = args[0]
                args = args[1:]

            with self._job_lock:
                if not target in self._async_jobs:
                    raise errors.JobNotFound(target)
                job = self._async_jobs[target]
                return logged(self, job, *args, **kwargs)
        return _new_func

    def _simple_wrapper(method):
        """Just locks the job lock."""
        @wraps(method)
        def _new_func(self, *args, **kwargs):
            """Wrapper around method."""
            with self._job_lock:
                return method(self, *args, **kwargs)
        return _new_func

    if job_from_target:
        return _wrapper_jft
    else:
        return _simple_wrapper

class JobIndicationSender(object):
    """
    Makes creation and sending of indications easy. It keeps a reference
    to job, which can be *snapshotted* for making CIM instance out of it.
    These instances are then used to send indications via
    :py:class:`lmi.providers.IndicationManager.IndicationManager`.

    Typical usage::

        sender = JobIndicationSender(im, job, [fltr_id1, fltr_id2])
            ... # modify job
        sender.snapshot()
        sender.send()

    .. note::
        Number of kept CIM instances won't exceed 2. First one is created upon
        instantiation and the second one with the subsequent call to
        :py:meth:`snapshot()`. Any successive call to it will overwrite the
        second instance.

    :param job: Is job instance, which will be immediately snapshoted as old
        instance and later as a new one.
    :type job: :py:class:`~lmi.software.yumdb.jobs.YumJob`
    :param list indications: Can either be a list of indication ids or a single
        indication id.
    :param new: A job instance stored as new.
    :type new: :py:class:`~lmi.software.yumdb.jobs.YumJob`
    """

    def __init__(self, indication_manager, job,
            indications=JM.IND_JOB_CHANGED, new=None):
        if not isinstance(indication_manager, IndicationManager):
            raise TypeError("indication_manager must be a subclass of"
                    " IndicationManager")
        if not isinstance(job, jobs.YumJob):
            raise TypeError("job must be an instance of YumJob")
        if not new is None and not isinstance(new, jobs.YumJob):
            raise TypeError("new must be an instance of YumJob")
        self._indication_manager = indication_manager
        self._job = job
        self._old_instance = JOB_TO_MODEL(job)
        if new is not None:
            new = JOB_TO_MODEL(job)
        self._new_instance = new
        self._indications = set()
        self.indication_ids = indications

    @property
    def job(self):
        """
        :returns: Associated job object.
        :rtype: :py:class:`~lmi.software.yumdb.jobs.YumJob`
        """
        return self._job

    @property
    def indication_ids(self):
        """
        :returns: Set of indication filter IDs.
        :rtype: set
        """
        return self._indications.copy()

    @indication_ids.setter
    def indication_ids(self, indication_ids):
        """
        Set the indication filter IDs.

        :param list indication_ids: Can be even single id.
        """
        if isinstance(indication_ids, basestring):
            indication_ids = set((indication_ids,))
        self._indications = set(indication_ids)

    @cmpi_logging.trace_method
    def add_indication_ids(self, indication_ids):
        """
        Add filter IDs.
        """
        if isinstance(indication_ids, basestring):
            indication_ids = set((indication_ids,))
        self._indications.update(indication_ids)

    @cmpi_logging.trace_method
    def snapshot(self):
        """
        Make a second CIM instance, overwriting previous one (not the first).
        """
        self._new_instance = JOB_TO_MODEL(self._job)

    @cmpi_logging.trace_method
    def send(self, make_snapshot=False):
        """
        Send all requested indications for given job.
        """
        if not self._indications:
            raise errors.IndicationError(
                    "can not send any indication without id")
        if make_snapshot:
            self.snapshot()
        if (   JM.IND_JOB_CHANGED in self._indications
           and self._new_instance is None):
            raise errors.IndicationError("no snapshot made for modified job")
        indication_manager = IndicationManager.get_instance()
        for fltr_id in self._indications:
            if fltr_id == JM.IND_JOB_CREATED:
                LOG().debug("sending instance creation indication for job %s",
                        self._job)
                indication_manager.send_instcreation(
                        self._new_instance if self._new_instance is not None
                                           else self._old_instance,
                        fltr_id)
            else:
                LOG().debug("sending instance modification indication for job"
                        " %s with ID: %s", self._job, fltr_id)
                indication_manager.send_instmodification(
                        self._old_instance, self._new_instance,
                        fltr_id)

class JobManager(threading.Thread):
    """
    Separate thread for managing queue of jobs requested by client.
    There are three kinds of jobs, that are handled differently:

      * asynchronous - kept in ``_async_jobs`` dictionary until job is
            deleted by request or it expires;
            no reply is sent to client upon job's completion
      * synchronous  - reply is sent to client after job's completion;
            no reference to the job is kept afterwards
      * job control  - they are not enqueued in ``_job_queue`` for
            :py:class:`YumWorker` to process, but are handled directly and in
            the *FIFO* order

    Both asynchronous and synchronous jobs are enqueued in ``_job_queue`` and
    will be sent to :py:class:`~lmi.software.yumdb.process.YumWorker` for
    processing. It's a priority queue sorting jobs by their priority.
    :py:class:`~lmi.software.yumdb.sessionmanager.SessionManager` is the actual
    object managing ``YumWorker`` process. All jobs are passed to it.
    """
    #: enumeration of actions, that may be enqueued in calendar
    ACTION_REMOVE = 0

    #: names for actions defined above, their values are indexes to this list
    ACTION_NAMES = ['remove']

    def __init__(self):
        threading.Thread.__init__(self, name="JobManager")

        #: Session manager shall be accessed via property ``session_manager``.
        #: It's instantiated when first needed.
        self._sessionmgr = None

        #: lock for critical access to _sessionmgr, _calendar,
        #: _async_jobs and _job_queue
        self._job_lock = threading.RLock()
        #: priority queue of jobs that are processed by YumWorker
        self._job_queue = []
        #: (time, jobid, action)
        self._calendar = []
        #: {jobid : job}
        self._async_jobs = {}
        #: Condition object being wait upon in the main loop of this thread.
        #: It's notified either when the new job request comes or when the
        #: YumWorker replies.
        self._job_pending = threading.Condition(self._job_lock)

        #: used to guard access to _finished set
        self._reply_lock = threading.Lock()
        #: used to wait for job to be processed and received
        self._reply_cond = threading.Condition(self._reply_lock)
        #: job ids are kept here for each finished job
        self._finished = set()

    # *************************************************************************
    # Private methods
    # *************************************************************************
    @cmpi_logging.trace_method
    def _control_job(self, job):
        """
        Function dispatching job to handler for particular
        :py:class:`~lmi.software.yumdb.jobs.YumJob` subclass.

        .. note::
            This can be called only from client thread.
        """
        try:
            handler = {
                # these are from YumDB client
                jobs.YumJobGetList     : self._handle_get_list,
                jobs.YumJobGet         : self._handle_get,
                jobs.YumJobGetByName   : self._handle_get_by_name,
                jobs.YumJobSetPriority : self._handle_set_priority,
                jobs.YumJobReschedule  : self._handle_reschedule,
                jobs.YumJobUpdate      : self._handle_update,
                jobs.YumJobDelete      : self._handle_delete,
                jobs.YumJobTerminate   : self._handle_terminate,
            }[job.__class__]
            LOG().info("processing control job %s", str(job))
        except KeyError:
            raise errors.UnknownJob("No handler for job \"%s\"." %
                    job.__class__.__name__)
        return handler(**job.job_kwargs)

    @cmpi_logging.trace_method
    def _enqueue_job(self, job):
        """
        Insert incoming job into :py:attr:`_job_queue` or handle it directly if
        it's control job.

        .. note::
            This can be called only from client thread.
        """
        if isinstance(job, jobs.YumJobControl):
            result = job.RESULT_SUCCESS
            job.start()
            try:
                data = self._control_job(job)
            except Exception:   #pylint: disable=W0703
                result = job.RESULT_ERROR
                data = sys.exc_info()
                data = (data[0], data[1], traceback.format_tb(data[2]))
                LOG().exception("control job %s failed", job)
            self._finish_job(job, result, data)
        else:
            LOG().debug('job %s enqued for YumWorker to handle', job)
            heapq.heappush(self._job_queue, job)
            if getattr(job, 'async', False) is True:
                ind = self._prepare_indication_for(job, JM.IND_JOB_CREATED)
                self._async_jobs[job.jobid] = job
                ind.send()
            self._job_pending.notify()

    @cmpi_logging.trace_method
    def _schedule_event(self, after, jobid, action):
        """
        Enqueue event into calendar. Event consists of *time*, *jobid* and
        *action*.

        :param float after: Number of seconds from now until the action is
            shall be triggered.
        :param int jobid: Id of job which is a subject of action execution.
        :param int action: Value of action to do.

        .. note::
            This can be called only from client thread.
        """
        schedule_at = time.time() + after
        for (sched, jid, act) in self._calendar:
            if jid == jobid and act == action:
                if sched <= schedule_at:    # same event already scheduled
                    return
                # schedule it for early time
                LOG().debug('rescheduling action %s on job %d to take place'
                        ' after %d seconds (instead of %d)',
                        self.ACTION_NAMES[action], jid, after,
                        sched - schedule_at + after)
                self._calendar.remove((sched, jid, act))
                self._calendar.append((schedule_at, jid, act))
                heapq.heapify(self._calendar)
                return
        LOG().debug('scheduling action %s on job %d to take place after '
                ' %d seconds', self.ACTION_NAMES[action], jobid, after)
        heapq.heappush(self._calendar, (schedule_at, jobid, action))

    @cmpi_logging.trace_method
    def _run_event(self, jobid, action):
        """
        Process event from calendar.

        :param int jobid: Id of job that is a subject of action.
        :param int action: One of predefined actions to be executed on
            particular job.
        """
        LOG().info('running action %s on job(id=%d)',
                self.ACTION_NAMES[action], jobid)
        if action == self.ACTION_REMOVE:
            with self._job_lock:
                del self._async_jobs[jobid]
        else:
            msg = "unsupported action: %s" % action
            raise ValueError(msg)

    @cmpi_logging.trace_method
    def _prepare_indication_for(self, job, *args, **kwargs):
        """
        Convenience method making indication sender context manager.

        :param list args: Positional arguments passed after *job* to
            the constructor of :py:class:`JobIndicationSender`.
        :param dictionary kwargs: Its keyword arguments.
        :returns: Context manager for snapshoting and sending job replated
            indications.
        :rtype: :py:class:`JobIndicationSender`
        """
        return JobIndicationSender(IndicationManager.get_instance(), job,
            *args, **kwargs)

    @property
    def session_manager(self):
        """
        Accessor of session manager. Object is instantiated upon first access.

        :rtype: :py:class:`lmi.software.yumdb.sessionmanager.SessionManager`
        """
        with self._job_lock:
            if self._sessionmgr is None:
                self._sessionmgr = SessionManager(self._job_pending)
                self._sessionmgr.start()
            return self._sessionmgr

    # *************************************************************************
    # Job handlers
    # *************************************************************************
    @job_handler()
    def _handle_get(self, job):
        """ Control job handler returning job object requested. """
        return job

    @job_handler(False)
    def _handle_get_list(self):
        """ Control job handler returning list of all asynchronous jobs. """
        with self._job_lock:
            return sorted(self._async_jobs.values())

    @job_handler(False)
    def _handle_get_by_name(self, target):
        """
        Control job handler returning job object requested by its name.

        :param string target: Name of job to find. ``Name`` property of
            available asynchronous jobs is queried.
        """
        for job in self._async_jobs.values():
            if 'name' in job.metadata and target == job.metadata['name']:
                return job
        raise errors.JobNotFound(target)

    @job_handler()
    def _handle_set_priority(self, job, new_priority):
        """
        Control job handler modifying job's priority and updating its position
        in queue.

        :returns: Modified job object.
        :rtype: :py:class:`~lmi.software.yumdb.jobs.YumJob`
        """
        if not isinstance(new_priority, (int, long)):
            raise TypeError('priority must be an integer')
        if job.priority != new_priority:
            ind = self._prepare_indication_for(job)
            job.update(priority=new_priority)
            if job in self._job_queue:
                heapq.heapify(self._job_queue)
            ind.send(True)
        return job

    @job_handler()
    def _handle_reschedule(self, job,
            delete_on_completion,
            time_before_removal):
        """
        Control job handler rescheduling job's deletion.

        :param job: Affected job object.
        :type job: :py:class:`~lmi.software.yumdb.jobs.YumJob`
        :param boolean delete_on_completion: Whether the job shall be
            automatically removed after its completion. The
            *time_before_removel* then comes into play.
        :param float time_before_removal: Number of seconds to wait before
            job's automatic removal.
        """
        if (   job.delete_on_completion == delete_on_completion
           and job.time_before_removal == time_before_removal):
            return
        if job.finished and job.delete_on_completion:
            for i, event in enumerate(self._calendar):
                if event[1] == job.jobid and event[2] == self.ACTION_REMOVE:
                    del self._calendar[i]
                    heapq.heapify(self._calendar)
                    break
        ind = self._prepare_indication_for(job)
        if delete_on_completion:
            schedule_at = time_before_removal
            if job.finished:
                schedule_at = job.finished + schedule_at - time.time()
            self._schedule_event(schedule_at, job.jobid, self.ACTION_REMOVE)
        job.delete_on_completion = delete_on_completion
        job.time_before_removal = time_before_removal
        ind.send(True)
        return job

    @job_handler()
    def _handle_update(self, job, data):
        """
        Control job handler updating any job metadata.

        :param dictionary data: Job attributes with associated values that
            shall be assigned to particular *job*.
        """
        ind = self._prepare_indication_for(job)
        job.update(**data)
        ind.send(True)
        return job

    @job_handler()
    def _handle_delete(self, job):
        """
        Control job handler deleting finished asynchronous job. If the *job* is
        not yet finished, the
        :py:exc:`lmi.software.yumdb.errors.InvalidJobState` is raised.
        """
        if not job.finished:
            raise errors.InvalidJobState(
                    'can not delete unfinished job "%s"' % job)
        try:
            self._job_queue.remove(job)
            heapq.heapify(self._job_queue)
            LOG().debug('job "%s" removed from queue', job)
        except ValueError:
            LOG().debug('job "%s" not started and not enqueued', job)
        del self._async_jobs[job.jobid]
        return job

    @job_handler()
    def _handle_terminate(self, job):
        """
        Control job handler terminating not started job.
        """
        if job.started and not job.finished:
            raise errors.InvalidJobState('can not kill running job "%s"' % job)
        if job.finished:
            raise errors.InvalidJobState('job "%s" already finished' % job)
        self._job_queue.remove(job)
        heapq.heapify(self._job_queue)
        ind = self._prepare_indication_for(job)
        job.finish(result=job.RESULT_TERMINATED)
        ind.send(True)
        LOG().info('terminated not started job "%s"', job)
        return job

    # *************************************************************************
    # Public methods
    # *************************************************************************
    @cmpi_logging.trace_method
    def _finish_job(self, job, result, result_data):
        """
        This should be called on any processed job.

        If the *job* is synchronous, reply is send at once. Otherwise the
        result is stored for later client's query in the job itself, and
        indications are sent.

        .. note::
            This shall be called only from :py:meth:`run` method.

        :param int result: Result code that will be stored in a *job*.
        :param result_data: Resulting data object that is to be stored in a
            *job*.
        :returns: Job object.
        """
        if job.state != job.RUNNING:
            raise errors.InvalidJobState(
                    'can not finish not started job "%s"' % job)
        if getattr(job, 'async', False):
            ind = self._prepare_indication_for(job,
                    (JM.IND_JOB_CHANGED, JM.IND_JOB_PERCENT_UPDATED))
        job.finish(result, result_data)
        if getattr(job, 'async', False):
            if job.delete_on_completion:
                default = Configuration.get_instance().get_safe(
                            'Log', 'MinimumTimeBeforeRemoval', float)
                schedule_at = max(job.time_before_removal, default)
                self._schedule_event(schedule_at, job.jobid,
                        self.ACTION_REMOVE)
            if result == job.RESULT_SUCCESS:
                ind.add_indication_ids(JM.IND_JOB_SUCCEEDED)
            elif result == job.RESULT_ERROR:
                ind.add_indication_ids(JM.IND_JOB_FAILED)
            ind.send(True)
        with self._reply_lock:
            self._finished.add(job.jobid)
            self._reply_cond.notifyAll()
        return job

    @cmpi_logging.trace_method
    def _get_job(self):
        """
        Pop the first job enqueued by client out of :py:attr:`_job_queue`.
        Job is started and returned.

        :returns: Job object. If the queue is empty, ``None`` is returned.
        :rtype: :py:class:`~lmi.software.yumdb.jobs.YumJob`
        """
        if self._job_queue:
            job = heapq.heappop(self._job_queue)
            if getattr(job, "async", False):
                ind = self._prepare_indication_for(job,
                        (JM.IND_JOB_CHANGED, JM.IND_JOB_PERCENT_UPDATED))
                job.start()
                ind.send(True)
            else:
                job.start()
            return job

    @cmpi_logging.trace_method
    def process(self, job):
        """
        Enqueue given job and block until it's processed.

        :returns: Modified job object with assigned result.
        :rtype: :py:class:`~lmi.software.yumdb.jobs.YumJob`
        """
        with self._job_lock:
            self._enqueue_job(job)
        if getattr(job, 'async', False):
            return job.jobid
        with self._reply_lock:
            while job.jobid not in self._finished:
                self._reply_cond.wait()
            return job

    @cmpi_logging.trace_method
    def _clean_up(self):
        """
        Release the session manager with its separated process.
        This shall be called from the main loop of this thread.
        """
        with self._job_lock:
            if self._sessionmgr is not None:
                self._sessionmgr.join()
                self._sessionmgr = False

    @cmpi_logging.trace_method
    def begin_session(self):
        """
        Nest into a session. This means: lock the yum database for exclusive
        use until the top-level session ends. The opposite method,
        :py:meth:`end_session` needs to be called same number of times in order
        for yum database to be released.
        """
        return self.session_manager.begin_session()

    @cmpi_logging.trace_method
    def end_session(self):
        """
        Emerge from session. When the last session ends, yum database is
        unlocked.

        .. see::
            :py:meth:`begin_session`
        """
        return self.session_manager.end_session()

    def run(self):
        """
        Main lool of *JobManager*'s thread. It works with just one job object
        at once until it's processed. It consumes enqueued jobs in
        :py:attr:`_job_queue` and passes them to session manager object. It
        also triggers actions scheduled in calendar.
        """
        LOG().info("%s thread started", self.name)
        # This points to the currect job being processed by session manager.
        # If no job is being processed, it's ``None``.
        job = None
        with self._job_lock:
            while True:
                timeout = None

                # check calendar for scheduled events
                if self._calendar:
                    timeout = self._calendar[0][0] - time.time()
                while timeout is not None and timeout <= 0:
                    _, jobid, action = heapq.heappop(self._calendar)
                    self._run_event(jobid, action)
                    if self._calendar:
                        timeout = self._calendar[0][0] - time.time()
                    else:
                        timeout = None

                # handle processed job if any
                if job:
                    if self.session_manager.got_reply:
                        _, result, data = self.session_manager.pop_reply()
                        self._finish_job(job, result, data)
                        if isinstance(job, jobs.YumShutDown):
                            break
                        job = None

                # check newly enqueued jobs
                if job is None:
                    job = self._get_job()
                    if job:
                        self.session_manager.process(job, sync=False)

                # wait for any event
                LOG().debug('waiting on input queue for job%s',
                    (' with timeout %s' % timeout) if timeout else '')
                self._job_pending.wait(timeout=timeout)

        self._clean_up()
        LOG().info('%s thread terminating', self.name)

