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
Common utilities meant to be used only by ``yumdb`` subpackage.
"""

import logging
import yum

from lmi.providers import cmpi_logging
from lmi.software.util import Configuration

def disable_existing_loggers():
    """
    Disable all existing loggers and remove logging handlers.
    """
    for logger in logging.root.manager.loggerDict.values():
        logger.disabled = 1
        logger.handlers = []
    logger = logging.getLogger()
    logger.disabled = 1
    logger.handlers = []

def disable_logging():
    """
    Send everything to ``/dev/null``.
    """
    disable_existing_loggers()
    handler = logging.FileHandler('/dev/null')
    handler.setLevel(logging.CRITICAL)
    root = logging.getLogger('root')
    root.disabled = 0
    root.addHandler(handler)
    root.setLevel(logging.CRITICAL)

def setup_logging_to_file(output_file, level=logging.DEBUG):
    """
    Log everything to file.

    :param str output_file: Path to a desired log file.
    :param int level: Lowest level of logging messages to handle.
    """
    disable_existing_loggers()
    formatter = cmpi_logging.DispatchingFormatter(
        formatters={
            "lmi.providers.cmpi_logging.trace_function_or_method":
                    "%(asctime)s %(levelname)s:%(message)s"
                },
        default="%(asctime)s %(levelname)s:%(module)s:"
                        "%(funcName)s:%(lineno)d - %(message)s")
    handler = logging.FileHandler(output_file)
    handler.setLevel(level)
    handler.setFormatter(formatter)
    root = logging.getLogger()
    root.setLevel(level)
    root.addHandler(handler)
    # re-enable function tracing logger which has been disabled by
    # disable_existing_loggers() call
    logger = logging.getLogger(
            cmpi_logging.__name__ + '.trace_function_or_method')
    logger.disabled = 0

def setup_logging():
    """
    This is meant to be used by ``YumWorker`` process to setup logging
    independent of what providers are using. Unfortunately ``YumWorker``
    can not use the same facilities as the rest of program, because
    logging is done through *broker*.
    """
    config = Configuration.get_instance()
    cp = config.config
    logging_setup = False
    if cp.has_option('YumWorkerLog', 'FileConfig'):
        try:
            file_path = config.file_path('YumWorkerLog', 'FileConfig')
            logging.config.fileConfig(file_path)
            logging_setup = True
        except Exception:
            pass
    if logging_setup is False:
        out = config.get_safe('YumWorkerLog', 'OutputFile')
        if out is not None:
            try:
                level = config.get_safe('YumWorkerLog', 'Level')
                level = cmpi_logging.LOGGING_LEVELS[level.lower()]
                setup_logging_to_file(out, level)
                logging_setup = True
            except Exception:
                pass
    if logging_setup is False:
        disable_logging()

def is_pkg_installed(pkg, rpmdb):
    """
    Returns ``True`` if the package is installed.

    :param pkg: Yum package object.
    :type pkg: :py:class:`yum.packages.PackageObject`
    :param rpmdb: Installed package sack.
    :type rpmdb: :py:class:`yum.rpmsack.RPMDBPackageSack`
    :returns: Whether the package is installed or not.
    :rtype: boolean
    """
    if not isinstance(pkg, yum.packages.PackageObject):
        raise TypeError("pkg must be a PackageObject")
    return (  isinstance(pkg, yum.rpmsack.RPMInstalledPackage)
           or pkg.pkgtup in rpmdb._tup2pkg)
