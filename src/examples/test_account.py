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
# Authors: Roman Rakus <rrakus@redhat.com>
#

import sys
import pywbem

def usage():
  print """Usage: %s <address> <username> <password> <command> [parameter...]
    Connect to CIM server at address and issue a command on the machine.

Available commands and their parameters:
  list_user - list users, parameter is user name or empty for all
  list_group - list groups, parameter is group name or empty for all
  group_members - list members of group, parameter is group name
  delete_user - delete account, needed parameter is account name
  delete_group - delete group, needed parameter is group name
  delete_identity - delete user or group, parameter InstanceID of identity
  create_account - creates a new account, parameter required, user login name
  create_group - creates a new group, parameter required, group name

Example: %s https://127.0.0.1:5989 root redhat list_user""" % (sys.argv[0], sys.argv[0])

if len(sys.argv) < 5:
    usage()
    sys.exit(1)

url = sys.argv[1]

username = None
password = None

if sys.argv[2]:
    username = sys.argv[2]
if sys.argv[3]:
    password = sys.argv[3]

command = sys.argv[4]
parameters = sys.argv[5:]

cliconn = pywbem.WBEMConnection(url, (username, password))

if command == "list_user":
# Listintg users is simple, just query all instances from LMI_Account
# or select only by given Name
  slct = "select * from LMI_Account"
  if parameters:
    for i, parameter in enumerate(parameters):
      if i==0:
        slct += ' where Name = "%s"' % parameter
      else:
        slct += ' or Name = "%s"' % parameter

  instances = cliconn.ExecQuery('WQL', slct)
  for instance in instances:
    print instance.tomof()

elif command == "list_group":
# Listintg gruops is simple, just query all instances from LMI_Group
# or select only by given Name
  slct = "select * from LMI_Group"
  if parameters:
    for i, parameter in enumerate(parameters):
      if i==0:
        slct += ' where Name = "%s"' % parameter
      else:
        slct += ' or Name = "%s"' % parameter


  instances = cliconn.ExecQuery('WQL', slct)
  for instance in instances:
    print instance.tomof()

elif command == "group_members":
# Group members is a bit tricky. You need to select group (LMI_Group)
# by given Name, then you need to select identities (LMI_Identity), which
# are connected through LMI_MemberOfGroup to LMI_Group.
# And finally select all accounts (LMI_Account) which are connected through
# LMI_AssignedAccountIdentity with selected identities (this should be
# 1 to 1)
  if not parameters:
    usage()
    sys.exit(1)

  else:
    groups = cliconn.ExecQuery('WQL', 'select * from LMI_Group where Name = "%s"' % parameters[0])
  for group in groups:
    identities = cliconn.Associators(group.path, AssocClass='LMI_MemberOfGroup')
    for identity in identities:
        accounts = cliconn.Associators(identity.path, AssocClass='LMI_AssignedAccountIdentity')
        for account in accounts:
          print account.tomof()

elif command == "create_account":
# create a new account
# Firstly find system name, which is necessary parameter for method
# then invoke the method
  if not parameters:
    usage()
    sys.exit(1)

  computerSystems = cliconn.ExecQuery('WQL', 'select * from Linux_ComputerSystem')
  if not computerSystems:
    print >>sys.stderr, "No usable Linux_ComputerSystem instance found."
    sys.exit(2)

  if len(computerSystems) > 1:
    print >>sys.stderr, "More than one Linux_ComputerSystem instance found, don't know which to use."
    sys.exit(3)

  lams = cliconn.ExecQuery('WQL', 'select * from LMI_AccountManagementService')[0]

  print cliconn.InvokeMethod("CreateAccount", lams.path,
    Name = parameters[0],
    System = computerSystems[0].path)

elif command == "create_group":
# create a new group
# Firstly find system name, which is necessary parameter for method
# then invoke the method
  if not parameters:
    usage()
    sys.exit(1)

  computerSystems = cliconn.ExecQuery('WQL', 'select * from Linux_ComputerSystem')
  if not computerSystems:
    print >>sys.stderr, "No usable Linux_ComputerSystem instance found."
    sys.exit(2)

  if len(computerSystems) > 1:
    print >>sys.stderr, "More than one Linux_ComputerSystem instance found, don't know which to use."
    sys.exit(3)

  lams = cliconn.ExecQuery('WQL', 'select * from LMI_AccountManagementService')[0]

  print cliconn.InvokeMethod("CreateGroup", lams.path,
    Name = parameters[0],
    System = computerSystems[0].path)

elif command == "delete_user":
# Find user by given name and call DeleteInstance on the instance path
  if not parameters:
    usage()
    sys.exit(1)

  slct = 'select * from LMI_Account where Name = "%s"' % parameters[0]

  instances = cliconn.ExecQuery('WQL', slct)
  if instances:
    print cliconn.DeleteInstance(instances[0].path)
  else:
    print >> sys.stderr, "User does not exist: %s" %parameters[0]

elif command == "delete_user":
# Find user by given name and call DeleteInstance on the instance path
  if not parameters:
    usage()
    sys.exit(1)

  slct = 'select * from LMI_Account where Name = "%s"' % parameters[0]

  instances = cliconn.ExecQuery('WQL', slct)
  if instances:
    print cliconn.DeleteInstance(instances[0].path)
  else:
    print >> sys.stderr, "User does not exist: %s" %parameters[0]

elif command == "delete_group":
# Find group by given name and call DeleteInstance on the instance path
  if not parameters:
    usage()
    sys.exit(1)

  slct = 'select * from LMI_Group where Name = "%s"' % parameters[0]

  instances = cliconn.ExecQuery('WQL', slct)
  if instances:
    print cliconn.DeleteInstance(instances[0].path)
  else:
    print >> sys.stderr, "Group does not exist: %s" %parameters[0]

elif command == "delete_identity":
# Have to pass correct InstanceID
# It is in format:
# LMI:UID:user_id
# or
# LMI:GID:group_id
  if not parameters:
    usage()
    sys.exit(1)

  slct = 'select * from LMI_Identity where InstanceID = "%s"' % parameters[0]

  instances = cliconn.ExecQuery('WQL', slct)
  if instances:
    print cliconn.DeleteInstance(instances[0].path)
  else:
    print >> sys.stderr, "Identity does not exist: %s" %parameters[0]

else:
# unknown command
  print "Unknown command", command
  usage()
  sys.exit(1)
