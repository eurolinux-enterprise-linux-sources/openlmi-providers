#!/bin/sh
#
# servicedisc.sh
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
# Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
#


# path to systemd service directory
SYSTEMD_SDIR=/lib/systemd/system
# path to sysv service initscript directory
SYSV_SDIR=/etc/rc.d/init.d

if [ -d $SYSTEMD_SDIR ];
then
  for i in $SYSTEMD_SDIR/*[^@].service;
  do
    SFILE=${i#$SYSTEMD_SDIR/}
    echo ${SFILE%.service}
  done
  exit 0
elif [ -d $SYSV_SDIR ];
then
  chkconfig --list | awk '{print $1}'
  exit 0
fi

# unsupported init system
exit 1
