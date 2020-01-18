/*
 * Copyright (C) 2012-2013 Red Hat, Inc.  All rights reserved.
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

const ConfigEntry *provider_config_defaults = NULL;
const char *provider_name = "logicalfile";


CMPIStatus lmi_check_required(
    const CMPIBroker *b,
    const CMPIContext *ctx,
    const CMPIObjectPath *o)
{
    const char *prop;

    CMPIStatus st;
    CMPIData data;
    /* check computer system creation class name */
    data = CMGetKey(o, "CSCreationClassName", &st);
    check_status(st);
    if (CMIsNullValue(data)) {
        CMReturnWithChars(b, CMPI_RC_ERR_FAILED, "CSCreationClassName is empty");
    }
    prop = get_string_property_from_op(o, "CSCreationClassName");
    if (strcmp(prop, lmi_get_system_creation_class_name()) != 0) {
        CMReturnWithChars(b, CMPI_RC_ERR_FAILED, "Wrong CSCreationClassName");
    }

    /* check fqdn */
    data = CMGetKey(o, "CSName", &st);
    check_status(st);
    if (CMIsNullValue(data)) {
        CMReturnWithChars(b, CMPI_RC_ERR_FAILED, "CSName is empty");
    }
    prop = get_string_property_from_op(o, "CSName");
    if (strcmp(prop, lmi_get_system_name()) != 0) {
        CMReturnWithChars(b, CMPI_RC_ERR_FAILED, "Wrong CSName");
    }

    CMReturn(CMPI_RC_OK);
}

void get_class_from_stat(const struct stat *sb, char *fileclass) {
    (S_ISREG(sb->st_mode)) ? strcpy(fileclass, "LMI_DataFile") :
    (S_ISDIR(sb->st_mode)) ? strcpy(fileclass, "LMI_UnixDirectory") :
    (S_ISCHR(sb->st_mode)) ? strcpy(fileclass, "LMI_UnixDeviceFile") :
    (S_ISBLK(sb->st_mode)) ? strcpy(fileclass, "LMI_UnixDeviceFile") :
    (S_ISLNK(sb->st_mode)) ? strcpy(fileclass, "LMI_SymbolicLink") :
    (S_ISFIFO(sb->st_mode)) ? strcpy(fileclass, "LMI_FIFOPipeFile") :
    (S_ISSOCK(sb->st_mode)) ? strcpy(fileclass, "LMI_UnixSocket") :
    strcpy(fileclass, "Unknown");
    assert(strcmp(fileclass, "Unknown") != 0);
}

int get_class_from_path(const char *path, char *fileclass)
{
    int rc = 0;
    struct stat sb;

    if (lstat(path, &sb) < 0) {
        rc = 1;
    } else {
        get_class_from_stat(&sb, fileclass);
    }

    return rc;
}

CMPIStatus get_fsinfo_from_stat(const CMPIBroker *b, const struct stat *sb, const char *path,
                                char **fsclassname, char **fsname)
{
    struct udev *udev_ctx;
    struct udev_device *udev_dev;
    const char *dev_name;
    CMPIStatus st = {.rc = CMPI_RC_OK};

    udev_ctx = udev_new();
    if (!udev_ctx) {
        return_with_status(b, &st, ERR_FAILED, "Could not create udev context");
    }

    char dev_id[16];
    snprintf(dev_id, 16, "b%u:%u", major(sb->st_dev), minor(sb->st_dev));

    udev_dev = udev_device_new_from_device_id(udev_ctx, dev_id);
    if ((dev_name = udev_device_get_property_value(udev_dev, "ID_FS_UUID_ENC"))) {
        if (asprintf(fsname, "UUID=%s", dev_name) < 0) {
            return_with_status(b, &st, ERR_FAILED, "asprintf failed");
        }
        *fsclassname = FSCREATIONCLASSNAME_LOCAL;
    } else if ((dev_name = udev_device_get_property_value(udev_dev, "DEVNAME"))) {
        if (asprintf(fsname, "DEVICE=%s", dev_name) < 0) {
            return_with_status(b, &st, ERR_FAILED, "asprintf failed");
        }
        *fsclassname = FSCREATIONCLASSNAME_LOCAL;
    } else {
        if (asprintf(fsname, "PATH=%s", path) < 0) {
            return_with_status(b, &st, ERR_FAILED, "asprintf failed");
        }
        *fsclassname = FSCREATIONCLASSNAME_TRANSIENT;
    }
    udev_device_unref(udev_dev);
    udev_unref(udev_ctx);

    return st;
}

CMPIStatus get_fsinfo_from_path(const CMPIBroker *b, const char *path, char **fsclassname, char **fsname)
{
    CMPIStatus st = {.rc = CMPI_RC_OK};
    struct stat sb;

    if (lstat(path, &sb) < 0) {
        return_with_status(b, &st, ERR_FAILED, "lstat(2) failed");
    }

    return get_fsinfo_from_stat(b, &sb, path, fsclassname, fsname);
}

const char *get_string_property_from_op(const CMPIObjectPath *o, const char *prop)
{
    CMPIData d;
    d = CMGetKey(o, prop, NULL);
    return KChars(d.value.string);
}

CMPIStatus check_assoc_class(
    const CMPIBroker *cb,
    const char *namespace,
    const char *assocClass,
    const char *class)
{
    CMPIObjectPath *o;
    CMPIStatus st = {.rc = CMPI_RC_OK};

    o = CMNewObjectPath(cb, namespace, class, &st);
    if (!o) {
        /* assume that status has been properly set */
        return st;
    } else if (st.rc != CMPI_RC_OK) {
        CMRelease(o);
        return st;
    }
    if (assocClass && !CMClassPathIsA(cb, o, assocClass, &st)) {
        CMRelease(o);
        st.rc = RC_ERR_CLASS_CHECK_FAILED;
        return st;
    }
    CMRelease(o);
    return st;
}

CMPIStatus stat_logicalfile_and_fill(
    const CMPIBroker *b,
    logicalfile_t *lf,
    mode_t mode,
    const char *errmsg)
{
    struct stat sb;
    char buf[BUFLEN];
    char *fsname = NULL;
    char *fsclassname = NULL;
    const char *path = KChars(lf->lf.datafile.Name.value);
    CMPIStatus st = {.rc = CMPI_RC_OK};

    if (lstat(path, &sb) < 0 || !(sb.st_mode & S_IFMT & mode)) {
        snprintf(buf, BUFLEN, errmsg, path);
        CMReturnWithChars(b, CMPI_RC_ERR_NOT_FOUND, buf);
    }

    get_class_from_stat(&sb, buf);

    st = get_fsinfo_from_stat(b, &sb, path, &fsclassname, &fsname);
    check_status(st);

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
    default:
        /* impossible */
        assert(0);
    }

    free(fsname);
    return st;
}

void _dump_objectpath(const CMPIObjectPath *o)
{
    printf("OP: %s\n", CMGetCharsPtr(o->ft->toString(o, NULL), NULL));
}
/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* End: */
