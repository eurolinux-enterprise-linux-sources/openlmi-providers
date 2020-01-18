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
# Authors: Michal Minar <miminar@redhat.com>
"""
Abstraction for RPM package for test purposes.
"""

import json

import util

class Package(object):  #pylint: disable=R0902
    """
    Element of test package database. It's a container for package
    informations. It contains two sets of versions for single package.
    That's meant for updating tests.
    """
    def __init__(self, name, epoch, ver, rel, arch, repo,
            **kwargs):
        """
        Arguments prefixed with 'up_' are for newer package.
        """
        self._name = name
        if not epoch or epoch.lower() == "(none)":
            epoch = "0"
        self._epoch = epoch
        self._ver = ver
        self._rel = rel
        self._arch = arch
        self._repo = repo
        safe = kwargs.get('safe', False)
        self._up_epoch = epoch if safe else kwargs.get('up_epoch', epoch)
        self._up_ver   = ver   if safe else kwargs.get('up_ver'  , ver)
        self._up_rel   = rel   if safe else kwargs.get('up_rel'  , rel)
        self._up_repo  = repo  if safe else kwargs.get('up_repo', repo)

    def __str__(self):
        return self.get_nevra()

    @property
    def name(self): return self._name   #pylint: disable=C0111,C0321
    @property
    def epoch(self): return self._epoch #pylint: disable=C0111,C0321
    @property
    def ver(self): return self._ver     #pylint: disable=C0111,C0321
    @property
    def rel(self): return self._rel     #pylint: disable=C0111,C0321
    @property
    def arch(self): return self._arch   #pylint: disable=C0111,C0321
    @property
    def repo(self): return self._repo   #pylint: disable=C0111,C0321
    @property
    def nevra(self):                    #pylint: disable=C0111,C0321
        return self.get_nevra(False)
    @property
    def evra(self):                     #pylint: disable=C0111,C0321
        return self.get_evra(False)

    @property
    def up_epoch(self): return self._up_epoch   #pylint: disable=C0111,C0321
    @property
    def up_ver(self): return self._up_ver       #pylint: disable=C0111,C0321
    @property
    def up_rel(self): return self._up_rel       #pylint: disable=C0111,C0321
    @property
    def up_repo(self): return self._up_repo     #pylint: disable=C0111,C0321
    @property
    def up_nevra(self):                         #pylint: disable=C0111,C0321
        return self.get_nevra(True)
    @property
    def up_evra(self):                          #pylint: disable=C0111,C0321
        return self.get_evra(True)

    @property
    def is_safe(self):
        """
        @return True if properties prefixed with up_ matches those without
        it. In that case a package is suited for non-dangerous tests.
        """
        return all(   getattr(self, a) == getattr(self, 'up_' + a)
                  for a in ('epoch', 'ver', 'rel', 'repo'))

    def get_nevra(self, newer=True, with_epoch='NOT_ZERO'):
        """
        @newer if True, evr part is made from properties prefixed with 'up_'
        @return pkg nevra string
        """
        if newer:
            attrs = ['name', 'up_epoch', 'up_ver', 'up_rel', 'arch']
        else:
            attrs = ['name', 'epoch', 'ver', 'rel', 'arch']
        return util.make_nevra(*[getattr(self, '_'+a) for a in attrs],
                with_epoch=with_epoch)

    def get_evra(self, newer=True):
        """
        @newer if True, evr part is made from properties prefixed with 'up_'
        @return pkg nevra string
        """
        attrs = [('up_' if newer else '')+a for a in ('epoch', 'ver', 'rel')]
        attrs.append('arch')
        return util.make_evra(*[getattr(self, '_'+a) for a in attrs])

class PackageEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, Package):
            return obj.__dict__
        return json.JSONEncoder.default(self, obj)

def from_json(json_object):
    if '_arch' in json_object:
        kwargs = {k[1:]: v for k, v in json_object.items()}
        return Package(**kwargs)
    return json_object

