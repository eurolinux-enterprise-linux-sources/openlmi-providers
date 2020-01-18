#!/bin/sh
# 
# serviceutil.sh
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
# Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
#

# path to systemd service directory
SYSTEMD_SDIR=/lib/systemd/system
# path to sysv service initscript directory
SYSV_SDIR=/etc/rc.d/init.d
# service unit name
SUNIT_NAME=$2.service

if [ -f $SYSTEMD_SDIR/$SUNIT_NAME ];
then
  case "$1" in
    start|stop|reload|restart|try-restart|condrestart|reload-or-restart|reload-or-try-restart|enable|disable|is-enabled)
      systemctl -- $1 $SUNIT_NAME
      ;;
    status)
      output=`systemctl -- status $SUNIT_NAME`
      if echo "$output" | grep Active: | grep inactive > /dev/null 2>&1; then
        echo "stopped"
      elif echo "$output" | grep Active: | grep failed > /dev/null 2>&1; then
        echo "stopped" # TODO - should be failed and propagated to the state property
      else
        pid=`echo "$output" | sed -n -e 's/^[ \t]\+Main PID:[ \t]\+\([0-9]\+\).*/\1/p'`
        if [ "$pid" == "" ]; then
          # Some service don't have Main PID (e.g. iptables), report as PID 1
          echo "1 $2"
        else
          echo "$pid $2"
        fi
      fi
      ;;
    *)
      echo "Unsupported method!"
      exit 1
  esac
elif [ -f $SYSV_SDIR/$2 ];
then
  case "$1" in
    start|stop|reload|restart|condrestart)
      $SYSV_SDIR/$2 $1
      ;;
    status)
     output=`$SYSV_SDIR/$2 status`
      if echo "$output" | grep "stopped" > /dev/null 2>&1; then
        echo "stopped"
      elif echo "$output" | grep "not running" > /dev/null 2>&1; then
        echo "stopped"
      elif echo "$output" | grep "running" > /dev/null 2>&1; then
        echo "$output" | awk '{print $3 " " $1}' | tr -d '=)='
      fi
      ;;
    is-enabled)
     CUR_RLVL=`runlevel | cut -d " " -f 2`
     output=`chkconfig --list tog-pegasus | cut -f $((CUR_RLVL + 2))`
     if echo "$output" | grep "on" > /dev/null 2>&1; then
        echo "enabled"
     elif echo "$output" | grep "off" > /dev/null 2>&1; then
        echo "disabled"
     fi
     ;;
    enable)
     chkconfig $2 on
      ;;
    disable)
     chkconfig $2 off
      ;;
    *)
      echo "Unsupported method!"
      exit 1
  esac
fi

exit 0
