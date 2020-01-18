# -*- encoding: utf-8 -*-
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
Define job classes representing kinds of jobs of worker process.
"""

import logging
import os
import threading
import time
import yum

from lmi.software import util
from lmi.software.yumdb import errors
from lmi.software.yumdb.packageinfo import PackageInfo
from lmi.software.yumdb.repository import Repository

class YumJob(object):  #pylint: disable=R0903
    """
    Base class for any job, that is processable by YumWorker process.
    It contains jobid attribute, that must be unique for
    each job, it's counted from zero a incremented after each creation.

    metadata attribute typically contain:
      name        - name of job, that is modifiable by user
      method      - identificator of method, that lead to creation of job
    """
    __slots__ = ( 'jobid', 'created', 'started', 'finished', 'last_change'
                , 'priority', 'result', 'result_data')

    # jobs can be created concurrently from multiple threads, that's
    # why we need to make its creation thread safe
    _JOB_ID_LOCK = threading.Lock()
    _JOB_ID = 0

    # job state enumeration
    NEW, RUNNING, COMPLETED, TERMINATED, EXCEPTION = range(5)
    # job result enumeration
    RESULT_SUCCESS, RESULT_TERMINATED, RESULT_ERROR = range(3)

    ResultNames = ("success", "terminated", "error")

    @staticmethod
    def _get_job_id():
        """
        Generates new job ids. It should be called only from constructor
        of YumJob. Ensures, that each job has a unique number.
        @return number of jobs created since program start -1
        """
        with YumJob._JOB_ID_LOCK:
            val = YumJob._JOB_ID
            YumJob._JOB_ID += 1
        return val

    @classmethod
    def handle_ignore_job_props(cls):
        """
        @return set of job properties, that does not count as job's handler
        arguments - job handler does not care fore metadata, jobid, priority,
        etc...
        """
        return set(YumJob.__slots__)

    def __init__(self, priority=None):
        if priority is None:
            priority = util.Configuration.get_instance().get_safe(
                    'Jobs', 'DefaultPriority', int)
        if not isinstance(priority, (int, long)):
            raise TypeError("priority must be integer")
        self.jobid = self._get_job_id()
        self.started = None
        self.finished = None
        self.priority = priority
        self.created = time.time()
        self.last_change = self.created
        self.result = None
        self.result_data = None

    @property
    def state(self):
        """
        @return integer representing job's state
        """
        if not self.started:
            return self.NEW
        if not self.finished:
            return self.RUNNING
        if self.result == self.RESULT_ERROR:
            return self.EXCEPTION
        if self.result == self.RESULT_TERMINATED:
            return self.TERMINATED
        return self.COMPLETED

    @property
    def job_kwargs(self):
        """
        Jobs are in worker handled in handlers specific for each subclass.
        These handlers are methods of worker. They accepts concrete arguments
        that can be obtained from job by invoking this property.
        @return dictionary of keyword arguments of job
        """
        kwargs = {}
        cls = self.__class__
        while not cls in (YumJob, object):
            for slot in cls.__slots__:
                if (   not slot in kwargs
                   and not slot in cls.handle_ignore_job_props()):
                    kwargs[slot] = getattr(self, slot)
            cls = cls.__bases__[0]
        for prop in YumJob.__slots__:
            kwargs.pop(prop, None)
        return kwargs

    def start(self):
        """Modify the state of job to RUNNING."""
        if self.started:
            raise errors.InvalidJobState("can not start already started job")
        self.started = time.time()
        self.last_change = self.started

    def finish(self, result, data=None):
        """
        Modify the state of job to one of {COMPLETED, EXCEPTION, TERMINATED}.
        Depending on result parameter.
        """
        if not self.started and result != self.RESULT_TERMINATED:
            raise errors.InvalidJobState("can not finish not started job")
        self.finished = time.time()
        if result == self.RESULT_TERMINATED:
            self.started = self.finished
        self.result = result
        self.result_data = data
        self.last_change = self.finished

    def update(self, **kwargs):
        """Change job's properties."""
        change = False
        for key, value in kwargs.items():
            if getattr(self, key) != value:
                setattr(self, key, value)
                change = True
        if change is True:
            self.last_change = time.time()

    def __eq__(self, other):
        return self.__class__ is other.__class__ and self.jobid == other.jobid

    def __ne__(self, other):
        return (  self.__class__ is not other.__class__
               or self.jobid != other.jobid)

    def __lt__(self, other):
        """
        JobControl jobs have the highest priority.
        """
        return (   other is not None    # terminating command
               and (  (  isinstance(self, YumJobControl)
                      and not isinstance(other, YumJobControl))
                   or (  self.priority < other.priority
                      or (   self.priority == other.priority
                         and (  self.jobid < other.jobid
                             or (   self.jobid == other.jobid
                                and (self.created < other.created)))))))

    def __cmp__(self, other):
        if other is None:   # terminating command
            return 1
        if (   isinstance(self, YumJobControl)
           and not isinstance(other, YumJobControl)):
            return -1
        if (   not isinstance(self, YumJobControl)
           and isinstance(other, YumJobControl)):
            return 1
        if self.priority < other.priority:
            return -1
        if self.priority > other.priority:
            return 1
        if self.jobid < other.jobid:
            return -1
        if self.jobid > other.jobid:
            return 1
        if self.created < other.created:
            return -1
        if self.created > other.created:
            return 1
        return 0

    def __str__(self):
        return "%s(id=%d,p=%d)" % (
                self.__class__.__name__, self.jobid, self.priority)

    def __getstate__(self):
        ret = self.job_kwargs
        for prop in self.handle_ignore_job_props():
            ret[prop] = getattr(self, prop)
        return ret

    def __setstate__(self, state):
        for k, value in state.items():
            setattr(self, k, value)

class YumAsyncJob(YumJob):              #pylint: disable=R0903
    """
    Base class for jobs, that support asynchronnous execution.
    No reply is sent upon job completition or error. The results are
    kept on server.
    """
    __slots__ = ( 'async'
                , 'delete_on_completion'
                , 'time_before_removal'
                , 'metadata')

    @classmethod
    def handle_ignore_job_props(cls):
        return YumJob.handle_ignore_job_props().union(YumAsyncJob.__slots__)

    def __init__(self, priority=None, async=False, metadata=None):
        YumJob.__init__(self, priority)
        self.async = bool(async)
        self.delete_on_completion = True
        self.time_before_removal = util.Configuration.get_instance().get_safe(
                'Jobs', 'DefaultTimeBeforeRemoval', float)
        if metadata is None and self.async is True:
            metadata = {}
        self.metadata = metadata

    def __str__(self):
        return "%s(id=%d,p=%d%s%s)" % (
                self.__class__.__name__, self.jobid,
                self.priority,
                ',async' if self.async else '',
                (',name="%s"'%self.metadata['name'])
                    if self.metadata and 'name' in self.metadata else '')

    def update(self, **kwargs):
        if 'metadata' in kwargs:
            self.metadata.update(kwargs.pop('metadata'))
        return YumJob.update(self, **kwargs)

# *****************************************************************************
# Job control funtions
# *****************************************************************************
class YumJobControl(YumJob):            #pylint: disable=R0903
    """Base class for any job used for asynchronous jobs management."""
    pass

class YumJobGetList(YumJobControl):     #pylint: disable=R0903
    """Request for obtaining list of all asynchronous jobs."""
    pass

class YumJobOnJob(YumJobControl):
    """
    Base class for any control job acting upon particular asynchronous job.
    """
    __slots__ = ('target', )
    def __init__(self, target):
        YumJobControl.__init__(self)
        if not isinstance(target, (int, long)):
            raise TypeError("target must be an integer")
        self.target = target

class YumJobGet(YumJobOnJob):         #pylint: disable=R0903
    """Get job object by its id."""
    pass

class YumJobGetByName(YumJobOnJob):   #pylint: disable=R0903
    """Get job object by its name property."""
    def __init__(self, name):
        YumJobOnJob.__init__(self, -1)
        self.target = name

class YumJobSetPriority(YumJobOnJob): #pylint: disable=R0903
    """Change priority of job."""
    __slots__ = ('new_priority', )

    def __init__(self, target, priority):
        YumJobOnJob.__init__(self, target)
        self.new_priority = priority

class YumJobUpdate(YumJobOnJob):      #pylint: disable=R0903
    """
    .. _YumJobUpdate:

    Update job's metadata. There are some forbidden properties, that
    can not be changed in this way. Those are all affecting job's priority
    and its scheduling for deletion. Plus any that store job's state.
    All forbidden properties are listed in ``FORBIDDEN_PROPERTIES``.
    """
    __slots__ = ('data', )
    FORBIDDEN_PROPERTIES = (
        'async', 'jobid', 'created', 'started', 'priority', 'finished',
        'delete_on_completion', 'time_before_removal', 'last_change')

    def __init__(self, target, **kwargs):
        YumJobOnJob.__init__(self, target)
        assert not set.intersection(
                set(YumJobUpdate.FORBIDDEN_PROPERTIES), set(kwargs))
        self.data = kwargs

class YumJobReschedule(YumJobOnJob):  #pylint: disable=R0903
    """Change the schedule of job's deletion."""
    __slots__ = ('delete_on_completion', 'time_before_removal')
    def __init__(self, target, delete_on_completion, time_before_removal):
        YumJobOnJob.__init__(self, target)
        if not isinstance(time_before_removal, (int, long, float)):
            raise TypeError("time_before_removal must be float")
        self.delete_on_completion = bool(delete_on_completion)
        self.time_before_removal = time_before_removal

class YumJobDelete(YumJobOnJob):      #pylint: disable=R0903
    """Delete job - can only be called on finished job."""
    pass

class YumJobTerminate(YumJobOnJob):   #pylint: disable=R0903
    """
    Can only be called on not yet started job.
    Running job can not be terminated.
    """
    pass

# *****************************************************************************
# Yum API functions
# *****************************************************************************
class YumLock(YumJob):              #pylint: disable=R0903
    """
    This job shall be sent when session starts. Session consists of several
    requests to ``YumWorker`` during which no other application shall
    interfere. Exclusive access to yum database is enforced with this job. When
    the session is over, YumUnlock job needs to be sent to YumWorker.
    """
    pass

class YumUnlock(YumJob):            #pylint: disable=R0903
    """
    This job shall be sent when session ends.
    """
    pass

class YumShutDown(YumJob):
    """
    Last job executed before provider terminates. It shuts down *YumWorker*,
    *SessionManager* and *JobManager*.
    """
    pass

class YumGetPackageList(YumJob):  #pylint: disable=R0903
    """
    Job requesing a list of packages.
    Arguments:
      kind - supported values are in SUPPORTED_KINDS tuple
        * installed lists all installed packages; more packages with
          the same name can be installed varying in their architecture
        * avail_notinst lists all available, not installed packages;
          allow_duplicates must be True to include older packages (but still
          available)
        * avail_reinst lists all installed packages, that are available;
          package can be installed, but not available anymore due to updates
          of repository, where only the newest packages are kept
        * available lists a union of avail_notinst and avail_reinst
        * all lists union of installed and avail_notinst

      allow_duplicates - whether multiple packages can be present
        in result for single (name, arch) of package differing
        in their version

      sort - whether to sort packages by nevra

      include_repos - either a string passable to RepoStorage.enableRepo()
        or a list of repository names, that will be temporared enabled before
        listing packages; this is applied after disabling of repositories

      exclude_repos - either a string passable to RepoStorage.disableRepo()
        or a list of repository names, that will be temporared disabled before
        listing packages; this is applied before enabling of repositories

    Worker replies with [pkg1, pkg2, ...].
    """
    __slots__ = ('kind', 'allow_duplicates', 'sort', 'include_repos',
            'exclude_repos')

    SUPPORTED_KINDS = ( 'installed', 'available', 'avail_reinst'
                      , 'avail_notinst', 'all')

    def __init__(self, kind, allow_duplicates, sort=False,
            include_repos=None, exclude_repos=None):
        YumJob.__init__(self)
        if not isinstance(kind, basestring):
            raise TypeError("kind must be a string")
        if not kind in self.SUPPORTED_KINDS:
            raise ValueError("kind must be one of {%s}" %
                    ", ".join(self.SUPPORTED_KINDS))
        for arg in ('include_repos', 'exclude_repos'):
            val = locals()[arg]
            if (   not val is None
               and not isinstance(arg, (tuple, list, basestring))):
                raise TypeError("expected list or string for %s" % arg)
        self.kind = kind
        self.allow_duplicates = bool(allow_duplicates)
        self.sort = bool(sort)
        self.include_repos = include_repos
        self.exclude_repos = exclude_repos

class YumFilterPackages(YumGetPackageList):   #pylint: disable=R0903
    """
    Job similar to YumGetPackageList, but allowing to specify
    filter on packages.
    Arguments (plus those in YumGetPackageList):
      name, epoch, version, release, arch, nevra, envra, evra,
      repoid, exact_match

    Argument ``exact_match`` makes the name property being compared byte
    by byte. If ``False``, all packages containing ``name``'s value either
    in name of summary will match.

    Some of those are redundant, but filtering is optimized for
    speed, so supplying all of them won't affect performance.

    Worker replies with [pkg1, pkg2, ...].
    """
    __slots__ = (
        'name', 'epoch', 'version', 'release', 'arch',
        'nevra', 'envra', 'evra', 'repoid', 'exact_match')

    def __init__(self, kind, allow_duplicates,
            exact_match=True,
            sort=False, include_repos=None, exclude_repos=None,
            name=None, epoch=None, version=None,
            release=None, arch=None,
            nevra=None, evra=None,
            envra=None,
            repoid=None):
        if nevra is not None and not util.RE_NEVRA.match(nevra):
            raise ValueError("Invalid nevra: %s" % nevra)
        if evra is not None and not util.RE_EVRA.match(evra):
            raise ValueError("Invalid evra: %s" % evra)
        if envra is not None and not util.RE_ENVRA.match(evra):
            raise ValueError("Invalid envra: %s" % envra)
        YumGetPackageList.__init__(self, kind, allow_duplicates, sort,
                include_repos=include_repos, exclude_repos=exclude_repos)
        self.name = name
        self.epoch = None if epoch is None else str(epoch)
        self.version = version
        self.release = release
        self.arch = arch
        self.nevra = nevra
        self.evra = evra
        self.envra = envra
        self.repoid = repoid
        self.exact_match = bool(exact_match)

class YumSpecificPackageJob(YumAsyncJob):  #pylint: disable=R0903
    """
    Abstract job taking instance of yumdb.PackageInfo as argument or
    package's nevra.
    Arguments:
      pkg - plays different role depending on job subclass;
            can also be a nevra
    """
    __slots__ = ('pkg', )
    def __init__(self, pkg, async=False, metadata=None):
        if isinstance(pkg, basestring):
            if not util.RE_NEVRA_OPT_EPOCH.match(pkg):
                raise errors.InvalidNevra('not a valid nevra "%s"' % pkg)
        elif not isinstance(pkg, PackageInfo):
            raise TypeError("pkg must be either string or instance"
                " of PackageInfo")
        YumAsyncJob.__init__(self, async=async, metadata=metadata)
        self.pkg = pkg

class YumInstallPackage(YumSpecificPackageJob):   #pylint: disable=R0903
    """
    Job requesting installation of specific package.
    pkg argument should be available.
    Arguments:
      pkg - same as in YumSpecificPackageJob
      force is a boolean saying:
        True  -> reinstall the package if it's already installed
        False -> fail if the package is already installed

    Worker replies with new instance of package.
    """
    __slots__ = ('force', )
    def __init__(self, pkg, async=False, force=False, metadata=None):
        YumSpecificPackageJob.__init__(
                self, pkg, async=async, metadata=metadata)
        self.force = bool(force)

class YumRemovePackage(YumSpecificPackageJob):    #pylint: disable=R0903
    """
    Job requesting removal of specific package.
    pkg argument should be installed.
    """
    pass

class YumUpdateToPackage(YumSpecificPackageJob):  #pylint: disable=R0903
    """
    Job requesting update to provided specific package.
    Package is updated to epoch, version and release of this
    provided available package.

    Worker replies with new instance of package.
    """
    pass

class YumUpdatePackage(YumSpecificPackageJob):    #pylint: disable=R0903
    """
    Job requesting update of package, optionally reducing possible
    candidate packages to ones with specific evr.
    Arguments:
      to_epoch, to_version, to_release
      force is a boolean, that has meaning only when update_only is False:
        True  -> reinstall the package if it's already installed
        False -> fail if the package is already installed

    The arguments more given, the more complete filter of candidates.

    Worker replies with new instance of package.
    """
    __slots__ = ('to_epoch', 'to_version', 'to_release', 'force')

    def __init__(self, pkg, async=False,
            to_epoch=None, to_version=None, to_release=None, force=False,
            metadata=None):
        if not isinstance(pkg, (basestring, PackageInfo)):
            raise TypeError("pkg must be either instance of yumdb.PackageInfo"
                    " or nevra string")
        YumSpecificPackageJob.__init__(
                self, pkg, async=async, metadata=metadata)
        self.to_epoch = to_epoch
        self.to_version = to_version
        self.to_release = to_release
        self.force = bool(force)

class YumCheckPackage(YumSpecificPackageJob):       #pylint: disable=R0903
    """
    Request verification information for instaled package and its files.

    Arguments:
        pkg - either instance of PackageInfo or nevra string.
              In latter case it will be replaced for YumWorker with instance
              of PackageInfo.

    Worker replies with ``(pkg_info, pkg_check)``.
      where:
        ``pkg_info``  - is instance of PackageInfo
        ``pkg_check`` - new instance of yumdb.PackageCheck
    """
    def __init__(self, pkg, async=False, metadata=None):
        YumSpecificPackageJob.__init__(self, pkg, async=async,
                metadata=metadata)
        if isinstance(pkg, PackageInfo) and not pkg.installed:
            raise ValueError("package must be installed to check it")

class YumCheckPackageFile(YumCheckPackage):     #pylint: disable=R0903
    """
    Request verification information for particular file of installed
    package.

    Worker replies with ``(pkg_info, pkg_check)``.
      where:
        ``pkg_info``  - is instance of PackageInfo
        ``pkg_check`` - new instance of yumdb.PackageCheck containing only
                        requested file.
    """
    __slots__ = ('file_name', )
    def __init__(self, pkg, file_name, *args, **kwargs):
        YumCheckPackage.__init__(self, pkg, *args, **kwargs)
        if not isinstance(file_name, basestring):
            raise TypeError("file_name must be string")
        self.file_name = file_name

class YumInstallPackageFromURI(YumAsyncJob):     #pylint: disable=R0903
    """
    Job requesting installation of specific package from URI.
    Arguments:
      uri is either a path to rpm package on local filesystem or url
        of rpm stored on remote host
      update_only is a boolean:
        True  -> install the package only if the older version is installed
        False -> install the package if it's not already installed
      force is a boolean, that has meaning only when update_only is False:
        True  -> reinstall the package if it's already installed
        False -> fail if the package is already installed

    Worker replies with new instance of package.
    """
    __slots__ = ('uri', 'update_only', "force")
    def __init__(self, uri, async=False, update_only=False, force=False,
            metadata=None):
        if not isinstance(uri, basestring):
            raise TypeError("uri must be a string")
        if uri.startswith('file://'):
            uri = uri[len('file://'):]
        if not yum.misc.re_remote_url(uri) and not os.path.exists(uri):
            raise errors.InvalidURI(uri)
        YumAsyncJob.__init__(self, async=async, metadata=metadata)
        self.uri = uri
        self.update_only = bool(update_only)
        self.force = bool(force)

class YumGetRepositoryList(YumJob):                 #pylint: disable=R0903
    """
    Job requesing a list of repositories.
    Arguments:
      kind - supported values are in SUPPORTED_KINDS tuple

    Worker replies with [repo1, repo2, ...].
    """
    __slots__ = ('kind', )

    SUPPORTED_KINDS = ('all', 'enabled', 'disabled')

    def __init__(self, kind):
        YumJob.__init__(self)
        if not isinstance(kind, basestring):
            raise TypeError("kind must be a string")
        if not kind in self.SUPPORTED_KINDS:
            raise ValueError("kind must be one of {%s}" %
                    ", ".join(self.SUPPORTED_KINDS))
        self.kind = kind

class YumFilterRepositories(YumGetRepositoryList):  #pylint: disable=R0903
    """
    Job similar to YumGetRepositoryList, but allowing to specify
    filter on packages.
    Arguments (plus those in YumGetRepositoryList):
      name, gpg_check, repo_gpg_check

    Some of those are redundant, but filtering is optimized for
    speed, so supplying all of them won't affect performance.

    Worker replies with [repo1, repo2, ...].
    """
    __slots__ = ('repoid', 'gpg_check', 'repo_gpg_check')

    def __init__(self, kind,
            repoid=None, gpg_check=None, repo_gpg_check=None):
        YumGetRepositoryList.__init__(self, kind)
        self.repoid = repoid
        self.gpg_check = None if gpg_check is None else bool(gpg_check)
        self.repo_gpg_check = (
                None if repo_gpg_check is None else bool(repo_gpg_check))

class YumSpecificRepositoryJob(YumJob):             #pylint: disable=R0903
    """
    Abstract job taking instance of yumdb.Repository as argument.
    Arguments:
      repo - (``Repository`` or ``str``) plays different role depending
             on job subclass
    """
    __slots__ = ('repo', )
    def __init__(self, repo):
        if not isinstance(repo, (Repository, basestring)):
            raise TypeError("repoid must be either instance of"
                " yumdb.Repository or string")
        YumJob.__init__(self)
        self.repo = repo

class YumSetRepositoryEnabled(YumSpecificRepositoryJob):#pylint: disable=R0903
    """
    Job allowing to enable or disable repository.
    Arguments:
      enable - (``boolean``) representing next state
    """
    __slots__ = ('enable', )
    def __init__(self, repo, enable):
        YumSpecificRepositoryJob.__init__(self, repo)
        self.enable = bool(enable)

def log_reply_error(job, reply):
    """
    Raises an exception in case of error occured in worker process
    while processing job.
    """
    if isinstance(reply, (int, long)):
        # asynchronous job
        return
    logger = logging.getLogger(__name__)
    if not isinstance(reply, YumJob):
        raise TypeError('expected instance of YumJob for reply, not "%s"'
                % reply.__class__.__name__)
    if reply.result == YumJob.RESULT_ERROR:
        logger.error("%s failed with error %s: %s",
                job, reply.result_data[0].__name__, str(reply.result_data[1]))
        logger.trace_warn("%s exception traceback:\n%s%s: %s",
                job, "".join(reply.result_data[2]),
                reply.result_data[0].__name__, str(reply.result_data[1]))
        reply.result_data[1].tb_printed = True
        raise reply.result_data[1]
    elif reply.result == YumJob.RESULT_TERMINATED:
        logger.warn('%s terminated', job)
    else:
        logger.debug('%s completed with success', job)

