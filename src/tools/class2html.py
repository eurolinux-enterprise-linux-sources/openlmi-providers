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

class HtmlExporter(object):
    
    def __init__(self, cliconn):
        self.classcache = {}
        self.cliconn = cliconn
        # original list of classes
        self.classes = []
        # set of all classes to print, i.e. incl. all parents
        self.queue = set()
        self.file = None

    def escape(self, string):
        re_string = re.compile(r'(?P<htmlchars>[<&>])|(?P<space>^[ \t]+)|(?P<lineend>\r\n|\r|\n)|(?P<protocal>(^|\s)((http|ftp)://.*?))(\s|$)', re.S|re.M|re.I)
        re_nl = re.compile(r"\\n")
        def do_sub(m):
            c = m.groupdict()
            if c['htmlchars']:
                return cgi.escape(c['htmlchars'])
            if c['lineend']:
                return '<br>'
            elif c['space']:
                t = m.group().replace('\t', '&nbsp;'*tabstop)
                t = t.replace(' ', '&nbsp;')
                return t
            elif c['space'] == '\t':
                return ' '*tabstop;
            else:
                url = m.group('protocal')
                if url.startswith(' '):
                    prefix = ' '
                    url = url[1:]
                else:
                    prefix = ''
                last = m.groups()[-1]
                if last in ['\n', '\r', '\r\n']:
                    last = '<br>'
                return '%s<a href="%s">%s</a>%s' % (prefix, url, url, last)
        string2 = re.sub(re_string, do_sub, string)
        return re.sub(re_nl, "<br/>", string2)
    
    def load_class(self, classname):
        if classname in self.classcache:
            return self.classcache[classname]

        c = self.cliconn.GetClass(classname)
        self.classcache[classname] = c
        return c

    def source_class(self, c, param):
        """
            Find a class where a CIMMethod or CIMProperty is first defined.
            Start at class c and inspect all parents.
        """

        while True:
            if not c.superclass:
                # we're at the top class
                return c.classname

            if isinstance(param, pywbem.CIMMethod):
                if not c.methods.has_key(param.name):
                    break
            else:
                if not c.properties.has_key(param.name):
                    break
            parent = c
            c = self.load_class(c.superclass)
        return parent.classname

    def display_type(self, param):
        """
            Return displayable type of given parameter.
            It adds [] if it's array and class name of referenced classes.
        """
        url=None
        if param.reference_class:
            ptype = param.reference_class
            url=param.reference_class + ".html"
        else:
            ptype = param.type
        if param.is_array:
            ptype = ptype + "[]"
        if url:
            ptype = "<a href=\"%s\">%s</a>" % (url, ptype)
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
                if p.qualifiers["In"].value:
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

    def print_keys(self, c):
        print >>self.file, "<h3>Key properties:</h3>"
        print >>self.file, "<table class=\"key\">"
        for prop in c.properties.values():
            if prop.qualifiers.has_key('Key'):
                src = self.source_class(c, prop)
                print >>self.file, "<tr class=\"key\"><td class=\"key\"><a href=\"%s.html#%s\">%s</a></td></tr>" % (src, prop.name, prop.name)
        print >>self.file, "</table>"

    def print_class(self, c):
        """
            Print one class, inc. header.
        """
        parent = None
        if c.superclass:
            parent = self.load_class(c.superclass)
        print >>self.file, "<h2>%s : <a href=\"%s.html\">%s</a></h2>" % (c.classname, c.superclass, c.superclass)

        description = c.qualifiers.get("Description", None)
        if not description:
            description = parent.qualifiers.get("Description", None)
        if description:
            print >>self.file, "<p>%s</p>" % (self.escape(description.value))

        self.print_keys(c)

        local_props = []
        inherited_props = []
        for name in sorted(c.properties.keys()):
            if parent and parent.properties.has_key(name):
                inherited_props.append(c.properties[name])
                if not self.compare_properties(c.properties[name], parent.properties[name]):
                    # the property was overridden
                    local_props.append(c.properties[name])
            else:
                local_props.append(c.properties[name])

        local_methods = []
        inherited_methods = []
        for name in sorted(c.methods.keys()):
            if parent and parent.methods.has_key(name):
                inherited_methods.append(c.methods[name])
                if not self.compare_properties(c.methods[name], parent.methods[name]):
                    # the property was overridden
                    local_methods.append(c.methods[name])
            else:
                local_methods.append(c.methods[name])

        print >>self.file, "<h3>Local properties:</h3>"

        if local_props:
            print >>self.file, "<table class=\"properties\"> <tr> <th>Name</th> <th>Type</th> <th>Qualifiers</th>"

            for prop in local_props:
                if prop in inherited_props:
                    style="i"
                else:
                    style="b"
                print >>self.file, "<tr> <td><%s><a name=\"%s\"/>%s</%s></td> <td>%s</td><td>" % (style, prop.name, prop.name, style, self.display_type(prop))
                print >>self.file, "<table class=\"qualifiers\">"
                self.print_qualifiers(prop.qualifiers.values())
                print >>self.file, "</table>"
                print >>self.file, "</td></tr>"
            print >>self.file, "</table>"
            print >>self.file, "<small><b>bold</b> - new property, <i>italics</i> - overridden property, usually with new description.</small>"
        else:
            print >>self.file, "None"

        print >>self.file, "<h3>Local methods:</h3>"
        if local_methods:
            print >>self.file, "<table class=\"properties\"> <tr> <th>Type</th><th>Name</th><th>Qualifiers</th>"
            for m in local_methods:
                print >>self.file, "<tr> <td>%s</td> <td><b><a name=\"%s\"/>%s</b></td><td>" % (m.return_type, m.name, m.name)
                print >>self.file, "<table class=\"qualifiers\">"
                self.print_qualifiers(m.qualifiers.values())
                print >>self.file, "<tr><td class=\"qualifiers\"><b>Parameters</b></td><td class=\"qualifiers\">"
                self.print_parameters(m.parameters)
                print >>self.file, "</td></tr>"
                print >>self.file, "</table>"
                print >>self.file, "</td></tr>"
            print >>self.file, "</table>"
        else:
            print >>self.file, "None"

        print >>self.file, "<h3>Inherited properties:</h3>"
        if inherited_props:
            print >>self.file, "<table class=\"inherited\"> <tr> <th>Name</th><th>Type</th><th>Source</th>"
            for p in inherited_props:
                src = self.source_class(c, p)
                print >>self.file, "<tr><td><a href=\"%s.html#%s\">%s</a></td><td>%s</td><td>%s</td></tr>" % (src, p.name, p.name, self.display_type(p), src)
            print >>self.file, "</table>"
        else:
            print >>self.file, "None"

        print >>self.file, "<h3>Inherited methods:</h3>"
        if inherited_methods:
            print >>self.file, "<table class=\"inherited\"> <tr> <th>Name</th><th>Source</th>"
            for m in inherited_methods:
                src = self.source_class(c, m)
                print >>self.file, "<tr><td><a href=\"%s.html#%s\">%s</a></td><td>%s</td></tr>" % (src, m.name, m.name, src)
            print >>self.file, "</table>"
        else:
            print >>self.file, "None"



    def print_file(self, filename):
        f = open(filename, "r")
        data = f.read()
        self.file.write(data)
        f.close()

    def print_page(self, c, header, footer):
        print "exporting ", c.classname
        self.file = open(c.classname + ".html", "w")
        print >>self.file, "<html><head><title>%s</title>" % (c.classname)
        print >>self.file, "<link rel=\"stylesheet\" type=\"text/css\" charset=\"utf-8\" media=\"all\" href=\"mof.css\" />"
        print >>self.file, "</head><body>"
        if header:
            self.print_file(header)
        self.print_class(c)
        if footer:
            self.print_file(footer)
        print >>self.file, "</body></html>"
        self.file.close()
        
    def add_class(self, classname):
        self.classes.append(classname)
        print "adding", classname
        self.queue.add(classname)
        # add all parents
        c = self.load_class(classname)
        parentname = c.superclass
        while parentname:
            if parentname in self.queue:
                break
            print "adding", parentname
            self.queue.add(parentname)
            c = self.load_class(parentname)
            parentname = c.superclass

    def export(self, header=None, footer=None):
        """
            Print everything, i.e. index.html + all classes + all their parents.
        """
        # remove duplicate classes from the queue (and sort them)
        self.queue = sorted(self.queue)
        self.print_index(header, footer)
        while self.queue:
            c = self.queue.pop()
            self.print_page(self.load_class(c), header, footer)

    def _do_print_tree(self, name, subtree, level):
        print >>self.file, "<tr class=\"tree\"><td class=\"tree\">"
        if level:
            print >>self.file, "&nbsp;&nbsp;&nbsp;|"*level + "--- "
        if name in self.classes:
            printname = "<span class=\"tree_local\">%s</span>" % (name)
        else:
            printname = "<span class=\"tree_inherited\">%s</span>" % (name)
        print >>self.file, "<a href=\"%s.html\">%s</a>" % (name, printname)
        print >>self.file, "</td></tr>"
        for n in subtree.keys():
            self._do_print_tree(n, subtree[n], level+1)

    def print_tree(self):
        """
            Create inheritance tree of classes.
        """
        # hash classname -> list of hash of (direct) sublasses 
        subclasses = {}
        # hash classname -> nr. of its parents
        parents = {}
        # initialize the hash
        for cname in self.queue:
            subclasses[cname] = {}
            parents[cname] = 0

        # fill the hash
        for cname in self.queue:
            c = self.load_class(cname)
            if c.superclass:
                subclasses[c.superclass][cname] = subclasses[cname]
                parents[cname] += 1

        print >>self.file, "<table class=\"tree\">"
        # print all top classes
        for cname in self.queue:
            if parents[cname] == 0:
                self._do_print_tree(cname, subclasses[cname], 0)
        print >>self.file, "</table>"

    def print_index(self, header, footer):
        print "exporting index"
        self.file = open("index.html", "w")
        print >>self.file, "<html><head><title>Index</title>"
        print >>self.file, "<link rel=\"stylesheet\" type=\"text/css\" charset=\"utf-8\" media=\"all\" href=\"mof.css\" />"
        print >>self.file, "</head><body>"
        if header:
            self.print_file(header)
        self.print_tree()
        if footer:
            self.print_file(footer)
        print >>self.file, "</body></html>"
        self.file.close()
            
description = """
Generate HTML documentation for given CIM classes. The tool connects to specified CIMOM
and reads class definition from there. It generates separate HTML file for each class
specified on command line and for all it's parents.

The tool also generates index.html with inheritance tree.

All generated HTML pages will optionally contain header and/or footer. Both must be valid
HTML snippets and will be inserted just after <body> or before </body>.
"""

parser = optparse.OptionParser(usage="usage: %prog [options] classname [classname ...]", description=description)
parser.add_option('-u', '--url', action='store', dest='addr', default='https://localhost:5989', help='URL of CIM server, default: https://localhost:5989')
parser.add_option('-U', '--user', action='store', dest='user', default=None, help='CIM user name')
parser.add_option('-P', '--password', action='store', dest='password', default=None, help='CIM password')
parser.add_option('-H', '--header', action='store', dest='header', default=None, help='File with HTML page header')
parser.add_option('-F', '--footer', action='store', dest='footer', default=None, help='File with HTML page footer')

(options, args) = parser.parse_args()

cliconn = pywbem.WBEMConnection(options.addr, (options.user, options.password))
exporter = HtmlExporter(cliconn)
for c in args:
    exporter.add_class(c)
exporter.export(options.header, options.footer)

