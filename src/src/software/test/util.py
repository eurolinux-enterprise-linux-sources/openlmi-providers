#!/usr/bin/python
# -*- Coding:utf-8 -*-
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
# Lesser General Public License for more details. #
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Authors: Radek Novacek <rnovacek@redhat.com>
# Authors: Michal Minar <miminar@redhat.com>
# Authors: Jan Grec <jgrec@redhat.com>

"""
Common test utilities.
"""

import datetime
import platform
import re
import os
import subprocess
import sys

from lmi.test import wbem

RE_NEVRA = re.compile(
    r'^(?P<name>[^\s]+)-(?P<evra>(?P<epoch>\d+):(?P<ver>[^-\s]+)'
    r'-(?P<rel>[^\s]+)\.(?P<arch>[^.\s]+))$')
RE_NEVRA_OPT_EPOCH = re.compile(
    r'^(?P<name>[^\s]+)-(?P<evra>((?P<epoch>\d+):)?(?P<ver>[^-\s]+)'
    r'-(?P<rel>[^\s]+)\.(?P<arch>[^.\s]+))$')
RE_ENVRA = re.compile(
    r'^(?P<epoch>\d+|\(none\)):(?P<name>[^\s]+)-(?P<ver>[^-\s]+)'
    r'-(?P<rel>[^\s]+)\.(?P<arch>[^.\s]+)$')
RE_REPO = re.compile(
        r'(?:^\*?)(?P<name>[^\s/]+\b)(?!\s+id)', re.MULTILINE | re.IGNORECASE)

DEV_NULL = open('/dev/null', 'w')
# Whether to use `pkcon` utility to enable/disable repositories
USE_PKCON = False

# Conversion function from string obtained from environment to boolean.
_STR_TO_BOOL = lambda a: a.lower() in ('1', 'yes', 'on', 'true')
_RE_ENV_VAR_PREFIX = re.compile(r'^LMI_((CIMOM|SOFTWARE)_)?', re.IGNORECASE)

# Entries are in form:
#   (env variable name, (conversion function, default value))
# where *conversion function* is invoked on value obtained from environment and
# *default* is already converted value that will be returned for variable
# missing in environment.
ENV_VARS = {
        # LMI_ variables
        'LMI_RUN_DANGEROUS'          : (_STR_TO_BOOL, False),
        'LMI_RUN_TEDIOUS'            : (_STR_TO_BOOL, False),

        # LMI_CIMOM_ variables
        'LMI_CIMOM_BROKER'           : (str, "tog-pegasus"),
        'LMI_CIMOM_URL'              : (str, "http://localhost:5988"),

        # LMI_SOFTWARE_ variables
        'LMI_SOFTWARE_BACKEND'       : (str, 'yum'),
        'LMI_SOFTWARE_CLEANUP_CACHE' : (_STR_TO_BOOL, True),
        'LMI_SOFTWARE_DB_CACHE'      : (str, None),
        'LMI_SOFTWARE_YUM_REPOS_DIR' : (str, '/etc/yum.repos.d'),
}

def get_env(var, *args):
    """
    Get environment variable converted to expected type.

    :param str var: Variable name with or without common prefixes beginning
        with `LMI_`. Those are `LMI_`, `LMI_CIMOM_`, `LMI_SOFTWARE_`. This
        string is converted to upper case before the environment is searched.
    :param default: Optional default value returned if *var* is not present in
        environment. This takes precedence over default values provided by
        `ENV_VARS`.
    :returns: Value of *var* from environment. *default* value if not found.
        Default value from `ENV_VARS` dictionary if not provided. `''` for
        unexpected *var*.
    """
    if not isinstance(var, basestring):
        raise TypeError("var must be a string")
    if len(args) > 1:
        raise TypeError("get_env() accepts 2 arguments at most")

    var = var.upper()
    var_with_prefix = None
    match = _RE_ENV_VAR_PREFIX.match(var)
    if match:
        var_with_prefix = var
        var = _RE_ENV_VAR_PREFIX.sub('', var)
    else:
        for src_dict in (ENV_VARS, os.environ):
            for prefix in ('LMI_', 'LMI_SOFTWARE_', 'LMI_CIMOM_'):
                if prefix + var in src_dict:
                    var_with_prefix = prefix + var
                    break
            if var_with_prefix:
                break
        if not var_with_prefix:
            var_with_prefix = var

    val = os.environ.get(var_with_prefix, None)
    _type, default = ENV_VARS.get(var_with_prefix, (str, None))
    if len(args) > 0:
        default = args[0]
    if val is None:
        val = default
    else:
        val = _type(val)
    return val

USE_PKCON = get_env('backend') == "package-kit"

def check_output(*popenargs, **kwargs):
    """ subprocess.check_output for Python 2.6. """
    process = subprocess.Popen(stdout=subprocess.PIPE, *popenargs, **kwargs)
    output, unused_err = process.communicate()
    retcode = process.poll()
    if retcode:
        cmd = kwargs.get("args")
        if cmd is None:
            cmd = popenargs[0]
        error = subprocess.CalledProcessError(retcode, cmd)
        error.output = output
        raise error
    return output

def make_nevra(name, epoch, ver, rel, arch, with_epoch='NOT_ZERO'):
    """
    @param with_epoch may be one of:
        "NOT_ZERO" - include epoch only if it's not zero
        "ALWAYS"   - include epoch always
        "NEVER"    - do not include epoch at all
    """
    estr = ''
    if with_epoch.lower() == "always":
        estr = str(epoch)
    elif with_epoch.lower() == "not_zero":
        if epoch and str(epoch).lower() not in ("0", "(none)"):
            estr = str(epoch)
    if len(estr):
        estr += ":"
    return "%s-%s%s-%s.%s" % (name, estr, ver, rel, arch)

def make_evra(epoch, ver, rel, arch):
    """ @return evra string """
    if not epoch or str(epoch).lower() == "(none)":
        epoch = "0"
    return "%s:%s-%s.%s" % (epoch, ver, rel, arch)

def run_yum(*params, **kwargs):
    """
    Runs yum with params and returns its output
    It's here especially to allow pass a repolist argument, that
    specifies list of repositories, to run the command on.
    """
    cmd = ['yum'] + list(params)
    repolist = kwargs.get('repolist', None)
    if repolist is None:
        repolist = []
    if repolist:
        cmd += ['--disablerepo=*']
        cmd += ['--enablerepo='+r for r in repolist]
    return check_output(cmd)

def get_repo_list():
    """
    @return list of software repository names
    """
    repos_str = check_output(['yum', 'repolist', '-q'])
    return RE_REPO.findall(repos_str)

def get_system_architecture():
    """
    @return the system architecture name as seen by rpm
    """
    return check_output(['rpm', '-q', '--qf', '%{ARCH}\n', 'rpm'])

def verify_pkg(name):
    """
    @return True, if package is installed and passes rpm verification check
    """
    return subprocess.call(["rpm", "--quiet", "-Va", name]) == 0


def is_config_file(pkg, file_path):
    """
    @return True, if file_path is a configuration file of package pkg.
    """
    cmd = ['rpm', '-qc', pkg.name]
    out = check_output(cmd)
    return file_path in set(out.splitlines())   #pylint: disable=E1103

def is_doc_file(pkg, file_path):
    """
    @return True, if file_path is a documentation file of package pkg.
    """
    cmd = ['rpm', '-qd', pkg.name]
    out = check_output(cmd)
    return file_path in set(out.splitlines())   #pylint: disable=E1103

def make_identity_path(pkg, newer=True):
    """
    Make instance name for LMI_SoftwareIdentity from Package.
    """
    return wbem.CIMInstanceName(
            classname="LMI_SoftwareIdentity",
            namespace="root/cimv2",
            keybindings=wbem.NocaseDict({
                "InstanceID" : "LMI:LMI_SoftwareIdentity:%s" % pkg.get_nevra(
                    newer=newer)
            }))

def get_target_operating_system():
    """
    :returns: integer corresponding for TargetOperatingProperty of CIM_Check
        for this system
    """
    target_operating_system = 36 # LINUX
    if hasattr(platform, 'linux_distribution') and \
            platform.linux_distribution(
                    full_distribution_name=False)[0].lower() == 'redhat':
        target_operating_system = 79 # RHEL
        if platform.uname()[4].lower() == 'x86_64':
            target_operating_system = 80 # RHEL 64bit
    return wbem.Uint16(target_operating_system)

def get_installed_packages():
    """
    :returns: list of packages in format: ``NAME-EPOCH:VERSION-RELEASE.ARCH``
    """
    output = check_output(
            [ "/bin/rpm", "-qa", "--qf"
            , "%{NAME}-%{EPOCH}:%{VERSION}-%{RELEASE}.%{ARCH}\n"],
            stderr=DEV_NULL).split()
    package_list = []
    for package in output:
        # Skip all gpg public keys returned by rpm -qa
        if "gpg-pubkey" not in package:
            package_list.append(package.replace("(none)", "0"))
    return package_list

def make_pkg_op(ns, pkg):
    """
    :returns: Object path of ``LMI_SoftwareIdentity``
    :rtype: :py:class:`lmi.shell.LMIInstanceName`
    """
    if not isinstance(pkg, basestring):
        nevra = pkg.nevra
    else:
        nevra = pkg
    return ns.LMI_SoftwareIdentity.new_instance_name({
        "InstanceID" : 'LMI:LMI_SoftwareIdentity:' + nevra
    })

if sys.version_info[0] > 2 \
        or (sys.version_info[0] == 2 and sys.version_info[1] > 6):
    def timedelta_get_total_seconds(td):
        """
        :param td: Time delta object.
        :type datetime.timedelta
        :returns: Total number of seconds of time interval represented by given
            time delta.
        """
        return td.total_seconds()
else:
    # timedelta is missing `total_seconds()` method in Python < 2.7
    def timedelta_get_total_seconds(td):
        """
        :param td: Time delta object.
        :type datetime.timedelta
        :returns: Total number of seconds of time interval represented by given
            time delta.
        """
        if not isinstance(td, datetime.timedelta):
            raise TypeError("td must be a timedelta object")
        return ( (td.microseconds + (td.seconds + td.days * 24 * 3600) * 10**6)
               / 10**6)

