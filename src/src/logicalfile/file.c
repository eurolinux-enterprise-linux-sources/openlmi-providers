/*
 * Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Jan Synacek <jsynacek@redhat.com>
 */
#include "file.h"

const ConfigEntry *provider_config_defaults = (const ConfigEntry *)&(ConfigEntry []) {
    /* group, key, value */
    {"LMI_SymbolicLink",  "AllowSymlink", "False"},
    {"LMI_UnixDirectory", "AllowMkdir",   "True"},
    {"LMI_UnixDirectory", "AllowRmdir",   "True"},
    {NULL, NULL, NULL}
};
const char *provider_name = "logicalfile";

CMPIStatus stat_logicalfile_and_fill(
    const CMPIBroker *b,
    logicalfile_t *lf,
    mode_t mode,
    const char *errmsg)
{
    struct stat sb;
    char buf[BUFLEN];
    char *fsname;
    char *fsclassname;
    const char *path = KChars(lf->lf.datafile.Name.value);
    CMPIStatus st = {.rc = CMPI_RC_OK};

    if (lstat(path, &sb) < 0 || !(sb.st_mode & S_IFMT & mode)) {
        snprintf(buf, BUFLEN, errmsg, path);
        CMReturnWithChars(b, CMPI_RC_ERR_NOT_FOUND, buf);
    }

    get_logfile_class_from_stat(&sb, buf);

    /* only use udev information if no fs information is provided */
    /* discarding const qualifiers is ok here, it makes the code a bit more simple */
    fsname = (char *) KChars(lf->lf.datafile.FSName.value);
    fsclassname = (char *) KChars(lf->lf.datafile.FSCreationClassName.value);
    st = get_fsinfo_from_stat(b, &sb, path, &fsclassname, &fsname);
    lmi_return_if_status_not_ok(st);

    switch(mode) {
    case S_IFREG:
        fill_basic(b, DataFile, &lf->lf.datafile, buf, fsclassname, fsname, sb);
        break;
    case S_IFCHR:
        /* FALLTHROUGH */
    case S_IFBLK:
        fill_basic(b, UnixDeviceFile, &lf->lf.unixdevicefile, buf, fsclassname, fsname, sb);
        /* device-specific stuff */
        char tmp[21];
        sprintf(tmp, "%lu", sb.st_rdev);
        LMI_UnixDeviceFile_Set_DeviceId(&lf->lf.unixdevicefile, tmp);
        sprintf(tmp, "%u",  major(sb.st_rdev));
        LMI_UnixDeviceFile_Set_DeviceMajor(&lf->lf.unixdevicefile, tmp);
        sprintf(tmp, "%u", minor(sb.st_rdev));
        LMI_UnixDeviceFile_Set_DeviceMinor(&lf->lf.unixdevicefile, tmp);
        if (S_ISCHR(sb.st_mode)) {
            LMI_UnixDeviceFile_Set_DeviceFileType(&lf->lf.unixdevicefile, DEVTYPE_CHR);
        } else if (S_ISBLK(sb.st_mode)) {
            LMI_UnixDeviceFile_Set_DeviceFileType(&lf->lf.unixdevicefile, DEVTYPE_BLK);
        }
        break;
    case S_IFDIR:
        fill_basic(b, UnixDirectory, &lf->lf.unixdirectory, buf, fsclassname, fsname, sb);
        break;
    case S_IFIFO:
        fill_basic(b, FIFOPipeFile, &lf->lf.fifopipefile, buf, fsclassname, fsname, sb);
        break;
    case S_IFLNK:
        fill_basic(b, SymbolicLink, &lf->lf.symboliclink, buf, fsclassname, fsname, sb);
        /* symlink-specific stuff */
        char rpath[PATH_MAX];
        const char *path;
        path = KChars(lf->lf.symboliclink.Name.value);
        realpath(path, rpath);
        LMI_SymbolicLink_Set_TargetFile(&lf->lf.symboliclink, rpath);
        break;
    case S_IFSOCK:
        fill_basic(b, UnixSocket, &lf->lf.unixsocket, buf, fsclassname, fsname, sb);
        break;
    default:
        /* impossible */
        assert(0);
    }

    return st;
}

/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4 */
/* End: */
