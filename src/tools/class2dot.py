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
# Authors: Jan Safranek <jsafrane@redhat.com>
#

import os;
import pywbem;
import optparse
import re
import cgi
import sys

class DotExporter(object):
    
    def __init__(self, cliconn):
        self.classcache = {}
        self.cliconn = cliconn
        # original list of classes
        self.classes = set()
        self.file = sys.stdout

    def load_class(self, classname):
        if classname in self.classcache:
            return self.classcache[classname]

        c = self.cliconn.GetClass(classname)
        self.classcache[classname] = c
        return c

    def display_type(self, param):
        """
            Return displayable type of given parameter.
            It adds [] if it's array and class name of referenced classes.
        """
        if param.reference_class:
            ptype = param.reference_class
        else:
            ptype = param.type
        if param.is_array:
            ptype = ptype + "[]"
        return ptype

    def print_parameters(self, params):
        """
            Print table of method parametes.
        """
        if not params:
            print >>self.file, "None"
            return

        print >>self.file, "<table class=\"parameters\">"
        for p in params.values():
            direction = set()
            if p.qualifiers.has_key("Out"):
                direction.add("OUT")
            if p.qualifiers.has_key("In"):
                direction.add("IN")
            if not direction:
                direction.add("IN")
            direction = "/".join(sorted(direction))

            print >>self.file, "<tr>"
            print >>self.file, "<td>%s</td><td>%s</td><td><b>%s</b></td>" % (direction, self.display_type(p), p.name)
            print >>self.file, "<td><table class=\"qualifiers\">"
            self.print_qualifiers(p.qualifiers.values())
            print >>self.file, "</table></td>"
            print >>self.file, "</tr>"
        print >>self.file, "</table>"

    def print_qualifiers(self, qualifiers):
        """
            Print contenf of table of qualifiers.
            Only Deprecated, Description, Values and ValueMap are recognized.
        """
        deprecated = ""
        for q in sorted(qualifiers):
            if q.name == "Deprecated":
                deprecated = "<div class=\"deprecated\">DEPRECATED</div> "
            if q.name == "Description":
                print >>self.file, "<tr><td class=\"qualifiers\" colspan=\"2\">%s%s</td></tr>" % (deprecated, self.escape(q.value))
            if q.name == "Values":
                print >>self.file, "<tr><td class=\"qualifiers\"><b>Values</b></td> <td class=\"qualifiers\"><table><tr><td class=\"qualifiers\">%s</td></tr></table></td></tr>" % ("</td></tr><tr><td class=\"qualifiers\">".join(q.value))
            if q.name == "ValueMap":
                print >>self.file, "<tr><td class=\"qualifiers\"><b>ValueMap</b></td> <td class=\"qualifiers\"><table><tr><td class=\"qualifiers\">%s</td></tr></table></td></tr>" % ("</td></tr><tr><td class=\"qualifiers\">".join(q.value))

    def compare_properties(self, p1, p2):
        """
            Compare two properties if they should printed in Inherited properties.
            Only Name, Description and Implemented are checked.
            Returns False, if the property should be printed in Local.
        """
        if p1.name != p2.name:
            return False
        d1 = p1.qualifiers.get("Description", None)
        d2 = p2.qualifiers.get("Description", None)
        if d1.value != d2.value:
            return False
        i1 = p1.qualifiers.get("Implemented", None)
        i2 = p2.qualifiers.get("Implemented", None)
        if i1 and i1.value and not (i2 and i2.value):
            return False
        return True

    def print_class(self, c, display_local = True, box_only = False):
        """
            Print one class, inc. header.
        """
        parent = None
        if c.superclass:
            parent = self.load_class(c.superclass)

        if c.superclass:
            # draw arrow to parent
            print >>self.file, "\"%s\"->\"%s\"" % (c.classname, c.superclass)

        if box_only:
            return

        local_props = []
        for name in sorted(c.properties.keys()):
            if parent and parent.properties.has_key(name):
                if not self.compare_properties(c.properties[name], parent.properties[name]):
                    # the property was overridden
                    local_props.append(c.properties[name])
            else:
                local_props.append(c.properties[name])

        local_methods = []
        for name in sorted(c.methods.keys()):
            if parent and parent.methods.has_key(name):
                if not self.compare_properties(c.methods[name], parent.methods[name]):
                    # the property was overridden
                    local_methods.append(c.methods[name])
            else:
                local_methods.append(c.methods[name])

        self.file.write("\"%s\" [href=\"%s.html\",shape=\"record\", label=\"{%s" %(c.classname, c.classname, c.classname))

        if display_local:
            self.file.write("|")
            for prop in local_props:
                self.file.write("%s %s\\l" % (self.display_type(prop), prop.name))

            self.file.write("|")

            if local_methods:
                for m in local_methods:
                    self.file.write("%s()\l" % (m.name))

        self.file.write("}\"];\n")


    def add_class(self, classname):
        self.classes.add(classname)

    def export(self, shrink, noassoc = False):
        """
            Print all classes and their parents.
        """
        print >>self.file, """
digraph "classes_No_Name" {
charset="utf-8"
rankdir=BT
node [shape="record" fontsize=10 fontname="sans-serif"]
edge [arrowhead="empty" fontsize=10 fontname="sans-serif"]
"""
        while self.classes:
            c = self.classes.pop()
            
            cl = self.load_class(c)
            if noassoc and cl.qualifiers.get("Association", False):
                continue
         
            if shrink and shrink.match(c):
            
                self.print_class(cl, box_only = True)
            else:
                self.print_class(cl)

        print >>self.file, "}"

description = """
Generate UML image for given classes. The tool connects to specified CIMOM
and reads class definition from there. Each class specified on command line
will be drawn as one box, containing locally defined or re-defined properties
and methods. Inheritance will be shown as arrow between a parent class and a
subclass.
"""

parser = optparse.OptionParser(usage="usage: %prog [options] classname [classname ...]", description=description)
parser.add_option('-u', '--url', action='store', dest='addr', default='https://localhost:5989', help='URL of CIM server, default: https://localhost:5989')
parser.add_option('-U', '--user', action='store', dest='user', default=None, help='CIM user name')
parser.add_option('-s', '--shrink', action='store', dest='shrink', default=None, help='Regular expression pattern of CIM classes, which will be drawn only as boxes, without properties.')
parser.add_option('-A', '--no-associations', action='store_true', dest='noassoc', default=False, help='Skip association classes.')
parser.add_option('-P', '--password', action='store', dest='password', default=None, help='CIM password')
(options, args) = parser.parse_args()

sys.stdout.softspace=0

shrink = None
if options.shrink:
    shrink = re.compile(options.shrink)

cliconn = pywbem.WBEMConnection(options.addr, (options.user, options.password))
exporter = DotExporter(cliconn)
for c in args:
    exporter.add_class(c)
exporter.export(shrink = shrink, noassoc = options.noassoc)

