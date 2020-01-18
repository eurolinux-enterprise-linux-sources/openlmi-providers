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
#ifndef _FILE_H
#define _FILE_H

#include <linux/limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <konkret/konkret.h>
#include <assert.h>
#include <libudev.h>
#include "LMI_DataFile.h"
#include "LMI_UnixDeviceFile.h"
#include "LMI_SymbolicLink.h"
#include "LMI_UnixDirectory.h"
#include "LMI_UnixSocket.h"
#include "LMI_FIFOPipeFile.h"
#include "globals.h"

#ifndef BUFLEN
  #define BUFLEN 512
#endif
#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

const ConfigEntry *provider_config_defaults;
const char *provider_name;

#define FSCREATIONCLASSNAME "LMI_LocalFileSystem"
#define GROUP_COMPONENT "GroupComponent"
#define PART_COMPONENT "PartComponent"
#define SAME_ELEMENT "SameElement"
#define SYSTEM_ELEMENT "SystemElement"
#define DEVTYPE_BLK 2
#define DEVTYPE_CHR 3

/* CMPI_RC_ERR_<error> values end at 200. 0xFF should be safe. */
#define RC_ERR_CLASS_CHECK_FAILED 0xFF

#define sb_permmask(sb) ((sb).st_mode & (S_IRWXU | S_IRWXG | S_IRWXO))
#define sb_isreadable(sb) (                                                   \
    (sb_permmask(sb) & S_IRUSR) ||                                            \
    (sb_permmask(sb) & S_IRGRP) ||                                            \
    (sb_permmask(sb) & S_IROTH)                                               \
)
#define sb_iswriteable(sb) (                                                  \
    (sb_permmask(sb) & S_IWUSR) ||                                            \
    (sb_permmask(sb) & S_IWGRP) ||                                            \
    (sb_permmask(sb) & S_IWOTH)                                               \
)
#define sb_isexecutable(sb) (                                                 \
    (sb_permmask(sb) & S_IXUSR) ||                                            \
    (sb_permmask(sb) & S_IXGRP) ||                                            \
    (sb_permmask(sb) & S_IXOTH)                                               \
)
#define stoms(t) ((t)*1000000)

#define fill_logicalfile(type, obj, name, fsname, creation_class)             \
    type##_Set_Name((obj), (name));                                           \
    type##_Set_CSCreationClassName((obj), get_system_creation_class_name());  \
    type##_Set_CSName((obj), get_system_name());                              \
    type##_Set_FSCreationClassName((obj), FSCREATIONCLASSNAME);               \
    type##_Set_FSName((obj), (fsname));                                       \
    type##_Set_CreationClassName((obj), (creation_class));

#define fill_basic(b, cmpitype, lmi_file, creation_class_name, fsname, sb)    \
    LMI_##cmpitype##_Set_CreationClassName(lmi_file, creation_class_name);    \
    LMI_##cmpitype##_Set_FSCreationClassName(lmi_file, FSCREATIONCLASSNAME);  \
    LMI_##cmpitype##_Set_FSName(lmi_file, fsname);                            \
    LMI_##cmpitype##_Set_Readable(lmi_file, sb_isreadable(sb));               \
    LMI_##cmpitype##_Set_Writeable(lmi_file, sb_iswriteable(sb));             \
    LMI_##cmpitype##_Set_Executable(lmi_file, sb_isexecutable(sb));           \
    LMI_##cmpitype##_Set_FileSize(lmi_file, sb.st_size);                      \
    LMI_##cmpitype##_Set_LastAccessed(lmi_file, CMNewDateTimeFromBinary(b, stoms(sb.st_atime), 0, NULL)); \
    LMI_##cmpitype##_Set_LastModified(lmi_file, CMNewDateTimeFromBinary(b, stoms(sb.st_mtime), 0, NULL));

#define check_status(st)                                                      \
    if (st.rc != CMPI_RC_OK) {                                                \
        return st;                                                            \
    }

#define check_class_check_status(st)                                          \
    if (st.rc == RC_ERR_CLASS_CHECK_FAILED) {                                 \
        CMReturn(CMPI_RC_OK);                                                 \
    }                                                                         \
    check_status(st);

#define return_with_status(b, st, code, msg)    \
    KSetStatus2(b, st, code, msg);              \
    return *(st);

typedef struct {
	union {
		LMI_DataFile datafile;
		LMI_UnixDeviceFile unixdevicefile;
		LMI_UnixDirectory unixdirectory;
		LMI_FIFOPipeFile fifopipefile;
		LMI_UnixSocket unixsocket;
		LMI_SymbolicLink symboliclink;
	} lf;
} logicalfile_t;

CMPIStatus lmi_check_required(const CMPIBroker *, const CMPIContext *, const CMPIObjectPath *);
void get_class_from_stat(const struct stat *, char *);
int get_class_from_path(const char *, char *);
CMPIStatus get_fsname_from_stat(const CMPIBroker *, const struct stat *, char **);
CMPIStatus get_fsname_from_path(const CMPIBroker *, const char *, char **);
const char *get_string_property_from_op(const CMPIObjectPath *, const char *);
CMPIStatus check_assoc_class(const CMPIBroker *, const char *, const char *, const char *);
CMPIStatus stat_logicalfile_and_fill(const CMPIBroker *, logicalfile_t *, mode_t, const char *);
void _dump_objectpath(const CMPIObjectPath *);

#endif /* _FILE_H */
/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* c-backslash-max-column: 78 */
/* End: */
