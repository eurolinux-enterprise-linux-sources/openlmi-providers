#!/usr/bin/python
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
# Authors: Radek Novacek <rnovacek@redhat.com>
#

import sys
import pywbem

if len(sys.argv) < 4:
    print """Usage: %s <address> <state> [username] [password]
    Connect to CIM server at address and change the power state of the machine.

Available states:
    4 - sleep
    5 - force reboot
    7 - hibernate
    8 - force poweroff
    12 - poweroff
    15 - reboot

Example: %s https://127.0.0.1:5989 4 root redhat""" % (sys.argv[0], sys.argv[0])
    sys.exit(1)

url = sys.argv[1]
try:
    state = int(sys.argv[2])
except ValueError:
    print >>sys.stderr, "Unknown state: %s" % sys.argv[2]
    sys.exit(4)

username = None
password = None
if len(sys.argv) > 3:
    username = sys.argv[3]
if len(sys.argv) > 4:
    password = sys.argv[4]

cliconn = pywbem.WBEMConnection(url, (username, password))

computerSystems = cliconn.ExecQuery('WQL', 'select * from Linux_ComputerSystem')

if len(computerSystems) == 0:
    print >>sys.stderr, "No usable Linux_ComputerSystem instance found."
    sys.exit(2)

if len(computerSystems) > 1:
    print >>sys.stderr, "More than one Linux_ComputerSystem instance found, don't know which to use."
    sys.exit(3)

print cliconn.InvokeMethod("RequestPowerStateChange", "LMI_PowerManagementService",
                     ManagedElement=computerSystems[0].path,
                     TimeoutPeriod=pywbem.datetime.now(),
                     PowerState=pywbem.Uint16(state),
                     Time=pywbem.datetime.now()
                    )
