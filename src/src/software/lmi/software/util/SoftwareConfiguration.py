# Copyright (C) 2012 Red Hat, Inc.  All rights reserved.
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
# -*- coding: utf-8 -*-
"""
Module for SoftwareConfiguration class.

SoftwareConfiguration
---------------------

.. autoclass:: SoftwareConfiguration
    :members:

"""

from lmi.base.BaseConfiguration import BaseConfiguration

class SoftwareConfiguration(BaseConfiguration):
    """
        Configuration class specific to software providers.
        OpenLMI configuration file should reside in
        /etc/openlmi/software/software.conf.
    """

    @classmethod
    def provider_prefix(cls):
        return "software"

    @classmethod
    def default_options(cls):
        """ :rtype: (``dict``) Dictionary of default values. """
        defaults = BaseConfiguration.default_options().copy()
        # Maximum time in seconds to wait for a job to accomplish.
        # If timeout expires, spawned process is checked (it might
        # be possibly killed) and is respawned in case it's dead.
        defaults["WaitCompleteTimeout" ] = "30"
        # Number of seconds to wait before next try to lock yum package
        # database. This applies, when yum database is locked by another
        # process.
        defaults["LockWaitInterval"] = "0.5"
        # Number of seconds to keep package cache in memory after the last use
        # (caused by user request). Package cache takes up a lot of memory.
        defaults["FreeDatabaseTimeout"] = "60"
        # Minimum time to keep asynchronous job in cache after completion. In
        # seconds.
        defaults["MinimumTimeBeforeRemoval"] = "10"
        # Number of seconds to keep asynchronous job after its completion.
        # It applies only to jobs with DeleteOnCompletion == True.
        defaults["DefaultTimeBeforeRemoval"] = "300"
        # Default asynchronous job priority.
        defaults["DefaultPriority"] = "10"
        return defaults

    @classmethod
    def mandatory_sections(cls):
        return list(BaseConfiguration.mandatory_sections()) + [
            'Yum', 'Jobs', 'YumWorkerLog']

