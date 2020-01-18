#!/usr/bin/python
#
# Copyright (C) 2013-2014 Red Hat, Inc.  All rights reserved.
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
# Authors: Jan Synacek <jsynacek@redhat.com>

from test_base import LogicalFileTestBase, Configuration
from lmi.test import CIMError
from lmi.test import unittest
from lmi.test import wbem
import os
import stat
import shutil
import subprocess

class TestLogicalFile(LogicalFileTestBase):
    """
    Exhaustive LogicalFile tests.
    """

    def setUp(self):
        super(TestLogicalFile, self).setUp()
        self.files = {'data':{'path' : self.testdir + "/data",
                              'class': 'LMI_DataFile',
                              'props': {'Readable':True,
                                        'Writeable':False,
                                        'Executable':True,
                                        'SELinuxCurrentContext':'testlf_u:testlf_r:testlf_data_t:s0',
                                        'SELinuxExpectedContext':''}},
                      'dir':{'path' : self.testdir + "/dir",
                             'class': 'LMI_UnixDirectory',
                             'props': {}},
                      'hardlink':{'path' : self.testdir + "/hardlink",
                                  'class': 'LMI_DataFile',
                                  'props': {'Readable':True,
                                            'Writeable':False,
                                            'Executable':True}},
                      'symlink':{'path' : self.testdir + "/symlink",
                                 'class': 'LMI_SymbolicLink',
                                 'props': {'SELinuxCurrentContext':'testlf_u:testlf_r:testlf_symlink_t:s0',
                                           'SELinuxExpectedContext':''}},
                      'fifo':{'path' : self.testdir + "/fifo",
                              'class': 'LMI_FIFOPipeFile',
                              'props': {}},
                      'chdev':{'path' : self.testdir + "/chdev",
                               'class': 'LMI_UnixDeviceFile',
                               'props': {'DeviceMajor':2, 'DeviceMinor':4,
                                         'DeviceFileType':3}},
                      'bldev':{'path' : self.testdir + "/bldev",
                               'class': 'LMI_UnixDeviceFile',
                               'props': {'DeviceMajor':3, 'DeviceMinor':5,
                                         'DeviceFileType':2}},
                      '..':{'path' : os.path.realpath(self.testdir + "/.."),
                            'class': 'LMI_UnixDirectory',
                            'props': {}}}

        self.transient_file = {'path' : self.testdir + "/transient",
                               'class' : 'LMI_UnixDirectory',
                               'props' : {'FSCreationClassName' : 'LMI_TransientFileSystem',
                                          'FSName' : 'PATH=' + self.testdir + "/transient"}}

        self.num_files = len(self.files.keys()) + 1

        self.cop = wbem.CIMInstanceName(classname='LMI_UnixDirectory',
                                          namespace='root/cimv2',
                                          keybindings={
                                              'CSCreationClassName':self.system_cs_name,
                                              'CSName':self.SYSTEM_NAME,
                                              'FSCreationClassName':'LMI_LocalFileSystem',
                                              'FSName':self.fsname,
                                              'CreationClassName':'LMI_UnixDirectory',
                                              'Name':self.testdir
                                          })

        conf_path = '/etc/openlmi/logicalfile/logicalfile.conf'
        self.configurations = dict()
        conf = Configuration(conf_path)
        conf.add('LMI_UnixDirectory', [('AllowMkdir', 'True')])
        self.configurations['mkdir'] = conf

        conf = Configuration(conf_path)
        conf.add('LMI_UnixDirectory', [('AllowRmdir', 'True')])
        self.configurations['rmdir'] = conf

        conf = Configuration(conf_path)
        conf.add('LMI_SymbolicLink', [('AllowSymlink', 'True')])
        self.configurations['symlink'] = conf

        self._prepare()

    def tearDown(self):
        self._cleanup()

    def _prepare(self):
        def check_output(command):
            '''
            Python < 2.7 doesn't have subprocess check_output, simulate it
            if necessary
            '''
            if hasattr(subprocess, 'check_output'):
                return subprocess.check_output(command)
            else:
                return subprocess.Popen(command, stdout=subprocess.PIPE).communicate()[0]

        try:
            os.stat(self.testdir)
        except OSError:
            os.makedirs(self.testdir)
            data_file = self.files['data']['path']
            data_props = self.files['data']['props']
            f = open(data_file, "w+")
            f.write("hello")
            f.close()
            os.chmod(data_file, 0550)
            if self.selinux_enabled:
                labels = data_props['SELinuxCurrentContext'].split(':')
                out = check_output(['chcon', '-h',
                                               '-u', labels[0],
                                               '-r', labels[1],
                                               '-t', labels[2], data_file])
                out = check_output(['matchpathcon', '-n', data_file])
                data_props['SELinuxExpectedContext'] = out[:-1] # remove \n
            os.mkdir(self.files['dir']['path'])
            os.link(data_file, self.files['hardlink']['path'])
            slink_file = self.files['symlink']['path']
            slink_props = self.files['symlink']['props']
            os.symlink(data_file, slink_file)
            if self.selinux_enabled:
                labels = slink_props['SELinuxCurrentContext'].split(':')
                out = check_output(['chcon', '-h',
                                               '-u', labels[0],
                                               '-r', labels[1],
                                               '-t', labels[2], slink_file])
                out = check_output(['matchpathcon', '-n', slink_file])
                slink_props['SELinuxExpectedContext'] = out[:-1] # remove \n
            os.mkfifo(self.files['fifo']['path'])
            chdev = self.files['chdev']
            chdev_device = os.makedev(chdev['props']['DeviceMajor'], chdev['props']['DeviceMinor'])
            os.mknod(chdev['path'], 0666 | stat.S_IFCHR, chdev_device)
            bldev = self.files['bldev']
            bldev_device = os.makedev(bldev['props']['DeviceMajor'], bldev['props']['DeviceMinor'])
            os.mknod(bldev['path'], 0666 | stat.S_IFBLK, bldev_device)
            transient_file = self.transient_file['path']
            os.mkdir(transient_file)
            subprocess.call(['mount', '-t', 'tmpfs', 'tmpfs', transient_file, '-o', 'size=1M'])

    def _cleanup(self):
        subprocess.call(['umount', self.transient_file['path']])
        shutil.rmtree(self.testdir)
        for conf in self.configurations.values():
            conf.dispose()

    def test_lmi_directorycontainsfile(self):
        assoc_class = 'LMI_DirectoryContainsFile'
        ### Associators and AssociatorNames
        for assoc_method in [self.wbemconnection.Associators, self.wbemconnection.AssociatorNames]:
            assocs = assoc_method(self.cop, AssocClass=assoc_class)
            self.assertEquals(len(assocs), self.num_files)
            for k, f in self.files.iteritems():
                # test that the files are actually there and have the correct class name
                match = filter(lambda a: a['Name'] == f['path'], assocs)
                self.assertEquals(len(match), 1)
                self.assertEquals(match[0].classname, f['class'])
                if not isinstance(match[0], wbem.CIMInstanceName):
                    # test some selected properties
                    if k == 'data' or k == 'hardlink':
                        self.assertEquals(match[0]['Readable'], f['props']['Readable'])
                        self.assertEquals(match[0]['Writeable'], f['props']['Writeable'])
                        self.assertEquals(match[0]['Executable'], f['props']['Executable'])
                    if k == 'chdev' or k == 'bldev':
                        self.assertEqual(match[0]['DeviceMajor'], str(f['props']['DeviceMajor']))
                        self.assertEqual(match[0]['DeviceMinor'], str(f['props']['DeviceMinor']))
                        self.assertEqual(match[0]['DeviceFileType'], f['props']['DeviceFileType'])
                    if k == 'symlink':
                        self.assertEqual(match[0]['TargetFile'], self.files['data']['path'])

                # test the other side of LMI_DirectoryContainsFile
                if k != '..':
                    cop_file = wbem.CIMInstanceName(classname=f['class'],
                                                  namespace='root/cimv2',
                                                  keybindings={
                                                      'CSCreationClassName':self.system_cs_name,
                                                      'CSName':self.SYSTEM_NAME,
                                                      'FSCreationClassName':'LMI_LocalFileSystem',
                                                      'FSName':self.fsname,
                                                      'CreationClassName':f['class'],
                                                      'Name':f['path']
                                                  })
                    assocs_file = assoc_method(cop_file,
                                               AssocClass=assoc_class,
                                               Role='PartComponent',
                                               ResultRole='GroupComponent',
                                               ResultClass=self.files['..']['class'])
                    self.assertEquals(len(assocs_file), 1)
                    self.assertEquals(assocs_file[0].classname, 'LMI_UnixDirectory')
                    self.assertEquals(assocs_file[0]['Name'], self.testdir)
                    # wrong Role
                    assocs_file = assoc_method(cop_file, AssocClass=assoc_class, Role='GroupComponent')
                    self.assertEquals(assocs_file, [])
                    # wrong ResultRole
                    assocs_file = assoc_method(cop_file, AssocClass=assoc_class, ResultRole='PartComponent')
                    self.assertEquals(assocs_file, [])
                    # good role and ResultRole, wrong ResultClass
                    assocs_file = assoc_method(cop_file,
                                               AssocClass=assoc_class,
                                               Role='PartComponent',
                                               ResultRole='GroupComponent',
                                               ResultClass=self.system_cs_name)
                    self.assertEquals(assocs_file, [])

        ### References and ReferenceNames
        for assoc_method in [self.wbemconnection.References, self.wbemconnection.ReferenceNames]:
            assocs = assoc_method(self.cop, ResultClass='LMI_DirectoryContainsFile')
            self.assertEquals(len(assocs), self.num_files)
            for k, f in self.files.iteritems():
                # test that the files are actually there and have the correct class name
                match = filter(lambda a: a['PartComponent']['Name'] == f['path'], assocs)
                self.assertEquals(len(match), 1)
                match_name = match[0]['PartComponent']
                self.assertEquals(match_name.classname, f['class'])
                # test the other side of LMI_DirectoryContainsFile
                if k != '..' and k != 'dir':
                    cop_file = wbem.CIMInstanceName(classname=f['class'],
                                                  namespace='root/cimv2',
                                                  keybindings={
                                                      'CSCreationClassName':self.system_cs_name,
                                                      'CSName':self.SYSTEM_NAME,
                                                      'FSCreationClassName':'LMI_LocalFileSystem',
                                                      'FSName':self.fsname,
                                                      'CreationClassName':f['class'],
                                                      'Name':f['path']
                                                  })
                    assocs_file = assoc_method(cop_file, ResultClass='LMI_DirectoryContainsFile')
                    self.assertEquals(len(assocs_file), 1)
                    file_name = assocs_file[0]['GroupComponent']
                    self.assertEquals(file_name.classname, 'LMI_UnixDirectory')
                    self.assertEquals(file_name['Name'], self.testdir)

    def test_lmi_fileidentity(self):
        ### Associators and AssociatorNames
        assoc_class = 'LMI_FileIdentity'
        for assoc_method in [self.wbemconnection.Associators, self.wbemconnection.AssociatorNames]:
            assocs = assoc_method(self.cop, AssocClass='LMI_DirectoryContainsFile')
            self.assertEquals(len(assocs), self.num_files)
            for k, f in self.files.iteritems():
                match = filter(lambda a: a['Name'] == f['path'], assocs)
                self.assertEquals(len(match), 1)
                self.assertEquals(match[0].classname, f['class'])

                if isinstance(match[0], wbem.CIMInstanceName):
                    match[0].path = match[0]
                ## SystemElement - LMI_UnixFile
                assocs_ident = assoc_method(match[0].path,
                                            AssocClass=assoc_class,
                                            Role='SystemElement',
                                            ResultRole='SameElement',
                                            ResultClass='LMI_UnixFile')
                self.assertEquals(len(assocs_ident), 1)
                self.assertEquals(assocs_ident[0]['LFName'], f['path'])
                self.assertEquals(assocs_ident[0]['LFCreationClassName'], f['class'])
                if not isinstance(match[0], wbem.CIMInstanceName):
                    if self.selinux_enabled and (k == 'data' or k == 'symlink'):
                        self.assertEquals(assocs_ident[0]['SELinuxCurrentContext'],
                                          f['props']['SELinuxCurrentContext'])
                        self.assertEquals(assocs_ident[0]['SELinuxExpectedContext'],
                                          f['props']['SELinuxExpectedContext'])
                # wrong Role
                assocs_ident = assoc_method(match[0].path, AssocClass=assoc_class, Role='SameElement')
                self.assertEquals(assocs_ident, [])
                # wrong ResultRole
                assocs_ident = assoc_method(match[0].path, AssocClass=assoc_class, ResultRole='SystemElement')
                self.assertEquals(assocs_ident, [])
                # good role and ResultRole, wrong ResultClass
                assocs_ident = assoc_method(match[0].path,
                                      AssocClass=assoc_class,
                                      Role='SystemElement',
                                      ResultRole='SameElement',
                                      ResultClass=self.system_cs_name)
                self.assertEquals(assocs_ident, [])

                ## SameElement - CIM_LogicalFile
                cop_ident = wbem.CIMInstanceName(classname='LMI_UnixFile',
                                                   namespace='root/cimv2',
                                                   keybindings={
                                                       'CSCreationClassName':self.system_cs_name,
                                                       'CSName':self.SYSTEM_NAME,
                                                       'FSCreationClassName':'LMI_LocalFileSystem',
                                                       'FSName':self.fsname,
                                                       'LFCreationClassName':f['class'],
                                                       'LFName':f['path']
                                                   })
                assocs_ident = assoc_method(cop_ident,
                                            AssocClass=assoc_class,
                                            Role='SameElement',
                                            ResultRole='SystemElement',
                                            ResultClass=f['class'])
                self.assertEquals(len(assocs_ident), 1)
                self.assertEquals(assocs_ident[0]['Name'], f['path'])
                self.assertEquals(assocs_ident[0]['CreationClassName'], f['class'])
                # wrong Role
                assocs_ident = assoc_method(cop_ident, AssocClass=assoc_class, Role='SystemElement')
                self.assertEquals(assocs_ident, [])
                # wrong ResultRole
                assocs_ident = assoc_method(cop_ident, AssocClass=assoc_class, ResultRole='SameElement')
                self.assertEquals(assocs_ident, [])
                # good role and ResultRole, wrong ResultClass
                assocs_ident = assoc_method(cop_ident,
                                      AssocClass=assoc_class,
                                      Role='SameElement',
                                      ResultRole='SystemElement',
                                      ResultClass=self.system_cs_name)
                self.assertEquals(assocs_ident, [])

        ### References and ReferenceNames
        for assoc_method in [self.wbemconnection.References, self.wbemconnection.ReferenceNames]:
            assocs = assoc_method(self.cop, ResultClass='LMI_DirectoryContainsFile')
            self.assertEquals(len(assocs), self.num_files)
            for k, f in self.files.iteritems():
                match = filter(lambda a: a['PartComponent']['Name'] == f['path'], assocs)
                self.assertEquals(len(match), 1)
                match_name = match[0]['PartComponent']
                self.assertEquals(match_name['CreationClassName'], f['class'])

                ## SystemElement - LMI_UnixFile
                assocs_ident = assoc_method(match_name, ResultClass=assoc_class)
                self.assertEquals(len(assocs_ident), 1)
                ident_name = assocs_ident[0]['SameElement']
                self.assertEquals(ident_name['LFName'], f['path'])
                self.assertEquals(ident_name['LFCreationClassName'], f['class'])


                ## SameElement - CIM_LogicalFile
                cop_ident = wbem.CIMInstanceName(classname='LMI_UnixFile',
                                                   namespace='root/cimv2',
                                                   keybindings={
                                                       'CSCreationClassName':self.system_cs_name,
                                                       'CSName':self.SYSTEM_NAME,
                                                       'FSCreationClassName':'LMI_LocalFileSystem',
                                                       'FSName':self.fsname,
                                                       'LFCreationClassName':f['class'],
                                                       'LFName':f['path']
                                                   })
                assocs_ident = assoc_method(cop_ident, ResultClass=assoc_class)
                self.assertEquals(len(assocs_ident), 1)
                ident_name = assocs_ident[0]['SystemElement']
                self.assertEquals(ident_name['Name'], f['path'])
                self.assertEquals(ident_name['CreationClassName'], f['class'])

    def test_lmi_rootdirectory(self):
        assoc_class = 'LMI_RootDirectory'
        ### Associators and AssociatorNames
        cop = self.cop.copy()
        cop.keybindings['Name'] = '/'
        for assoc_method in [self.wbemconnection.Associators, self.wbemconnection.AssociatorNames]:
            ## PartComponent - CIM_ComputerSystem
            assocs = assoc_method(cop,
                                  AssocClass=assoc_class,
                                  Role='PartComponent',
                                  ResultRole='GroupComponent',
                                  ResultClass=self.system_cs_name)
            self.assertEquals(len(assocs), 1)
            system = assocs[0]
            self.assertEquals(system['CreationClassName'], self.system_cs_name)
            self.assertEquals(system['Name'], self.SYSTEM_NAME)
            # wrong Role
            assocs = assoc_method(cop, AssocClass=assoc_class, Role='GroupComponent')
            self.assertEquals(assocs, [])
            # wrong ResultRole
            assocs = assoc_method(cop, AssocClass=assoc_class, ResultRole='PartComponent')
            self.assertEquals(assocs, [])
            # good role and ResultRole, wrong ResultClass
            assocs = assoc_method(cop,
                                  AssocClass=assoc_class,
                                  Role='PartComponent',
                                  ResultRole='GroupComponent',
                                  ResultClass='LMI_UnixDirectory')
            self.assertEquals(assocs, [])

            ## GroupComponent - LMI_UnixDirectory
            if isinstance(system, wbem.CIMInstanceName):
                system.path = system
            assocs = assoc_method(system.path,
                                  AssocClass=assoc_class,
                                  Role='GroupComponent',
                                  ResultRole='PartComponent',
                                  ResultClass='LMI_UnixDirectory')
            self.assertEquals(len(assocs), 1)
            self.assertEquals(assocs[0]['Name'], '/')
            # wrong Role
            assocs = assoc_method(system.path, AssocClass=assoc_class, Role='PartComponent')
            self.assertEquals(assocs, [])
            # wrong ResultRole
            assocs = assoc_method(system.path, AssocClass=assoc_class, ResultRole='GroupComponent')
            self.assertEquals(assocs, [])
            # good role and ResultRole, wrong ResultClass
            assocs = assoc_method(system.path,
                                  AssocClass=assoc_class,
                                  Role='GroupComponent',
                                  ResultRole='PartComponent',
                                  ResultClass=self.system_cs_name)
            self.assertEquals(assocs, [])


        ### References and ReferenceNames
        for assoc_method in [self.wbemconnection.References, self.wbemconnection.ReferenceNames]:
            ## PartComponent - CIM_ComputerSystem
            assocs = assoc_method(cop, ResultClass=assoc_class)
            self.assertEquals(len(assocs), 1)
            system = assocs[0]['GroupComponent']
            self.assertEquals(system['CreationClassName'], self.system_cs_name)
            self.assertEquals(system['Name'], self.SYSTEM_NAME)

            ## GroupComponent - LMI_UnixDirectory
            assocs = assoc_method(system, ResultClass=assoc_class)
            self.assertEquals(len(assocs), 1)
            self.assertEquals(assocs[0]['PartComponent']['Name'], '/')

        ### EnumerateInstances and GetInstance
        insts = self.wbemconnection.EnumerateInstances(assoc_class)
        self.assertEquals(len(insts), 1)
        system = insts[0]['GroupComponent']
        rootdir = insts[0]['PartComponent']
        self.assertEquals(system['CreationClassName'], self.system_cs_name)
        self.assertEquals(system['Name'], self.SYSTEM_NAME)
        self.assertEquals(rootdir['CSCreationClassName'], self.system_cs_name)
        self.assertEquals(rootdir['CSName'], self.SYSTEM_NAME)
        self.assertEquals(rootdir['Name'], '/')

        inst_cop = wbem.CIMInstanceName(classname=assoc_class,
                                          namespace='root/cimv2',
                                          keybindings={
                                              'GroupComponent':system,
                                              'PartComponent':rootdir
                                          })
        inst = self.wbemconnection.GetInstance(inst_cop)
        self.assertTrue(inst is not None)
        system = inst['GroupComponent']
        rootdir = inst['PartComponent']
        self.assertEquals(system['CreationClassName'], self.system_cs_name)
        self.assertEquals(system['Name'], self.SYSTEM_NAME)
        self.assertEquals(rootdir['CSCreationClassName'], self.system_cs_name)
        self.assertEquals(rootdir['CSName'], self.SYSTEM_NAME)
        self.assertEquals(rootdir['Name'], '/')

    def _mkdir(self, path):
        cop = self.cop.copy()
        cop['Name'] = path
        inst = wbem.CIMInstance('LMI_UnixDirectory', cop.keybindings)
        self.wbemconnection.CreateInstance(inst)

    def _test_mkdir(self, enabled_by_configuration):
        classname = 'LMI_UnixDirectory'
        path = self.testdir + '/mkdir-test'
        cop = self.cop.copy()
        cop['Name'] = path

        try:
            self._mkdir(path)
            inst = self.wbemconnection.GetInstance(cop)
            self.assertEquals(inst.classname, classname)
            self.assertEquals(inst['CreationClassName'], classname)
            self.assertEquals(inst['Name'], path)
            self._rmdir(path)
        except CIMError as pe:
            if enabled_by_configuration:
                self.fail(pe[1])
            else:
                self.assertEquals(pe[1],
                                  "CIM_ERR_FAILED: Can't mkdir: disabled by provider configuration")

        self.assertRaises(CIMError,
                          self._mkdir,
                          '/cant/create/me')

    def test_mkdir(self):
        conf = self.configurations['mkdir']
        # should succeed
        conf.save()
        self.restart_cim()
        self._test_mkdir(True)
        conf.dispose()
        # should fail
        conf.add('LMI_UnixDirectory', [('AllowMkdir', 'False')])
        conf.save()
        self.restart_cim()
        self._test_mkdir(False)
        conf.dispose()

    def _rmdir(self, path):
        cop = self.cop.copy()
        cop['Name'] = path
        inst = wbem.CIMInstance('LMI_UnixDirectory', cop.keybindings)
        self.wbemconnection.DeleteInstance(cop)

    def _test_rmdir(self, enabled_by_configuration):
        path = self.files['dir']['path']
        if not os.path.exists(path):
            self._mkdir(path)

        try:
            self._rmdir(path)
        except CIMError as pe:
            if enabled_by_configuration:
                self.fail(pe[1])
            else:
                self.assertEquals(pe[1],
                                  "CIM_ERR_FAILED: Can't rmdir: disabled by provider configuration")

        self.assertRaises(CIMError,
                          self._rmdir,
                          '/cant/remove/me')

    def test_rmdir(self):
        conf = self.configurations['rmdir']
        # should succeed
        conf.save()
        self.restart_cim()
        self._test_rmdir(True)
        conf.dispose()
        # should fail
        conf.add('LMI_UnixDirectory', [('AllowRmdir', 'False')])
        conf.save()
        self.restart_cim()
        self._test_rmdir(False)
        conf.dispose()

    def _test_create_symlink(self, enabled_by_configuration):
        classname = 'LMI_SymbolicLink'
        cop = self.cop.copy()
        cop.classname = classname
        target = self.files['data']['path']
        name = self.testdir + '/target-symlink'

        def create_symlink(target, path):
            cop['Name'] = path
            cop['TargetFile'] = target
            inst = wbem.CIMInstance(classname, cop.keybindings)
            self.wbemconnection.CreateInstance(inst)

        try:
            create_symlink(target, name)
            inst = self.wbemconnection.GetInstance(cop)
            self.assertEquals(inst.classname, classname)
            self.assertEquals(inst['CreationClassName'], classname)
            self.assertEquals(inst['TargetFile'], target)
            self.assertEquals(inst['Name'], name)
        except CIMError as pe:
            if enabled_by_configuration:
                self.fail(pe[1])
            else:
                self.assertEquals(pe[1],
                                  "CIM_ERR_FAILED: Can't create symlink: disabled by provider configuration")

        self.assertRaises(CIMError,
                          create_symlink,
                          target,
                          name)

    def test_create_symlink(self):
        conf = self.configurations['symlink']
        # should succeed
        conf.save()
        self.restart_cim()
        self._test_create_symlink(True)
        conf.dispose()
        # should fail
        conf.add('LMI_SymbolicLink',  [('AllowSymlink', 'False')])
        conf.save()
        self.restart_cim()
        self._test_create_symlink(False)
        conf.dispose()

    # for now, this test just checks if FSName, FSCreationClassName,
    # CreationClassName and LFCreationClassName are properly ignored; empty
    # strings are used, since they should represent "ignored" pretty well
    def _test_missing_or_wrong_properties(self, is_unixfile):
        testfile = self.files['data']
        if is_unixfile:
            prefix = 'LF'
            clsname = 'LMI_UnixFile'
        else:
            prefix = ''
            clsname = 'LMI_DataFile'
        cop = wbem.CIMInstanceName(classname=clsname,
                                     namespace='root/cimv2',
                                     keybindings={
                                         'CSCreationClassName':self.system_cs_name,
                                         'CSName':self.SYSTEM_NAME,
                                         'FSCreationClassName':'',
                                         'FSName':'',
                                     })

        cop.keybindings[prefix+'Name'] = testfile['path']
        cop.keybindings[prefix+'CreationClassName'] = ''

        try:
            self.wbemconnection.GetInstance(cop)
        except CIMError as pe:
            self.fail(pe[1])

    def test_unixfile_missing_or_wrong_properties(self):
        self._test_missing_or_wrong_properties(True)

    def test_logicalfile_missing_or_wrong_properties(self):
        self._test_missing_or_wrong_properties(False)

    def test_transient_file(self):
        cop = self.cop.copy()
        cop['Name'] = self.transient_file['path']
        cop['FSName'] = ''
        cop['FSCreationClassName'] = ''

        try:
            print type(cop)
            inst = self.wbemconnection.GetInstance(cop)
        except CIMError as pe:
            self.fail(pe[1])

        self.assertEquals(inst['FSCreationClassName'], 'LMI_TransientFileSystem')
        self.assertEquals(inst['FSName'], 'PATH=' + cop['Name'])

if __name__ == '__main__':
    unittest.main()
