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
#ifndef _FILE_H
#define _FILE_H

#include <dirent.h>
#include <libgen.h>
#include <assert.h>
#include "LMI_DataFile.h"
#include "LMI_UnixDeviceFile.h"
#include "LMI_SymbolicLink.h"
#include "LMI_UnixDirectory.h"
#include "LMI_UnixSocket.h"
#include "LMI_FIFOPipeFile.h"
#include "openlmi.h"
#include "openlmi-utils.h"

const ConfigEntry *provider_config_defaults;
const char *provider_name;

#define DEVTYPE_BLK 2
#define DEVTYPE_CHR 3

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

#define fill_logicalfile(ctx, type, obj, name, fsclassname, fsname, creation_class) \
    type##_Set_Name((obj), (name));                                              \
    type##_Set_CSCreationClassName((obj), lmi_get_system_creation_class_name()); \
    type##_Set_CSName((obj), lmi_get_system_name_safe(ctx));                     \
    type##_Set_FSCreationClassName((obj), fsclassname);                          \
    type##_Set_FSName((obj), (fsname));                                          \
    type##_Set_CreationClassName((obj), (creation_class));

#define fill_basic(b, cmpitype, lmi_file, creation_class_name, fsclassname, fsname, sb) \
    LMI_##cmpitype##_Set_CreationClassName(lmi_file, creation_class_name);    \
    LMI_##cmpitype##_Set_FSCreationClassName(lmi_file, fsclassname);          \
    LMI_##cmpitype##_Set_FSName(lmi_file, fsname);                            \
    LMI_##cmpitype##_Set_Readable(lmi_file, sb_isreadable(sb));               \
    LMI_##cmpitype##_Set_Writeable(lmi_file, sb_iswriteable(sb));             \
    LMI_##cmpitype##_Set_Executable(lmi_file, sb_isexecutable(sb));           \
    LMI_##cmpitype##_Set_FileSize(lmi_file, sb.st_size);                      \
    LMI_##cmpitype##_Set_LastAccessed(lmi_file, CMNewDateTimeFromBinary(b, LMI_SECS_TO_MS(sb.st_atime), 0, NULL)); \
    LMI_##cmpitype##_Set_LastModified(lmi_file, CMNewDateTimeFromBinary(b, LMI_SECS_TO_MS(sb.st_mtime), 0, NULL));

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

CMPIStatus stat_logicalfile_and_fill(const CMPIBroker *, logicalfile_t *, mode_t, const char *);

#endif /* _FILE_H */
/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* c-backslash-max-column: 78 */
/* End: */
