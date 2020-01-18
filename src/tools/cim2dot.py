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
# Authors: Jan Safranek <jsafrane@redhat.com>
#
# This is a tool to dump all object instances from a remote CIM server
# and draw them using dot.
#
# The dump starts at given class - all instances of this class will be drawn.
# The dump then continues with instances referenced by already drawn instances.
# The dumps stops when it reaches given recursion level.
#
# Generated dot output will have class names in node labels (class names are
# way shorter than object paths). Object paths are shown in 'tooltip' - view
# the generated .svg file in Firefox and point your mouse to a node.
# Hyperlinks lead to specified YAWN instance.
#
# Example:
# ./cim2dot.py -u http://ford.usersys.redhat.com:5988 -n root/ontap \
#              -l 20 \
#              -i "Statistic,Stats,LANEnd,ProtocolEnd,Ethernet,DiskDrive,ONTAP_StorageSystem" \
#              -t "http://localhost/yawn/GetInstance" \
#              -c CIM_StorageExtent \
#  | dot -Tsvg >image.svg
#

import sys
import pywbem
import zlib
import cPickle
import base64
import cgi
import optparse

class CimExporter(object):
    def __init__(self, loader, ignore=None, urltemplate='http://localhost/yawn/GetInstance', titles=None):
        self.loader = loader
        self.ignore = ignore
        self.instanceLabels = {} #path -> label
        self.urltemplate = urltemplate
        self.titles = titles

    def isIgnored(self, instance):
        if self.ignore is None:
            return False
        for i in self.ignore.split(','):
            if instance.classname.find(i) >= 0:
                print >>sys.stderr, "Ignoring", i, ':', instance.classname
                return True
        return False

    def getTitle(self, instance):
        title = instance.classname
        if self.titles is None:
            return title
        for i in self.titles.split(','):
            if instance.has_key(i):
                title +=  "\\n" + instance[i]
        return title

    def drawInstance(self, instance):
        if self.isIgnored(instance):
            return
        path = self.loader.getInstancePath(instance)
        name = 'obj' + str(len(self.instanceLabels))
        self.instanceLabels[path] = name

        _encodeObject = lambda x: (base64.b64encode(zlib.compress(cPickle.dumps(x, cPickle.HIGHEST_PROTOCOL))))
        params = {'url': self.loader.cliconn.url, 'ns': self.loader.namespace, 'instPath':_encodeObject(instance)}
        url = self.urltemplate + "?" + cgi.urllib.urlencode(params)
        title = self.getTitle(instance)
        print '%s [tooltip="%s",label="%s",URL="%s"];' % (name, path, title, url)

    def drawReference(self, reference):
        # find the first and the second CIMInstanceName among keybindings
        vals = reference.keybindings.values()
        i = 0;
        while not isinstance(vals[i], pywbem.CIMInstanceName):
            i = i+1
        src = vals[i]
        i = i+1
        while not isinstance(vals[i], pywbem.CIMInstanceName):
            i = i+1
        dst = vals[i]

        if self.isIgnored(src) or self.isIgnored(dst) or self.isIgnored(reference):
            return
        label = reference.classname
        srcName = self.instanceLabels[self.loader.getInstancePath(src)]
        dstName = self.instanceLabels[self.loader.getInstancePath(dst)]
        print '%s -- %s [label="%s"];' % (srcName, dstName, label)

    def export(self):
        instances = self.loader.instances.values()
        instances.sort()
        references = self.loader.references.values()
        references.sort()
        for i in instances:
            self.drawInstance(i)
        for r in references:
            self.drawReference(r)

class CimLoader(object):
    def __init__(self, address, classname, levels, user=None, password=None, namespace='root/cimv2', ignore=None):
        self.cliconn = pywbem.WBEMConnection(address, (user, password))
        self.cliconn.default_namespace = namespace
        self.levels = levels
        self.namespace = namespace
        self.classname = classname
        self.instances = {} #path -> CIMInstanceName
        self.queue = {} #path -> level
        self.references = {} #'classname:path-path' -> reference
        self.ignore = ignore

    def isIgnored(self, instance):
        if self.ignore is None:
            return False
        for i in self.ignore.split(','):
            if instance.classname.find(i) >= 0:
                print >>sys.stderr, "Skipping", i, ':', instance.classname
                return True
        return False

    def getInstancePath(self, instance):
        instance.host='f16'
        path = str(instance)
        return path.replace('"', "'")

    def getReferencePath(self, reference, src, dst):
        label = reference.classname
        srcName = self.getInstancePath(src)
        dstName = self.getInstancePath(dst)
        return label+':'+srcName+'-'+dstName

    def addInstances(self, instances, level):
        for i in instances:
            path = self.getInstancePath(i)
            if not self.instances.has_key(path):
                self.instances[path] = i
                self.queue[path] = level

    def addReferences(self, instance, level):
        if self.levels <= level:
            return
        try:
            refs = self.cliconn.ReferenceNames(instance)
        except pywbem.cim_operations.CIMError:
            print >>sys.stderr, 'Error getting references of %s: %s' % (self.getInstancePath(instance),  sys.exc_info()[0])
        else:
            for ref in refs:
                for i in ref.values():
                    if isinstance(i, pywbem.CIMInstanceName) and i.classname != instance.classname:
                        if self.isIgnored(i):
                            continue
                        self.addInstances([i,], level+1)
                        if not (self.references.has_key(self.getReferencePath(ref, instance, i))
                                or self.references.has_key(self.getReferencePath(ref, i, instance))):
                            self.references[self.getReferencePath(ref, instance, i)] = ref

    def load(self):
        # add initial instances
        instances = self.cliconn.EnumerateInstanceNames(self.classname, namespace=self.namespace)
        self.addInstances(instances, 0)

        while True:
            try:
                (path, level) = self.queue.popitem()
            except KeyError:
                break
            instance = self.instances[path]
            self.addReferences(instance, level)

    def dumpTo(self, filename):
        f =  open(filename, 'w')
        cPickle.dump((self.instances, self.references), f)
        f.close()

    def loadFrom(self, filename):
        f =  open(filename, 'r')
        (self.instances, self.references) = cPickle.load(f)
        f.close()

# Parse command line arguments
parser = optparse.OptionParser()
parser.add_option('-u', '--url', action='store', dest='addr', default='http://ford.usersys.redhat.com:5988', help='URL of CIM server, default: http://ford.usersys.redhat.com:5988')
parser.add_option('-c', '--class', action='store', dest='classname', help='Name of class to start with. All instances of this class + (recursively) all referenced classes will be drawn. This option is mandatory.')
parser.add_option('-l', '--levels', action='store', type='int', dest='levels', default='5', help='Number of references to track from the initial class. This option limits level of recursion. Default: 5.')
parser.add_option('-i', '--ignore', action='store', dest='ignore', default=None, help='Comma-separated list of classes to ignore. Classes, which contain these strings, will be ignored and won\'t be drawn (but they and their references will be loaded from the CIM server).')
parser.add_option('-I', '--really-ignore', action='store', dest='reallyignore', default=None, help='Comma-separated list of classes to skip. Classes, which contain these strings, will be ignored and won\'t even retrieved from the CIM server.')
parser.add_option('-k', '--draw-keys', action='store', dest='keys', default=None, help='Comma-separated list of object keys, which will be drawn in instances, if the instance has given key.')
parser.add_option('-n', '--namespace', action='store', dest='namespace', default='root/cimv2', help='CIM namespace. Default: root/cimv2')
parser.add_option('-t', '--template', action='store', dest='template', default='http://localhost/yawn/GetInstance', help='Hyperlink template to YAWN. It must have form of "http://localhost/yawn/GetInstance".')
parser.add_option('-U', '--user', action='store', dest='user', default=None, help='CIM user name')
parser.add_option('-P', '--password', action='store', dest='password', default=None, help='CIM password')
parser.add_option('-W', '--write', action='store', dest='outfile', default=None, help='Write discovered CIM objects and references to given file. All objects are written, even those ignored by --ignore option.')
parser.add_option('-R', '--read', action='store', dest='infile', default=None, help='Read objects from given file instead of remote CIM server. Useful when you just want to update --ignore parameter.')
(options, args) = parser.parse_args()

if not options.classname:
    print "Missing classname."
    parser.print_help()
    sys.exit(1)

l = CimLoader(options.addr, options.classname, options.levels, options.user, options.password, options.namespace, options.reallyignore)
if options.infile:
    l.loadFrom(options.infile)
else:
    l.load()

if options.outfile:
    l.dumpTo(options.outfile)


e = CimExporter(l, options.ignore, options.template, options.keys)
print """graph test{
node[shape=box]"""
e.export()
print '}'
