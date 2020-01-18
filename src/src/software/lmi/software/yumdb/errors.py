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
Exceptions raisable by YumWorker.
"""

class YumDBError(Exception):
    """Base class for all errors under yumdb package."""

class DatabaseLockError(YumDBError):
    """Raised, when the yum database can not be locked."""
    pass

class TransactionError(YumDBError):
    """Base exception representing yum transaction processing error."""
    pass
class TransactionBuildFailed(TransactionError):
    """Raised, when transaction building fails."""
    pass
class PackageAlreadyInstalled(TransactionError):
    """Raised, when trying to install already installed package."""
    def __init__(self, pkg):
        TransactionError.__init__(self,
                'Package "%s" is already installed.' % pkg)
class PackageOpenError(TransactionError):
    """Raised, when trying to open package obtained from URI."""
    def __init__(self, pkg, msg):
        TransactionError.__init__(self,
                'Failed to open package "%s": %s' % (pkg, msg))
class TransactionExecutionFailed(TransactionError):
    """Raised, when YumBase.doTransaction() method fails."""
    pass

class TerminatingError(TransactionError):
    """Raised when job can not be completed due to provider's termination."""
    pass

class PackageError(YumDBError):
    """Generic exception for error concerning package handling."""
    pass
class PackageNotFound(PackageError):
    """Raised, when requested package could not be found."""
    pass
class PackageNotInstalled(PackageError):
    """Raised, when requested package is not installed for desired action."""
    def __init__(self, pkg):
        PackageError.__init__(self, 'Package "%s" is not installed.' % pkg)
class FileNotFound(PackageError):
    """
    Raised, when requesting check on file that does not belong to
    particular package.
    """
    pass

class RepositoryError(YumDBError):
    """Generic exception for error concerning repository handling."""
    pass
class RepositoryNotFound(RepositoryError):
    """Raised, when requested repository cound not be found."""
    def __init__(self, repoid):
        RepositoryError.__init__(self, "No such repository: %s" % repoid)
class RepositoryChangeError(RepositoryError):
    """Raised, when modification of repository failed."""
    pass

class JobError(YumDBError):
    """Generic exception for job handling."""
    pass
class UnknownJob(JobError):
    """Raised, when no handler is available for given job on worker."""
    pass
class InvalidURI(JobError):
    """Raised, when passed uri is not a valid one."""
    def __init__(self, uri):
        JobError.__init__(self, "Invalid uri: \"%s\"" % uri)
class InvalidNevra(JobError):
    """Raised when trying to instantiate job with invalid nevra string."""
    pass
class JobControlError(JobError):
    """Generic exception for management of asynchronous jobs."""
    pass
class JobNotFound(JobControlError):
    """Raised upon request for not existing asynchronous job."""
    def __init__(self, target):
        JobControlError.__init__(self, "job %s could not be found" % target)
class InvalidJobState(JobControlError):
    """
    Raised when requested operation can not be executed on job in
    its current state.
    """
    pass

class IndicationError(YumDBError):
    """Generic error for indication handling."""
    pass

