#!/usr/bin/python
# -*- Coding:utf-8 -*-
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

import platform
import pywbem
import re
from subprocess import call, check_output

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
        if epoch and str(epoch).lower() not in {"0", "(none)"}:
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
    return call(["rpm", "--quiet", "-Va", name]) == 0


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
    return pywbem.CIMInstanceName(
            classname="LMI_SoftwareIdentity",
            namespace="root/cimv2",
            keybindings=pywbem.NocaseDict({
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
    return pywbem.Uint16(target_operating_system)

def get_installed_packages():
    """
    :returns: list of packages in format: ``NAME-EPOCH:VERSION-RELEASE.ARCH``
    """
    output = check_output(
            [ "/usr/bin/rpm", "-qa", "--qf"
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


