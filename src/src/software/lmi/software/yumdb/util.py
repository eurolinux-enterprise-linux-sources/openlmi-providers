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
Common utilities meant to be used only be ``yumdb`` subpackage.
"""

import logging
import yum

from lmi.providers import cmpi_logging
from lmi.software.util import Configuration

DEBUG_LOGGING_CONFIG = {
    "version" : 1,
    'disable_existing_loggers' : True,
    "formatters": {
        # this is a message format for logging function/method calls
        # it's manually set up in YumWorker's init method
        "default": {
            "()": "lmi.providers.cmpi_logging.DispatchingFormatter",
            "formatters" : {
                "lmi.providers.cmpi_logging.trace_function_or_method":
                    "%(asctime)s %(levelname)s:%(message)s"
                },
            "default" : "%(asctime)s %(levelname)s:%(module)s:"
                        "%(funcName)s:%(lineno)d - %(message)s"
            },
        },
    "handlers": {
        "file" : {
            "class" : "logging.FileHandler",
            "filename" : "/var/tmp/YumWorker.log",
            "level" : "DEBUG",
            "formatter": "default",
            },
        },
    "root": {
        "level": "DEBUG",
        "handlers" : ["file"]
        },
}

DISABLED_LOGGING_CONFIG = {
    'version' : 1,
    'disable_existing_loggers' : True,
    'handlers': {
        'null' : {
            'class': 'logging.FileHandler',
            'level': 'CRITICAL',
            'filename': '/dev/null'
            }
        },
    'root' : {
        'level': 'CRITICAL',
        'handlers' : ['null'],
        }
}

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
    if cp.has_option('YumWorkerLog', 'file_config'):
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
                defaults = DEBUG_LOGGING_CONFIG.copy()
                defaults["handlers"]["file"]["filename"] = out
                level = config.get_safe('YumWorkerLog', 'Level')
                level = cmpi_logging.LOGGING_LEVELS[level.lower()]
                defaults["handlers"]["file"]["level"] = level
                defaults["root"]["level"] = level
                logging.config.dictConfig(defaults)
                logging_setup = True
                # re-enable function tracing logger which has been disabled by
                # dictConfig() call
                logging.getLogger(cmpi_logging.__name__
                        + '.trace_function_or_method').disabled = False
            except Exception:
                pass
    if logging_setup is False:
        # disable logging completely
        logging.config.dictConfig(DISABLED_LOGGING_CONFIG)

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
