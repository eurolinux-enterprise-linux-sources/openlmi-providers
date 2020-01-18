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
This is a module for ``JobManager`` which is a separate thread of
``YumWorker`` process. It keeps a cache of asynchronous jobs and handles
input and output queues.

This module uses its own logging facilities because it runs in separeted
process not having access to broker logging features.

Before using ``JobManager``, module's variable ``JOB_TO_MODEL`` should
be set to callable taking ``YumJob`` instance and returning its matching
CIM abstraction instance.
"""
from functools import wraps
import heapq
import inspect
import logging
import Queue
import sys
import threading
import time
import traceback

from lmi.providers import cmpi_logging
from lmi.providers.IndicationManager import IndicationManager
from lmi.providers.JobManager import JobManager as JM
from lmi.software.yumdb import errors
from lmi.software.yumdb import jobs
from lmi.software.util import Configuration

# This is a callable, which must be initialized before JobManager is used.
# It should be a pointer to function, which takes a job and returns
# corresponding CIM instance. It's used for sending indications.
JOB_TO_MODEL = lambda job: None

# replacement for cmpi_logging.logger
LOG = None

# *****************************************************************************
# Decorators
# *****************************************************************************
def job_handler(job_from_target=True):
    """
    Decorator for JobManager methods serving as handlers for control jobs.

    Decorator locks the job_lock of manager's instance.
    """
    def _wrapper_jft(method):
        """
        It consumes "target" keyword argument (which is job's id) and makes
        it an instance of YumJob. The method is then called with "job" argument
        instead of "target".
        """
        logged = cmpi_logging.trace_method(method, frame_level=2)

        @wraps(method)
        def _new_func(self, *args, **kwargs):
            """Wrapper around method."""
            if 'target' in kwargs:
                kwargs['job'] = kwargs.pop('target')
            callargs = inspect.getcallargs(method, self, *args, **kwargs)
            target = callargs.pop('job')
            with self._job_lock:                #pylint: disable=W0212
                if not target in self._async_jobs:  #pylint: disable=W0212
                    raise errors.JobNotFound(target)
                job = self._async_jobs[target]  #pylint: disable=W0212
                callargs['job'] = job
                return logged(**callargs)
        return _new_func

    def _simple_wrapper(method):
        """Just locks the job lock."""
        @wraps(method)
        def _new_func(self, *args, **kwargs):
            """Wrapper around method."""
            with self._job_lock:                #pylint: disable=W0212
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
    These instances are then used to send indications via IndicationManager.

    Typical usage::

        sender = JobIndicationSender(im, job, [fltr_id1, fltr_id2])
            ... # modify job
        sender.snapshot()
        sender.send()

    **Note** that number of kept CIM instances won't exceed 2. First one
    is created upon instantiation and the second one be calling
    ``snapshot()``. Any successive call to ``snapshot()`` will overwrite
    the second instance.
    """

    def __init__(self, indication_manager, job,
            indications=JM.IND_JOB_CHANGED, new=None):
        """
        :param job (``YumJob``) Is job instance, which will be immediately
            snapshoted as old instance and later as a new one.
        :param indications (``list``) Can either be a list of indication ids
            or a single indication id.
        :param new (``YumJob``) A job instance stored as new.
        """
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
        Return instance of ``YumJob``.
        """
        return self._job

    @property
    def indication_ids(self):
        """
        Return set of indication filter IDs.
        """
        return self._indications.copy()

    @indication_ids.setter
    def indication_ids(self, indication_ids):
        """
        Set the indication filter IDs.

        :param indication_ids (``list``) Can be even single id.
        """
        if isinstance(indication_ids, basestring):
            indication_ids = set([indication_ids])
        self._indications = set(indication_ids)

    @cmpi_logging.trace_method
    def add_indication_ids(self, indication_ids):
        """
        Add filter IDs.
        """
        if isinstance(indication_ids, basestring):
            indication_ids = set([indication_ids])
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
        for fltr_id in self._indications:
            if fltr_id == JM.IND_JOB_CREATED:
                LOG.debug("sending instance creation indication for job %s",
                        self._job)
                self._indication_manager.send_instcreation(
                        self._new_instance if self._new_instance is not None
                                           else self._old_instance,
                        fltr_id)
            else:
                LOG.debug("sending instance modification indication for job %s"
                        " with ID: %s", self._job, fltr_id)
                self._indication_manager.send_instmodification(
                        self._old_instance, self._new_instance,
                        fltr_id)

class JobManager(threading.Thread):
    """
    Separate thread for managing queue of jobs requested by client.
    There are three kinds of jobs, that are handled differently:
      * asynchronous - kept in _async_jobs dictionary until job is
            deleted by request or it expires;
            no reply is sent to client upon job's completion
      * synchronous  - reply is sent to client after job's completion;
            no reference to the job is kept afterwards
      * job control  - they are not enqueued in _job_queue for YumWorker
            to process, but are handled directly and in the FIFO order

    Both asynchronous and synchronous jobs are enqueued in _job_queue
    for YumWorker to obtain them. It's a priority queue sorting jobs by their
    priority.
    """
    # enumeration of actions, that may be enqueued in calendar
    ACTION_REMOVE = 0

    ACTION_NAMES = ['remove']

    def __init__(self, queue_in, queue_out, indication_manager):
        threading.Thread.__init__(self, name="JobManager")
        self._queue_in = queue_in
        self._queue_out = queue_out
        self._indication_manager = indication_manager
        self._terminate = False

        # (time, jobid, action)
        self._calendar = []
        # {jobid : job}
        self._async_jobs = {}

        # lock for critical access to _calendar, _async_jobs and _job_queue
        self._job_lock = threading.RLock()
        # priority queue of jobs that are processed by YumWorker
        self._job_queue = []
        # condition for YumWorker waiting on empty _job_queue
        self._job_enqueued = threading.Condition(self._job_lock)

    # *************************************************************************
    # Private methods
    # *************************************************************************
    @cmpi_logging.trace_method
    def _control_job(self, job):
        """
        Function dispatching job to handler for particular YumJob subclass.
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
            LOG.info("processing control job %s", str(job))
        except KeyError:
            raise errors.UnknownJob("No handler for job \"%s\"." %
                    job.__class__.__name__)
        return handler(**job.job_kwargs)

    @cmpi_logging.trace_method
    def _enqueue_job(self, job):
        """
        Insert incoming job into _job_queue.
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
                LOG.exception("control job %s failed", job)
            job.finish(result, data)
            LOG.debug("sending reply for %s: (%s, %s)", job,
                    job.ResultNames[job.result],
                    cmpi_logging.render_value(job.result_data))
            self._queue_out.put(job)
        else:
            if job is None:
                LOG.debug('received terminating command')
                self._terminate = True
            LOG.debug('job %s enqued for YumWorker to handle', job)
            heapq.heappush(self._job_queue, job)
            if getattr(job, 'async', False) is True:
                ind = self._prepare_indication_for(job, JM.IND_JOB_CREATED)
                self._async_jobs[job.jobid] = job
                ind.send()
            self._job_enqueued.notify()

    @cmpi_logging.trace_method
    def _schedule_event(self, after, jobid, action):
        """
        Enqueue event into calendar. Event consists of time, jobid and
        action.
        """
        schedule_at = time.time() + after
        for (sched, jid, act) in self._calendar:
            if jid == jobid and act == action:
                if sched <= schedule_at:    # same event already scheduled
                    return
                # schedule it for early time
                LOG.debug('rescheduling action %s on job %d to take place'
                        ' after %d seconds (instead of %d)',
                        self.ACTION_NAMES[action], jid, after,
                        sched - schedule_at + after)
                self._calendar.remove((sched, jid, act))
                self._calendar.append((schedule_at, jid, act))
                heapq.heapify(self._calendar)
                return
        LOG.debug('scheduling action %s on job %d to take place after '
                ' %d seconds', self.ACTION_NAMES[action], jobid, after)
        heapq.heappush(self._calendar, (schedule_at, jobid, action))

    @cmpi_logging.trace_method
    def _run_event(self, jobid, action):
        """
        Process event from calendar.
        """
        if action == self.ACTION_REMOVE:
            with self._job_lock:
                del self._async_jobs[jobid]
        else:
            msg = "unsupported action: %s" % action
            raise ValueError(msg)

    @cmpi_logging.trace_method
    def _prepare_indication_for(self, job, *args, **kwargs):
        """
        Return instance of ``JobIndicationSender``.
        """
        return JobIndicationSender(self._indication_manager, job,
            *args, **kwargs)

    # *************************************************************************
    # Job handlers
    # *************************************************************************
    @job_handler()
    def _handle_get(self, job):     #pylint: disable=R0201
        """@return job object"""
        return job

    @job_handler(False)
    def _handle_get_list(self):
        """@return list of all asynchronous jobs"""
        with self._job_lock:
            return sorted(self._async_jobs.values())

    @job_handler(False)
    def _handle_get_by_name(self, target):
        """@return job object filtered by name"""
        for job in self._async_jobs.values():
            if 'name' in job.metadata and target == job.metadata['name']:
                return job
        raise errors.JobNotFound(target)

    @job_handler()
    def _handle_set_priority(self, job, new_priority):
        """
        Modify job's priority and updates its position in queue.
        @return modified job object
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
        Changes job's schedule for its deletion.
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
    def _handle_update(self, job, data):    #pylint: disable=R0201
        """
        Updates any job metadata.
        """
        ind = self._prepare_indication_for(job)
        job.update(**data)
        ind.send(True)
        return job

    @job_handler()
    def _handle_delete(self, job):
        """
        Deletes finished asynchronous job.
        """
        if not job.finished:
            raise errors.InvalidJobState(
                    'can not delete unfinished job "%s"' % job)
        try:
            self._job_queue.remove(job)
            heapq.heapify(self._job_queue)
            LOG.debug('job "%s" removed from queue', job)
        except ValueError:
            LOG.debug('job "%s" not started and not enqueued', job)
        del self._async_jobs[job.jobid]
        return job

    @job_handler()
    def _handle_terminate(self, job):
        """
        Terminates not started job.
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
        LOG.info('terminated not started job "%s"', job)
        return job

    # *************************************************************************
    # Public properties
    # *************************************************************************
    @property
    def queue_in(self):
        """Incoming queue for YumJob instances."""
        return self._queue_in

    @property
    def queue_out(self):
        """Output queue for results."""
        return self._queue_out

    # *************************************************************************
    # Public methods
    # *************************************************************************
    @cmpi_logging.trace_method
    def finish_job(self, job, result, result_data):
        """
        This should be called for any job by YumWorker after the job is
        processed.

        If the job is synchronous, reply is send at once. Otherwise the result
        is stored for later client's query in the job itself.
        """
        with self._job_lock:
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
            else:
                LOG.debug("sending reply for %s: (%s, %s)", job,
                        job.ResultNames[job.result],
                        cmpi_logging.render_value(job.result_data))
                self._queue_out.put(job)
            return job

    @cmpi_logging.trace_method
    def get_job(self, block=True, timeout=None):
        """
        Method supposed to be used only by YumWorker. It pops the first job
        from _job_queue, starts it and returns it.
        """
        start = time.time()
        with self._job_lock:
            if len(self._job_queue) == 0 and not block:
                raise Queue.Empty
            while len(self._job_queue) == 0:
                if timeout:
                    LOG.debug('waiting for job for %s seconds' % timeout)
                self._job_enqueued.wait(timeout)
                if len(self._job_queue) == 0:
                    now = time.time()
                    if timeout > now - start:
                        raise Queue.Empty
            job = heapq.heappop(self._job_queue)
            if job is not None:
                if getattr(job, "async", False):
                    ind = self._prepare_indication_for(job,
                            (JM.IND_JOB_CHANGED, JM.IND_JOB_PERCENT_UPDATED))
                    job.start()
                    ind.send(True)
                else:
                    job.start()
            return job

    def run(self):
        """The entry point of thread."""
        global LOG      #pylint: disable=W0603
        LOG = logging.getLogger(__name__)
        LOG.info("%s thread started", self.name)

        while self._terminate is False:
            try:
                timeout = None
                with self._job_lock:
                    if len(self._calendar) > 0:
                        timeout = self._calendar[0][0] - time.time()
                LOG.debug('waiting on input queue for job%s',
                    (' with timeout %s' % timeout) if timeout else '')
                job = self._queue_in.get(timeout=timeout)
                with self._job_lock:
                    self._enqueue_job(job)
                    while not self._queue_in.empty():
                        # this won't throw
                        self._enqueue_job(self._queue_in.get_nowait())

            except Queue.Empty:
                with self._job_lock:
                    while (   len(self._calendar)
                          and self._calendar[0][0] < time.time()):
                        _, jobid, action = heapq.heappop(self._calendar)
                        LOG.info('running action %s on job(id=%d)',
                                self.ACTION_NAMES[action], jobid)
                        self._run_event(jobid, action)
        LOG.info('%s thread terminating', self.name)

