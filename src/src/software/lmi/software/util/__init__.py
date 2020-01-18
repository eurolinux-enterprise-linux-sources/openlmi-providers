# -*- encoding: utf-8 -*-
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

"""Common utilities for LMI_Software* providers
"""

import platform
import pywbem
import re
import signal

from lmi.providers import ComputerSystem
from lmi.software.util.SoftwareConfiguration \
    import SoftwareConfiguration as Configuration

RE_INSTANCE_ID = re.compile(r'LMI:(?P<clsname>[a-z][a-z_0-9]+):(?P<id>\d+)',
    re.IGNORECASE)

RE_EVRA  = re.compile(
    r'^(?P<epoch>\d+):(?P<version>[\w.+{}]+)-(?P<release>[\w.+{}]+)'
    r'\.(?P<arch>[^.]+)$')
RE_NEVRA = re.compile(
    r'^(?P<name>.+)-(?P<evra>(?P<epoch>\d+):(?P<version>[\w.+{}]+)'
    r'-(?P<release>[\w.+{}]+)\.(?P<arch>[^.]+))$')
RE_NEVRA_OPT_EPOCH = re.compile(
    r'^(?P<name>.+)-(?P<evra>(?:(?P<epoch>\d+):)?(?P<version>[\w.+{}]+)'
    r'-(?P<release>[\w.+{}]+)\.(?P<arch>[^.]+))$')
RE_ENVRA = re.compile(
    r'^(?P<epoch>\d+):(?P<name>.+)-(?P<evra>(?P<version>[\w.+{}]+)'
    r'-(?P<release>[\w.+{}]+)\.(?P<arch>[^.]+))$')

def _get_distname():
    """
    @return name of linux distribution
    """
    if hasattr(platform, 'linux_distribution'):
        return platform.linux_distribution(
                full_distribution_name=False)[0].lower()
    else:
        return platform.dist()[0].lower()


def get_target_operating_system():
    """
    @return (val, text).
    Where val is a number from ValueMap of TargetOperatingSystem property
    of CIM_SoftwareElement class and text is its testual representation.
    """

    system = platform.system()
    if system.lower() == 'linux':
        try:
            val, dist = \
            { 'redhat'   : (79, 'RedHat Enterprise Linux')
            , 'suse'     : (81, 'SUSE')
            , 'mandriva' : (88, 'Mandriva')
            , 'ubuntu'   : (93, 'Ubuntu')
            , 'debian'   : (95, 'Debian')
            }[_get_distname()]
        except KeyError:
            linrel = platform.uname()[2]
            if linrel.startswith('2.4'):
                val, dist = (97, 'Linux 2.4.x')
            elif linrel.startswith('2.6'):
                val, dist = (99, 'Linux 2.6.x')
            else:
                return (36, 'LINUX') # no check for x86_64
        if platform.machine() == 'x86_64':
            val  += 1
            dist += ' 64-Bit'
        return (val, dist)
    elif system.lower() in ('macosx', 'darwin'):
        return (2, 'MACOS')
    # elif system.lower() == 'windows': # no general value
    else:
        return (0, 'Unknown')

def check_target_operating_system(system):
    """
    @return if param system matches current target operating system
    """
    if isinstance(system, basestring):
        system = int(system)
    if not isinstance(system, (int, long)):
        raise TypeError("system must be either string or integer, not %s" %
                system.__class__.__name__)
    tos = get_target_operating_system()
    if system == tos:
        return True
    if system == 36: # linux
        if platform.system().lower() == "linux":
            return True
    if (   system >= 97 and system <= 100 # linux 2.x.x
       and platform.uname()[2].startswith('2.4' if system < 99 else '2.6')
       # check machine
       and (  bool(platform.machine().endswith('64'))
           == bool(not (system % 2)))):
        return True
    return False

def nevra2filter(nevra):
    """
    Takes either regexp match object resulting from RE_NEVRA match or
    a nevra string.
    @return dictionary with package filter key-value pairs made from nevra
    """
    if isinstance(nevra, basestring):
        match = RE_NEVRA_OPT_EPOCH.match(nevra)
    elif nevra.__class__.__name__.lower() == "sre_match":
        match = nevra
    else:
        raise TypeError("nevra must be either string or regexp match object")
    epoch = match.group("epoch")
    if not epoch or match.group("epoch").lower() == "(none)":
        epoch = "0"
    return { "name"    : match.group("name")
           , "epoch"   : epoch
           , "version" : match.group("version")
           , "release" : match.group("release")
           , "arch"    : match.group("arch")
           }

def make_nevra(name, epoch, version, release, arch, with_epoch='NOT_ZERO'):
    """
    @param with_epoch may be one of:
        "NOT_ZERO" - include epoch only if it's not zero
        "ALWAYS"   - include epoch always
        "NEVER"    - do not include epoch at all
    """
    estr = ''
    if with_epoch.lower() == "always":
        estr = epoch
    elif with_epoch.lower() == "not_zero":
        if epoch != "0":
            estr = epoch
    if len(estr):
        estr += ":"
    return "%s-%s%s-%s.%s" % (name, estr, version, release, arch)

def pkg2nevra(pkg, with_epoch='NOT_ZERO'):
    """
    @return nevra string made of pkg
    """
    return make_nevra(pkg.name, pkg.epoch, pkg.version,
            pkg.release, pkg.arch, with_epoch)

def get_signal_name(signal_num):
    """
    @return name of signal for signal_num argument
    """
    if not isinstance(signal_num, (int, long)):
        raise TypeError("signal_num must be an integer")
    try:
        return dict((v, k) for k, v in signal.__dict__.items())[signal_num]
    except KeyError:
        return "UNKNOWN_SIGNAL(%d)" % signal_num

def new_instance_name(class_name, namespace=None, **kwargs):
    """
    Create new CIMInstanceName with provided attributes.

    :param class_name: (``str``) Name of CIM Class that will be instantiated.
    :param namespace: (``str``) Target CIM namespace.
    :param kwargs: (``dict``) Key properties with associated values.
    :rtype: (``pywbem.CIMInstanceName``) New object path.
    """
    keybindings = pywbem.NocaseDict(kwargs)
    if namespace is None:
        namespace = Configuration.get_instance().namespace
    try:
        host = ComputerSystem.get_system_name()
    except ComputerSystem.UninitializedError:
        host = None
    return pywbem.CIMInstanceName(
            class_name,
            host=host,
            namespace=namespace,
            keybindings=keybindings)
