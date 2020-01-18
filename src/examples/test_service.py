#!/usr/bin/python
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
# Authors: Radek Novacek <rnovacek@redhat.com>
#

import sys
import pywbem

commands = {
    'start': 'StartService',
    'stop': 'StopService',
    'reload': 'ReloadService',
    'restart': 'RestartService',
    'try-restart': 'TryRestartService',
    'condrestart': 'CondRestartService',
    'reload-or-restart': 'ReloadOrRestartService',
    'reload-or-try-restart': 'ReloadOrTryRestartService',
    'enable': 'TurnServiceOn',
    'disable': 'TurnServiceOff'
}

def sanitize(text):
    return text.replace('"', '').replace("'", "")

if len(sys.argv) < 5:
    print """Usage: %s <address> <service> <operation> [username] [password]
    Connect to CIM server at address and execute an operation on the service.

Available states:
    %s

Example: %s https://127.0.0.1:5989 bluetooth start root redhat""" % (sys.argv[0], "\n    ".join(commands.keys()), sys.argv[0])
    sys.exit(1)

url = sys.argv[1]
service = sanitize(sys.argv[2])
operation = sanitize(sys.argv[3])

username = None
password = None
if len(sys.argv) > 4:
    username = sys.argv[4]
if len(sys.argv) > 5:
    password = sys.argv[5]

cliconn = pywbem.WBEMConnection(url, (username, password))
cliconn.debug = True

services = cliconn.ExecQuery("WQL", "select * from LMI_Service where Name='%s'" % service)

if len(services) == 0:
    print >>sys.stderr, "Service %s doesn't exist" % service 
    sys.exit(2)

if len(services) > 1:
    print >>sys.stderr, "There are more services with name '%s', don't know which one use" % service
    sys.exit(3)

if operation not in commands.keys():
    print >>sys.stderr, "Invalid operation '%s'" % operation
    sys.exit(4)

print cliconn.InvokeMethod(commands[operation], services[0].path)
