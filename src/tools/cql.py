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

import os
import sys
import pywbem

HISTORY = os.path.expanduser("~/.cql-history")

class QueryException(Exception):
    pass

class Printer(object):
    def printTable(self, f, headers, table):
        raise NotImplementedError

class RawPrinter(Printer):
    def __init__(self, hsep="-", vsep="|", cross_sep="+"):
        self.hsep = hsep
        self.vsep = vsep
        self.cross_sep = cross_sep

    def printTable(self, f, headers, table):
        # Compute length of the columns
        lengths = None
        for row in table:
            if lengths is None:
                lengths = {}
                for h in headers:
                    lengths[h] = len(str(h))
            for key, column in row.items():
                length = len(str(column))
                lengths[key] = max(lengths[key], length)

        # Header and separator
        header = self.vsep
        sep = self.cross_sep
        for key in headers:
            h = str(key).ljust(lengths[key], " ")
            header += " %s %s" % (h, self.vsep)
            sep += "%s%s" % (self.hsep * (len(h) + 2), self.cross_sep)
        f.write("%s\n" % sep)
        f.write("%s\n" % header)
        f.write("%s\n" % sep)

        for row in table:
            f.write(self.vsep)
            for key in headers:
                f.write(" %s %s" % (str(row[key]).ljust(lengths[key], " "), self.vsep))
            f.write("\n")
        f.write("%s\n" % sep)

class CQLConsole(object):
    def __init__(self, url, username, password, printer=RawPrinter()):
        self.url = url
        self.username = username
        self.password = password
        self.printer = printer

        self.connection = pywbem.WBEMConnection(url, (username, password))

    def runQuery(self, query):
        try:
            result = self.execQuery(query)
        except QueryException, e:
            print >>sys.stderr, "Unable to execute query:", str(e)
            return 1

        table, headers = self.processResult(result)
        if len(table) > 0:
            self.printTable(sys.stdout, headers, table)
        else:
            print >>sys.stderr, "No rows returned"
            return 1

        return 0

    def execQuery(self, query):
        try:
            return self.connection.ExecQuery('CQL', query)
        except Exception, e:
            raise QueryException(e[1])

    def processResult(self, result):
        table = []
        headers = []
        for item in result:
            row = {}
            for k in item.keys():
                if k not in headers:
                    headers.append(k)
                row[k] = item[k]
            table.append(row)
        return table, headers

    def printTable(self, f, headers, table):
        self.printer.printTable(f, headers, table)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print """This is interactive console for running CQL (CIM Query Language) queries on CIMOM server.

Usage: %s <address> [username] [password] [query]

If query is specified interactive console won't start and this program will exit immediatelly.

You can use this to pretty show wide results:
%s <address> username password query | less -S""" % (sys.argv[0], sys.argv[0])
        sys.exit(1)

    url = sys.argv[1]
    username = None
    password = None
    sql = None
    if len(sys.argv) > 2:
        username = sys.argv[2]
    if len(sys.argv) > 3:
        password = sys.argv[3]
    if len(sys.argv) > 4:
        sql = sys.argv[4]

    console = CQLConsole(url, username, password)
    if sql is not None:
        sys.exit(console.runQuery(sql))

    use_readline = False
    try:
        import readline
        use_readline = True
    except Exception:
        pass

    if use_readline:
        if os.access(HISTORY, os.R_OK):
            readline.read_history_file(HISTORY)
        else:
            f = open(HISTORY, "w")
            f.close()

    while True:
        try:
            sql = raw_input("> ")
        except EOFError:
            break

        if len(sql) == 0:
            continue

        console.runQuery(sql)

    if use_readline:
        readline.write_history_file(HISTORY)
