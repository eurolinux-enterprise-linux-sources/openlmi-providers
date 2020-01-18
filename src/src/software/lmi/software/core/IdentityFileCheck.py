# -*- encoding: utf-8 -*-
# Software Management Providers
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
# Authors: Michal Minar <miminar@redhat.com>
#

"""
Just a common functionality related to SoftwareIdentityFileCheck provider.
"""

import hashlib
import os
import pywbem
import stat
import yum

from lmi.providers import cmpi_logging
from lmi.software import util
from lmi.software.yumdb import errors
from lmi.software.yumdb import jobs
from lmi.software.yumdb import packagecheck
from lmi.software.yumdb import packageinfo
from lmi.software.yumdb import YumDB

LOG = cmpi_logging.get_logger(__name__)

class Values(object):
    class TargetOperatingSystem(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        MACOS = pywbem.Uint16(2)
        ATTUNIX = pywbem.Uint16(3)
        DGUX = pywbem.Uint16(4)
        DECNT = pywbem.Uint16(5)
        Tru64_UNIX = pywbem.Uint16(6)
        OpenVMS = pywbem.Uint16(7)
        HPUX = pywbem.Uint16(8)
        AIX = pywbem.Uint16(9)
        MVS = pywbem.Uint16(10)
        OS400 = pywbem.Uint16(11)
        OS_2 = pywbem.Uint16(12)
        JavaVM = pywbem.Uint16(13)
        MSDOS = pywbem.Uint16(14)
        WIN3x = pywbem.Uint16(15)
        WIN95 = pywbem.Uint16(16)
        WIN98 = pywbem.Uint16(17)
        WINNT = pywbem.Uint16(18)
        WINCE = pywbem.Uint16(19)
        NCR3000 = pywbem.Uint16(20)
        NetWare = pywbem.Uint16(21)
        OSF = pywbem.Uint16(22)
        DC_OS = pywbem.Uint16(23)
        Reliant_UNIX = pywbem.Uint16(24)
        SCO_UnixWare = pywbem.Uint16(25)
        SCO_OpenServer = pywbem.Uint16(26)
        Sequent = pywbem.Uint16(27)
        IRIX = pywbem.Uint16(28)
        Solaris = pywbem.Uint16(29)
        SunOS = pywbem.Uint16(30)
        U6000 = pywbem.Uint16(31)
        ASERIES = pywbem.Uint16(32)
        HP_NonStop_OS = pywbem.Uint16(33)
        HP_NonStop_OSS = pywbem.Uint16(34)
        BS2000 = pywbem.Uint16(35)
        LINUX = pywbem.Uint16(36)
        Lynx = pywbem.Uint16(37)
        XENIX = pywbem.Uint16(38)
        VM = pywbem.Uint16(39)
        Interactive_UNIX = pywbem.Uint16(40)
        BSDUNIX = pywbem.Uint16(41)
        FreeBSD = pywbem.Uint16(42)
        NetBSD = pywbem.Uint16(43)
        GNU_Hurd = pywbem.Uint16(44)
        OS9 = pywbem.Uint16(45)
        MACH_Kernel = pywbem.Uint16(46)
        Inferno = pywbem.Uint16(47)
        QNX = pywbem.Uint16(48)
        EPOC = pywbem.Uint16(49)
        IxWorks = pywbem.Uint16(50)
        VxWorks = pywbem.Uint16(51)
        MiNT = pywbem.Uint16(52)
        BeOS = pywbem.Uint16(53)
        HP_MPE = pywbem.Uint16(54)
        NextStep = pywbem.Uint16(55)
        PalmPilot = pywbem.Uint16(56)
        Rhapsody = pywbem.Uint16(57)
        Windows_2000 = pywbem.Uint16(58)
        Dedicated = pywbem.Uint16(59)
        OS_390 = pywbem.Uint16(60)
        VSE = pywbem.Uint16(61)
        TPF = pywbem.Uint16(62)
        Windows__R__Me = pywbem.Uint16(63)
        Caldera_Open_UNIX = pywbem.Uint16(64)
        OpenBSD = pywbem.Uint16(65)
        Not_Applicable = pywbem.Uint16(66)
        Windows_XP = pywbem.Uint16(67)
        z_OS = pywbem.Uint16(68)
        Microsoft_Windows_Server_2003 = pywbem.Uint16(69)
        Microsoft_Windows_Server_2003_64_Bit = pywbem.Uint16(70)
        Windows_XP_64_Bit = pywbem.Uint16(71)
        Windows_XP_Embedded = pywbem.Uint16(72)
        Windows_Vista = pywbem.Uint16(73)
        Windows_Vista_64_Bit = pywbem.Uint16(74)
        Windows_Embedded_for_Point_of_Service = pywbem.Uint16(75)
        Microsoft_Windows_Server_2008 = pywbem.Uint16(76)
        Microsoft_Windows_Server_2008_64_Bit = pywbem.Uint16(77)
        FreeBSD_64_Bit = pywbem.Uint16(78)
        RedHat_Enterprise_Linux = pywbem.Uint16(79)
        RedHat_Enterprise_Linux_64_Bit = pywbem.Uint16(80)
        Solaris_64_Bit = pywbem.Uint16(81)
        SUSE = pywbem.Uint16(82)
        SUSE_64_Bit = pywbem.Uint16(83)
        SLES = pywbem.Uint16(84)
        SLES_64_Bit = pywbem.Uint16(85)
        Novell_OES = pywbem.Uint16(86)
        Novell_Linux_Desktop = pywbem.Uint16(87)
        Sun_Java_Desktop_System = pywbem.Uint16(88)
        Mandriva = pywbem.Uint16(89)
        Mandriva_64_Bit = pywbem.Uint16(90)
        TurboLinux = pywbem.Uint16(91)
        TurboLinux_64_Bit = pywbem.Uint16(92)
        Ubuntu = pywbem.Uint16(93)
        Ubuntu_64_Bit = pywbem.Uint16(94)
        Debian = pywbem.Uint16(95)
        Debian_64_Bit = pywbem.Uint16(96)
        Linux_2_4_x = pywbem.Uint16(97)
        Linux_2_4_x_64_Bit = pywbem.Uint16(98)
        Linux_2_6_x = pywbem.Uint16(99)
        Linux_2_6_x_64_Bit = pywbem.Uint16(100)
        Linux_64_Bit = pywbem.Uint16(101)
        Other_64_Bit = pywbem.Uint16(102)
        Microsoft_Windows_Server_2008_R2 = pywbem.Uint16(103)
        VMware_ESXi = pywbem.Uint16(104)
        Microsoft_Windows_7 = pywbem.Uint16(105)
        CentOS_32_bit = pywbem.Uint16(106)
        CentOS_64_bit = pywbem.Uint16(107)
        Oracle_Enterprise_Linux_32_bit = pywbem.Uint16(108)
        Oracle_Enterprise_Linux_64_bit = pywbem.Uint16(109)
        eComStation_32_bitx = pywbem.Uint16(110)
        Microsoft_Windows_Server_2011 = pywbem.Uint16(111)
        Microsoft_Windows_Server_2012 = pywbem.Uint16(113)
        Microsoft_Windows_8 = pywbem.Uint16(114)
        Microsoft_Windows_8_64_bit = pywbem.Uint16(115)
        _reverse_map = {
                0   : 'Unknown',
                1   : 'Other',
                2   : 'MACOS',
                3   : 'ATTUNIX',
                4   : 'DGUX',
                5   : 'DECNT',
                6   : 'Tru64 UNIX',
                7   : 'OpenVMS',
                8   : 'HPUX',
                9   : 'AIX',
                10  : 'MVS',
                11  : 'OS400',
                12  : 'OS/2',
                13  : 'JavaVM',
                14  : 'MSDOS',
                15  : 'WIN3x',
                16  : 'WIN95',
                17  : 'WIN98',
                18  : 'WINNT',
                19  : 'WINCE',
                20  : 'NCR3000',
                21  : 'NetWare',
                22  : 'OSF',
                23  : 'DC/OS',
                24  : 'Reliant UNIX',
                25  : 'SCO UnixWare',
                26  : 'SCO OpenServer',
                27  : 'Sequent',
                28  : 'IRIX',
                29  : 'Solaris',
                30  : 'SunOS',
                31  : 'U6000',
                32  : 'ASERIES',
                33  : 'HP NonStop OS',
                34  : 'HP NonStop OSS',
                35  : 'BS2000',
                36  : 'LINUX',
                37  : 'Lynx',
                38  : 'XENIX',
                39  : 'VM',
                40  : 'Interactive UNIX',
                41  : 'BSDUNIX',
                42  : 'FreeBSD',
                43  : 'NetBSD',
                44  : 'GNU Hurd',
                45  : 'OS9',
                46  : 'MACH Kernel',
                47  : 'Inferno',
                48  : 'QNX',
                49  : 'EPOC',
                50  : 'IxWorks',
                51  : 'VxWorks',
                52  : 'MiNT',
                53  : 'BeOS',
                54  : 'HP MPE',
                55  : 'NextStep',
                56  : 'PalmPilot',
                57  : 'Rhapsody',
                58  : 'Windows 2000',
                59  : 'Dedicated',
                60  : 'OS/390',
                61  : 'VSE',
                62  : 'TPF',
                63  : 'Windows (R) Me',
                64  : 'Caldera Open UNIX',
                65  : 'OpenBSD',
                66  : 'Not Applicable',
                67  : 'Windows XP',
                68  : 'z/OS',
                69  : 'Microsoft Windows Server 2003',
                70  : 'Microsoft Windows Server 2003 64-Bit',
                71  : 'Windows XP 64-Bit',
                72  : 'Windows XP Embedded',
                73  : 'Windows Vista',
                74  : 'Windows Vista 64-Bit',
                75  : 'Windows Embedded for Point of Service',
                76  : 'Microsoft Windows Server 2008',
                77  : 'Microsoft Windows Server 2008 64-Bit',
                78  : 'FreeBSD 64-Bit',
                79  : 'RedHat Enterprise Linux',
                80  : 'RedHat Enterprise Linux 64-Bit',
                81  : 'Solaris 64-Bit',
                82  : 'SUSE',
                83  : 'SUSE 64-Bit',
                84  : 'SLES',
                85  : 'SLES 64-Bit',
                86  : 'Novell OES',
                87  : 'Novell Linux Desktop',
                88  : 'Sun Java Desktop System',
                89  : 'Mandriva',
                90  : 'Mandriva 64-Bit',
                91  : 'TurboLinux',
                92  : 'TurboLinux 64-Bit',
                93  : 'Ubuntu',
                94  : 'Ubuntu 64-Bit',
                95  : 'Debian',
                96  : 'Debian 64-Bit',
                97  : 'Linux 2.4.x',
                98  : 'Linux 2.4.x 64-Bit',
                99  : 'Linux 2.6.x',
                100 : 'Linux 2.6.x 64-Bit',
                101 : 'Linux 64-Bit',
                102 : 'Other 64-Bit',
                103 : 'Microsoft Windows Server 2008 R2',
                104 : 'VMware ESXi',
                105 : 'Microsoft Windows 7',
                106 : 'CentOS 32-bit',
                107 : 'CentOS 64-bit',
                108 : 'Oracle Enterprise Linux 32-bit',
                109 : 'Oracle Enterprise Linux 64-bit',
                110 : 'eComStation 32-bitx',
                111 : 'Microsoft Windows Server 2011',
                113 : 'Microsoft Windows Server 2012',
                114 : 'Microsoft Windows 8',
                115 : 'Microsoft Windows 8 64-bit'
        }

    class ChecksumType(object):
        UNKNOWN = pywbem.Uint16(0)
        MD5 = pywbem.Uint16(1)
        SHA_1 = pywbem.Uint16(2)
        RIPE_MD_160 = pywbem.Uint16(3)
        SHA256 = pywbem.Uint16(8)
        SHA384 = pywbem.Uint16(9)
        SHA512 = pywbem.Uint16(10)
        SHA224 = pywbem.Uint16(11)
        _reverse_map = {
                0  : 'UNKNOWN',
                1  : 'MD5',
                2  : 'SHA-1',
                3  : 'RIPE-MD/160',
                8  : 'SHA256',
                9  : 'SHA384',
                10 : 'SHA512',
                11 : 'SHA224'
        }

    class FileType(object):
        Unknown = pywbem.Uint16(0)
        File = pywbem.Uint16(1)
        Directory = pywbem.Uint16(2)
        Symlink = pywbem.Uint16(3)
        FIFO = pywbem.Uint16(4)
        Character_Device = pywbem.Uint16(5)
        Block_Device = pywbem.Uint16(6)
        _reverse_map = {
                0 : 'Unknown',
                1 : 'File',
                2 : 'Directory',
                3 : 'Symlink',
                4 : 'FIFO',
                5 : 'Character Device',
                6 : 'Block Device'
        }

    class FileModeFlags(object):
        Execute_Other = pywbem.Uint8(0)
        Write_Other = pywbem.Uint8(1)
        Read_Other = pywbem.Uint8(2)
        Execute_Group = pywbem.Uint8(3)
        Write_Group = pywbem.Uint8(4)
        Read_Group = pywbem.Uint8(5)
        Execute_User = pywbem.Uint8(6)
        Write_User = pywbem.Uint8(7)
        Read_User = pywbem.Uint8(8)
        Sticky = pywbem.Uint8(9)
        SGID = pywbem.Uint8(10)
        SUID = pywbem.Uint8(11)
        _reverse_map = {
                0  : 'Execute Other',
                1  : 'Write Other',
                2  : 'Read Other',
                3  : 'Execute Group',
                4  : 'Write Group',
                5  : 'Read Group',
                6  : 'Execute User',
                7  : 'Write User',
                8  : 'Read User',
                9  : 'Sticky',
                10 : 'SGID',
                11 : 'SUID'
        }

    class FileModeFlagsOriginal(object):
        Execute_Other = pywbem.Uint8(0)
        Write_Other = pywbem.Uint8(1)
        Read_Other = pywbem.Uint8(2)
        Execute_Group = pywbem.Uint8(3)
        Write_Group = pywbem.Uint8(4)
        Read_Group = pywbem.Uint8(5)
        Execute_User = pywbem.Uint8(6)
        Write_User = pywbem.Uint8(7)
        Read_User = pywbem.Uint8(8)
        Sticky = pywbem.Uint8(9)
        SGID = pywbem.Uint8(10)
        SUID = pywbem.Uint8(11)
        _reverse_map = {
                0  : 'Execute Other',
                1  : 'Write Other',
                2  : 'Read Other',
                3  : 'Execute Group',
                4  : 'Write Group',
                5  : 'Read Group',
                6  : 'Execute User',
                7  : 'Write User',
                8  : 'Read User',
                9  : 'Sticky',
                10 : 'SGID',
                11 : 'SUID'
        }

    class FailedFlags(object):
        Existence = pywbem.Uint16(0)
        FileSize = pywbem.Uint16(1)
        FileMode = pywbem.Uint16(2)
        Checksum = pywbem.Uint16(3)
        Device_Number = pywbem.Uint16(4)
        LinkTarget = pywbem.Uint16(5)
        UserID = pywbem.Uint16(6)
        GroupID = pywbem.Uint16(7)
        Last_Modification_Time = pywbem.Uint16(8)
        _reverse_map = {
                0 : 'Existence',
                1 : 'FileSize',
                2 : 'FileMode',
                3 : 'Checksum',
                4 : 'Device Number',
                5 : 'LinkTarget',
                6 : 'UserID',
                7 : 'GroupID',
                8 : 'Last Modification Time'
        }

    class SoftwareElementState(object):
        Deployable = pywbem.Uint16(0)
        Installable = pywbem.Uint16(1)
        Executable = pywbem.Uint16(2)
        Running = pywbem.Uint16(3)
        _reverse_map = {
                0 : 'Deployable',
                1 : 'Installable',
                2 : 'Executable',
                3 : 'Running'
        }

    class FileTypeOriginal(object):
        Unknown = pywbem.Uint16(0)
        File = pywbem.Uint16(1)
        Directory = pywbem.Uint16(2)
        Symlink = pywbem.Uint16(3)
        FIFO = pywbem.Uint16(4)
        Character_Device = pywbem.Uint16(5)
        Block_Device = pywbem.Uint16(6)
        _reverse_map = {
                0 : 'Unknown',
                1 : 'File',
                2 : 'Directory',
                3 : 'Symlink',
                4 : 'FIFO',
                5 : 'Character Device',
                6 : 'Block Device'
        }

    class Invoke(object):
        """
        This is not an enumeration from mof file, it just gives human
        names to numeric values.
        """
        Satisfied = pywbem.Uint32(0)
        Not_Supported = pywbem.Uint32(1)
        Not_Satisfied = pywbem.Uint32(2)

class FileCheck(object):
    """
    File attribute storage keeping both RPM database metadata and info
    from local filesystem for single file. Most attributes are loaded at the
    time of first request and cached for later use.

    Most of properties return tuple:
        ``(installed, original)``
    where
        ``installed`` is a state of property of installed file, while
        ``original`` is the value stored in RPM database.

    If any of those values are None, it means that property could not be
    retrieved or it's not applicable to file's type.
    """

    def __init__(self, pkg_info, pkg_file, checksum_type):
        """
        :param pkg_file: (``yumdb.packagecheck.PackageFile``) Metadata for
            file loaded from rpm database.
        :param checksum_type: (``int``) Indentificator of checksum algorithm
            corresponding to values in yum.constants.RPM_CHECKSUM_TYPES.
        """
        if not isinstance(pkg_info, packageinfo.PackageInfo):
            raise TypeError("pkg_info must be an instance of PackageInfo")
        if not isinstance(pkg_file, packagecheck.PackageFile):
            raise TypeError("pkg_file must be an instance of PackageFile")
        self._pkg_info = pkg_info
        self._pkg_file = pkg_file
        self._exists = None
        # (md5 checksum, rpm-type checksum) - this is made on first request
        self._checksums = None
        self._checksum_type = checksum_type
        self._link_target = None
        # stores result of os.stat() call
        self._stat = None

    @property
    def pkg_info(self):
        """:rtype: (``PackageInfo``)"""
        return self._pkg_info

    @property
    def pkg_file(self):
        """:rtype: (``PackageFile``)"""
        return self._pkg_file

    @property
    def path(self):
        """Return absolute file path."""
        return self._pkg_file.path

    @property
    def exists(self):
        """Return true if file exists on local file system."""
        if self._exists is None:
            self._exists = os.path.lexists(self._pkg_file.path)
        return self._exists

    @property
    def stat(self):
        """
        Return cached stat object - result of ``os.lstat()``.
        None is returned if file does not exist.
        """
        if self._stat is None and self.exists:
            self._stat = os.lstat(self.path)
        return self._stat

    @property
    def file_type(self):
        """
        Return identifiers of file type for installed and rpm file.
        If the installed file does not exist, None is returned on
        its position.
        :rtype: (``tuple``) Pair of (installed, original).
        """
        fm = self.file_mode[0]
        if fm is None:
            ft = None
        elif stat.S_ISLNK(fm):
            ft = packagecheck.FILE_TYPE_SYMLINK
        elif stat.S_ISDIR(fm):
            ft = packagecheck.FILE_TYPE_DIRECTORY
        elif stat.S_ISFIFO(fm):
            ft = packagecheck.FILE_TYPE_FIFO
        elif stat.S_ISCHR(fm):
            ft = packagecheck.FILE_TYPE_CHARACTER_DEVICE
        elif stat.S_ISBLK(fm):
            ft = packagecheck.FILE_TYPE_BLOCK_DEVICE
        elif stat.S_ISREG(fm):
            ft = packagecheck.FILE_TYPE_FILE
        else:
            ft = packagecheck.FILE_TYPE_UNKNOWN
        return (ft, self._pkg_file.file_type)

    @property
    def md5_checksum(self):
        """
        Return md5 checksum string of installed file.
        This is computed on first access either to this or ``file_checksum``
        property and cached for later access.
        :rtype: (``str``)
        """
        if self._checksums is None:
            self.make_checksums()
        return self._checksums[0] if self._checksums else None

    @property
    def file_checksum_type(self):
        """
        Return identifier of hash algorithm used in rpm package.
        :rtype: (``int``)
        """
        return self._checksum_type

    @property
    def file_checksum(self):
        """
        Return checksum strings for installed and rpm file.
        This is computed on first access either to this or ``md5_checksum``
        property and cached for later access.
        :rtype: (``tuple``) Pair of (installed, original).
        """
        if self._checksums is None:
            self.make_checksums()
        return ( self._checksums[1] if self._checksums else None
               , self._pkg_file.checksum)

    @property
    def file_size(self):
        """:rtype: (``tuple``) Pair of (installed, original)."""
        return (self.getstat('size'), self._pkg_file.size)

    @property
    def file_mode(self):
        """:rtype: (``tuple``) Pair of (installed, original)."""
        return (self.getstat('mode'), self._pkg_file.mode)

    @property
    def device(self):
        """:rtype: (``tuple``) Pair of (installed, original)."""
        return (self.getstat('dev'), self._pkg_file.device)

    @property
    def link_target(self):
        """:rtype: (``tuple``) Pair of (installed, original)."""
        if (   self.file_type[0] == packagecheck.FILE_TYPE_SYMLINK
           and self._link_target is None):
            self._link_target = os.readlink(self.path)
        return (self._link_target, self._pkg_file.link_target)

    @property
    def user_id(self):
        """:rtype: (``tuple``) Pair of (installed, original)."""
        return (self.getstat('uid'), self._pkg_file.uid)

    @property
    def group_id(self):
        """:rtype: (``tuple``) Pair of (installed, original)."""
        return (self.getstat('gid'), self._pkg_file.gid)

    @property
    def last_modification_time(self):
        """:rtype: (``tuple``) Pair of (installed, original)."""
        return (self.getstat('mtime'), self._pkg_file.mtime)

    def getstat(self, attr):
        """
        Get attribute from stat object for given file.
        :param attr: (``str``) Will be prefixed with "st_".
        """
        if self.stat is not None:
            return getattr(self.stat, "st_"+attr)

    def make_checksums(self):
        """
        Compute checksums for installed file and cache them.
        :rtype: (``tuple``) MD5 and rpm-type checksums for given file
            as a pair.
        """
        if self.file_type[0] == packagecheck.FILE_TYPE_FILE:
            self._checksums = compute_checksums(
                    self._checksum_type, self.path)
        else:
            self._checksums = None
        return self._checksums


@cmpi_logging.trace_function
def hashfile(afile, hashers, blocksize=65536):
    """
    Generic function computing multiple hashes from single file at once.

    :param hashers: (``list``) A list of hash objects.
    :rtype: (``list``) List of digest strings (in hex format) for each
        given hash object in the same order.
    """
    if not isinstance(hashers, (tuple, list, set, frozenset)):
        hashers = (hashers, )
    buf = afile.read(blocksize)
    while len(buf) > 0:
        for hashfunc in hashers:
            hashfunc.update(buf)
        buf = afile.read(blocksize)
    return [ hashfunc.hexdigest() for hashfunc in hashers ]

@cmpi_logging.trace_function
def compute_checksums(checksum_type, file_path):
    """
    Compute file checksums in one go.

    :param checksum_type: (``int`) Selected hash algorithm to compute second
        checksum (the one used for rpm package). Correct value can be
        obtained from ``yum.constants.RPM_CHECKSUM_TYPES``.
    :param file_path: (``str``) Absolute path of regular file to hash.
    :rtype (md5sum, checksum) Tuple of strings containing hex represention
        of computed checksums.

    Both checksums are computed from file_path's content.
    First one is always md5, the second one depends on checksum_type.
    If file does not exists, (None, None) is returned.
    """
    LOG().debug("checksuming file %s", file_path)
    hashers = [hashlib.md5()]   #pylint: disable=E1101
    if checksum_type != packagecheck.CHECKSUMTYPE_STR2NUM["md5"]:
        hashers.append(checksumtype_num2hash(checksum_type)())
    try:
        with open(file_path, 'rb') as fobj:
            rslts = hashfile(fobj, hashers)
    except (OSError, IOError) as exc:
        LOG().error("could not open file \"%s\" for reading: %s",
                file_path, exc)
        return None, None
    return (rslts[0], rslts[1] if len(rslts) > 1 else rslts[0]*2)

@cmpi_logging.trace_function
def checksumtype_num2hash(csumt):
    """
    Get checksum function for rpm hash identifier.

    :param csumt: (``int``) Checksum type as a number obtained from package.
    :rtype: (``function``) Hash function object corresponding to csumt.
    """
    return getattr(hashlib, yum.constants.RPM_CHECKSUM_TYPES[csumt])

@cmpi_logging.trace_function
def mode2pywbem_flags(mode):
    """
    Utility for creation of value of ``FileModeFlags`` property out of file's
    raw mode.

    :param mode: (``int``) File's mode. Retrieved from os.lstat().
        If None, file does not exist.
    :rtype: (``list``) Of integer flags describing file's access permissions.
    """
    if mode is None:
        return None
    flags = []
    for i, flag in enumerate((
            stat.S_IXOTH,
            stat.S_IWOTH,
            stat.S_IROTH,
            stat.S_IXGRP,
            stat.S_IWGRP,
            stat.S_IRGRP,
            stat.S_IXUSR,
            stat.S_IWUSR,
            stat.S_IRUSR,
            stat.S_ISVTX,
            stat.S_ISGID,
            stat.S_ISUID)):
        if flag & mode:
            flags.append(pywbem.Uint8(i))
    return flags

@cmpi_logging.trace_function
def get_existing_check_from_job(check_id):
    """
    ``CheckID`` of ``LMI_SoftwareIdentityFileCheck`` refers to asynchronous job
    that caused creation of this file check. Let's parse it, get the job out
    of YumWorker and ask it for results so they don't have to be computed
    again.

    :param check_id: (``str``) Value of ``CheckID`` property.
    :rtype: (``tuple``) Pair of (pkg_info, pkg_check).
        ``None`` if job does not exists anymore.
    """
    try:
        match = util.RE_INSTANCE_ID.match(check_id)
        if not match:
            raise ValueError(check_id)
        if (  not match.group('clsname').lower()
           == "LMI_SoftwareVerificationJob".lower()):
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                    "CheckID does not contain supported job class name: "
                    " %s != %s" % (match.group('clsname'),
                        "LMI_SoftwareVerificationJob".lower()))
        job_id = int(match.group('id'))
        job = YumDB.get_instance().get_job(job_id)
        if not isinstance(job,
                (jobs.YumCheckPackage, jobs.YumCheckPackageFile)):
            LOG().error('CheckID="%s" of LMI_SoftwareIdentityFileCheck refers'
                ' to job %s.', check_id, job)
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "CheckID refers to wrong job.")
        return job.result_data

    except errors.JobNotFound:
        # allow this - job could already be deleted
        LOG().warn("could not find YumCheckPackage or"
                " YumCheckPackageFile job with id \"%d\"" % job_id)
    except ValueError:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "Could not parse \"CheckID\" key property: \"%s\"." % check_id)

@cmpi_logging.trace_function
def object_path2file_check(objpath):
    """
    @return (package_info, package_check, file_name)
    """
    if not isinstance(objpath, pywbem.CIMInstanceName):
        raise TypeError("objpath must be instance of CIMInstanceName, "
                "not \"%s\"" % objpath.__class__.__name__)

    for key in ('Name', 'SoftwareElementID', 'Version',
            'SoftwareElementState', 'TargetOperatingSystem'):
        if not key in objpath or not objpath[key]:
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                    "Missing key property \"%s\"." % key)
    version_match = util.RE_EVRA.match(objpath['Version'])
    if not version_match:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
            'Failed to parse \"Version\" property for valid EVRA: "%s".' %
            objpath['Version'])
    pkg_fltr = util.nevra2filter(objpath["SoftwareElementID"])
    if any(   pkg_fltr[attr] != version_match.group(attr)
          for attr in ('epoch', 'version', 'release', 'arch')):
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
            'Inconsistent \"Version\" and \"SoftwareElementID\" key'
            ' properties.')
    if (  objpath['SoftwareElementState']
       != Values.SoftwareElementState.Executable):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "Only \"Executable\" software element state supported")
    if not util.check_target_operating_system(
            objpath['TargetOperatingSystem']):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "Wrong target operating system.")

    ydb = YumDB.get_instance()
    pkg_check = None
    if (  objpath["CheckID"].lower()
       != "LMI:LMI_SoftwareIdentityFileCheck".lower()):
        check = get_existing_check_from_job(objpath["CheckID"])
        if check is not None:
            pkg_info, pkg_check = check
        # else - job does not exist (could be deleted)

    if pkg_check is None:
        # let the YumWorker check the rest
        try:
            pkg_info, pkg_check = ydb.check_package_file(
                    objpath["SoftwareElementID"], objpath["Name"])
        except errors.PackageNotFound:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
                "Could not find package matching SoftwareElementID \"%s\"" %
                objpath["SoftwareElementID"])
        except errors.FileNotFound as exc:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND, exc.args[1])

    # last check for file path
    if objpath["Name"] not in pkg_check:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,
            'File \"%s\" not found in package \"%s\".' %
            objpath["SoftwareElementID"])
    return FileCheck(pkg_info, pkg_check[objpath["Name"]],
            pkg_check.file_checksum_type)

@cmpi_logging.trace_function
def test_file(pkg_info, checksum_type, pkg_file):
    """
    :param checksum_type: (``int``) Identifier of checksum algorithm used
        in rpm package. It's one of keys of
        ``yum.constants.RPM_CHECKSUM_TYPES`` dictionary.
    :rtype: (``FileCheck``)
    """
    for var, cls in (
                ("pkg_info", packageinfo.PackageInfo),
                ("pkg_file", packagecheck.PackageFile)):
        if not isinstance(locals()[var], cls):
            raise TypeError("%s must be an instance of %s, not %s" % (
                var, cls.__name__, pkg_file.__class__.__name__))
    return FileCheck(pkg_info, pkg_file, checksum_type)

@cmpi_logging.trace_function
def _file_check2failed_flags(file_check):
    """
    Makes value of ``FailedFlags`` property from ``FileCheck`` instance.

    :param file_check: (``FileCheck``)
    :rtype: (``pywbem.CIMProperty``) Property ``FailedFlags`` value.
    """
    if not isinstance(file_check, FileCheck):
        raise TypeError("file_check must be an instance of FileCheck")

    if not file_check.exists:
        return [Values.FailedFlags.Existence]
    flags = set()
    fails_on = lambda attr: (
            getattr(file_check, attr)[0] != getattr(file_check, attr)[1])
    if fails_on('file_type'):
        flags.add(Values.FailedFlags.FileMode)

    # file type specific checks
    if file_check.file_type[1] == packagecheck.FILE_TYPE_FILE:
        if fails_on('file_size'):
            flags.add(Values.FailedFlags.FileSize)
        if fails_on('last_modification_time'):
            flags.add(Values.FailedFlags.Last_Modification_Time)
        if fails_on('file_checksum'):
            flags.add(Values.FailedFlags.Checksum)
    elif file_check.file_type[1] in (
            packagecheck.FILE_TYPE_CHARACTER_DEVICE,
            packagecheck.FILE_TYPE_BLOCK_DEVICE):
        if fails_on('device'):
            flags.add(Values.FailedFlags.Device_Number)
    elif file_check.file_type[1] == packagecheck.FILE_TYPE_SYMLINK:
        if fails_on('link_target'):
            flags.add(Values.FailedFlags.LinkTarget)

    # generic checks
    if (   fails_on('file_mode')
       # do not check permissions for symlinks
       and file_check.file_type[1] != packagecheck.FILE_TYPE_SYMLINK):
        flags.add(Values.FailedFlags.FileMode)
    if fails_on('user_id'):
        flags.add(Values.FailedFlags.UserID)
    if fails_on('group_id'):
        flags.add(Values.FailedFlags.GroupID)

    return list(flags)

@cmpi_logging.trace_function
def file_check_passed(file_check):
    """
    Verify single file and return true if it passes.
    """
    return len(_file_check2failed_flags(file_check)) == 0

@cmpi_logging.trace_function
def _fill_non_key_values(model, file_check):
    """
    Fills a non key values into instance of SoftwareIdentityFileCheck.

    :param model: (``CIMInstance``) Model, that will be filled with
        all supported non-key properties.
    """
    set_prop = lambda name, kwargs: \
            model.__setitem__(name, pywbem.CIMProperty(name, **kwargs))

    set_prop("CheckMode", dict(type='boolean', value=True))
    model["ChecksumType"] = pywbem.Uint16(file_check.file_checksum_type)

    set_prop("FailedFlags", dict(type='uint16', is_array=True,
            value=_file_check2failed_flags(file_check)))
    for mattr, fattr, kwargs, convert in (
            ('LastModificationTime', 'last_modification_time',
                {'type':'uint64'}, pywbem.Uint64),
            ('FileType'    , 'file_type'    ,{'type':'uint16'}, pywbem.Uint16),
            ('UserID'      , 'user_id'      ,{'type':'uint32'}, pywbem.Uint32),
            ('GroupID'     , 'group_id'     ,{'type':'uint32'}, pywbem.Uint32),
            ('FileMode'    , 'file_mode'    ,{'type':'uint32'}, pywbem.Uint32),
            ('FileSize'    , 'file_size'    ,{'type':'uint64'}, pywbem.Uint64),
            ('LinkTarget'  , 'link_target'  ,{'type':'string'}, str),
            ('FileChecksum', 'file_checksum',{'type':'string'}, str)):
        installed, orig = getattr(file_check, fattr)
        kwargs['value'] = None if orig is None else convert(orig)
        set_prop(mattr+"Original", kwargs)
        kwargs['value'] = None if installed is None else convert(installed)
        set_prop(mattr, kwargs)
    model['FileName'] = os.path.basename(file_check.path)
    for suf in ('', 'Original'):
        set_prop('FileModeFlags'+suf, dict(type='uint8', is_array=True,
            value=mode2pywbem_flags(file_check.file_mode[1])))
    set_prop("FileExists", dict(type='boolean', value=file_check.exists))
    set_prop('MD5Checksum', dict(type='string',
            value=file_check.md5_checksum))

@cmpi_logging.trace_function
def file_check2model(file_check, keys_only=True, model=None, job=None):
    """
    :param file_check: (``FileCheck``) File check information to transform
        to cim instance.
    :param keys_only: (``bool``) Whether to fill non-key properties.
    :param model: (``CIMInstance`` | ``CIMInstanceName``) If given,
        it will be used as a template. Only property values will be set.
        This instance will be returned on completion.
    :param job: (``YumAsyncJob``) If this is a consequence of asynchronous
        job operation, the instance of job should be supplied so that
        ``CheckID`` property can be filled.
    :rtype: (``CIMInstance`` | ``CIMInstanceName``) Object of
        LMI_SoftwareIdentityFileCheck with filled desired values.
    """
    if job is not None and not isinstance(job, jobs.YumCheckPackage):
        raise TypeError("job must be instance of jobs.YumCheckPackage")
    if not isinstance(file_check, FileCheck):
        raise TypeError("file_check must be an instance of FileCheck")

    if model is None:
        model = util.new_instance_name("LMI_SoftwareIdentityFileCheck")
        if not keys_only:
            model = pywbem.CIMInstance("LMI_SoftwareIdentityFileCheck",
                    path=model)

    if not keys_only:
        model.path.update(  #pylint: disable=E1103
            {k: None for k in ("Name", "SoftwareElementID",
            "SoftwareElementState", "TargetOperatingSystem", "Version",
            "CheckID")})

    model['Name'] = file_check.path
    model['SoftwareElementID'] = file_check.pkg_info.nevra
    model['SoftwareElementState'] = Values.SoftwareElementState.Executable
    model['TargetOperatingSystem'] = pywbem.Uint16(
            util.get_target_operating_system()[0])
    model['Version'] = file_check.pkg_info.evra
    model['CheckID'] = ("LMI:LMI_SoftwareVerificationJob:%d" % job.jobid
        if job is not None else "LMI:%s" % model.classname)
    if not keys_only:
        _fill_non_key_values(model, file_check)
    return model

