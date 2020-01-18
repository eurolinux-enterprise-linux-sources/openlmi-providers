/*
 * Copyright (C) 2013-2014 Red Hat, Inc. All rights reserved.
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
 * Authors: Peter Schiffer <pschiffe@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_SoftwareIdentityFileCheck.h"
#include "sw-utils.h"
#include "config.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <pthread.h>

#ifdef RPM_FOUND
#include <rpm/rpmlib.h>
#include <rpm/rpmdb.h>
#include <rpm/rpmts.h>
#include <rpm/rpmfi.h>
#include <rpm/rpmvf.h>
#include <rpm/rpmfileutil.h>
#endif

static const CMPIBroker* _cb = NULL;

#ifdef RPM_FOUND
    /* Mutex for transaction set */
    static pthread_mutex_t ts_mutex = PTHREAD_MUTEX_INITIALIZER;

    rpmts ts = NULL;
#endif

static void LMI_SoftwareIdentityFileCheckInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SoftwareIdentityFileCheck_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);

#ifdef RPM_FOUND
    pthread_mutex_lock(&ts_mutex);

    if (rpmReadConfigFiles(NULL, NULL) != 0) {
        pthread_mutex_unlock(&ts_mutex);
        lmi_error("Failed to read RPM configuration files.");
        abort();
    }

    if (!ts) {
        if (!(ts = rpmtsCreate())) {
            pthread_mutex_unlock(&ts_mutex);
            lmi_error("Failed to create RPM transaction set.");
            abort();
        }
    }

    pthread_mutex_unlock(&ts_mutex);
#endif
}

static CMPIStatus LMI_SoftwareIdentityFileCheckCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
#ifdef RPM_FOUND
    if (ts) {
        pthread_mutex_lock(&ts_mutex);
        rpmtsFree(ts);
        ts = NULL;
        pthread_mutex_unlock(&ts_mutex);
    }
#endif

    return software_cleanup(LMI_SoftwareIdentityFileCheck_ClassName);
}

static CMPIStatus LMI_SoftwareIdentityFileCheckEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityFileCheckEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

#ifdef RPM_FOUND
static short count_mode_flags(mode_t st_mode)
{
    short all_flags = S_IXOTH | S_IWOTH | S_IROTH | S_IXGRP | S_IWGRP | S_IRGRP
            | S_IXUSR | S_IWUSR | S_IRUSR | S_ISVTX | S_ISGID | S_ISUID;

    return(popcount(st_mode & all_flags));
}

static void set_file_mode_flags(
        CMPIBoolean (*lmi_set_file_type)(LMI_SoftwareIdentityFileCheck *, CMPICount, CMPIUint8),
        LMI_SoftwareIdentityFileCheck *w, mode_t st_mode)
{
    short curr_flag = 0;

    if (st_mode & S_IXOTH) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_Execute_Other);
    }
    if (st_mode & S_IWOTH) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_Write_Other);
    }
    if (st_mode & S_IROTH) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_Read_Other);
    }
    if (st_mode & S_IXGRP) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_Execute_Group);
    }
    if (st_mode & S_IWGRP) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_Write_Group);
    }
    if (st_mode & S_IRGRP) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_Read_Group);
    }
    if (st_mode & S_IXUSR) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_Execute_User);
    }
    if (st_mode & S_IWUSR) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_Write_User);
    }
    if (st_mode & S_IRUSR) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_Read_User);
    }
    if (st_mode & S_ISVTX) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_Sticky);
    }
    if (st_mode & S_ISGID) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_SGID);
    }
    if (st_mode & S_ISUID) {
        lmi_set_file_type(w, curr_flag++,
                LMI_SoftwareIdentityFileCheck_FileModeFlags_SUID);
    }
}

static void set_file_type(
        void (*lmi_set_file_type)(LMI_SoftwareIdentityFileCheck *, CMPIUint16),
        LMI_SoftwareIdentityFileCheck *w, mode_t st_mode)
{
    if (S_ISREG(st_mode)) {
        lmi_set_file_type(w, LMI_SoftwareIdentityFileCheck_FileType_File);
    } else if (S_ISDIR(st_mode)) {
        lmi_set_file_type(w, LMI_SoftwareIdentityFileCheck_FileType_Directory);
    } else if (S_ISCHR(st_mode)) {
        lmi_set_file_type(w, LMI_SoftwareIdentityFileCheck_FileType_Character_Device);
    } else if (S_ISBLK(st_mode)) {
        lmi_set_file_type(w, LMI_SoftwareIdentityFileCheck_FileType_Block_Device);
    } else if (S_ISFIFO(st_mode)) {
        lmi_set_file_type(w, LMI_SoftwareIdentityFileCheck_FileType_FIFO);
    } else if (S_ISLNK(st_mode)) {
        lmi_set_file_type(w, LMI_SoftwareIdentityFileCheck_FileType_Symlink);
    } else {
        lmi_set_file_type(w, LMI_SoftwareIdentityFileCheck_FileType_Unknown);
    }
}
#endif

static CMPIStatus LMI_SoftwareIdentityFileCheckGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
#ifndef RPM_FOUND
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
#else
    CMPIrc rc = CMPI_RC_OK;
    rpmfi fi = NULL;
    Header rpm_header;
    rpm_mode_t rpm_mode;
    rpmdbMatchIterator iter = NULL;
    struct stat sb;
    const char *elem_name, *file_name;
    char error_msg[BUFLEN] = "";
    short file_exists = 1;

    elem_name = lmi_get_string_property_from_objectpath(cop, "SoftwareElementID");
    file_name = lmi_get_string_property_from_objectpath(cop, "Name");

    if (!is_elem_name_installed(elem_name)) {
        rc = CMPI_RC_ERR_NOT_FOUND;
        goto done;
    }

    pthread_mutex_lock(&ts_mutex);

    iter = rpmtsInitIterator(ts, RPMDBI_LABEL, elem_name, 0);
    if (!iter || rpmdbGetIteratorCount(iter) != 1) {
        pthread_mutex_unlock(&ts_mutex);
        lmi_warn("Failed to find package: %s", elem_name);
        rc = CMPI_RC_ERR_NOT_FOUND;
        goto done;
    }

    rpm_header = rpmdbNextIterator(iter);

    if (!(fi = rpmfiNew(ts, rpm_header, 0, RPMFI_KEEPHEADER))) {
        pthread_mutex_unlock(&ts_mutex);
        snprintf(error_msg, BUFLEN, "Failed to create RPM file info.");
        goto done;
    }

    pthread_mutex_unlock(&ts_mutex);

    int fi_it;
    rpmfiInit(fi, 0);
    while ((fi_it = rpmfiNext(fi)) >= 0) {
        if (strcmp(rpmfiFN(fi), file_name) == 0) {
            break;
        }
    }
    if (fi_it < 0) {
        lmi_warn("Package %s doesn't contain file: %s", elem_name, file_name);
        rc = CMPI_RC_ERR_NOT_FOUND;
        goto done;
    }

    if (lstat(file_name, &sb) != 0) {
        if (errno == ENOENT) {
            file_exists = 0;
        } else {
            snprintf(error_msg, BUFLEN, "Failed to lstat file %s: %s",
                    file_name, strerror(errno));
            goto done;
        }
    }

    rpm_mode = rpmfiFMode(fi);

    LMI_SoftwareIdentityFileCheck w;
    LMI_SoftwareIdentityFileCheck_InitFromObjectPath(&w, _cb, cop);
    LMI_SoftwareIdentityFileCheck_Set_CheckMode(&w, 1);
    LMI_SoftwareIdentityFileCheck_Set_FileExists(&w, file_exists);

    if (basename(file_name)) {
        LMI_SoftwareIdentityFileCheck_Set_FileName(&w, basename(file_name));
    }

    rpmVerifyAttrs res = 0;
    pthread_mutex_lock(&ts_mutex);
    if (rpmVerifyFile(ts, fi, &res, 0) != 0) {
        pthread_mutex_unlock(&ts_mutex);
        LMI_SoftwareIdentityFileCheck_Init_FailedFlags(&w, 1);
        LMI_SoftwareIdentityFileCheck_Set_FailedFlags(&w, 0,
                LMI_SoftwareIdentityFileCheck_FailedFlags_Existence);
    } else if (res) {
        pthread_mutex_unlock(&ts_mutex);

        short curr_flag = 0;

        LMI_SoftwareIdentityFileCheck_Init_FailedFlags(&w, popcount(res));

        if (res & RPMVERIFY_FILESIZE) {
            LMI_SoftwareIdentityFileCheck_Set_FailedFlags(&w, curr_flag++,
                    LMI_SoftwareIdentityFileCheck_FailedFlags_FileSize);
        }
        if (res & RPMVERIFY_MODE) {
            LMI_SoftwareIdentityFileCheck_Set_FailedFlags(&w, curr_flag++,
                    LMI_SoftwareIdentityFileCheck_FailedFlags_FileMode);
        }
        if (res & RPMVERIFY_FILEDIGEST) {
            LMI_SoftwareIdentityFileCheck_Set_FailedFlags(&w, curr_flag++,
                    LMI_SoftwareIdentityFileCheck_FailedFlags_Checksum);
        }
        if (res & RPMVERIFY_RDEV) {
            LMI_SoftwareIdentityFileCheck_Set_FailedFlags(&w, curr_flag++,
                    LMI_SoftwareIdentityFileCheck_FailedFlags_Device_Number);
        }
        if (res & RPMVERIFY_LINKTO) {
            LMI_SoftwareIdentityFileCheck_Set_FailedFlags(&w, curr_flag++,
                    LMI_SoftwareIdentityFileCheck_FailedFlags_LinkTarget);
        }
        if (res & RPMVERIFY_USER) {
            LMI_SoftwareIdentityFileCheck_Set_FailedFlags(&w, curr_flag++,
                    LMI_SoftwareIdentityFileCheck_FailedFlags_UserID);
        }
        if (res & RPMVERIFY_GROUP) {
            LMI_SoftwareIdentityFileCheck_Set_FailedFlags(&w, curr_flag++,
                    LMI_SoftwareIdentityFileCheck_FailedFlags_GroupID);
        }
        if (res & RPMVERIFY_MTIME) {
            LMI_SoftwareIdentityFileCheck_Set_FailedFlags(&w, curr_flag++,
                    LMI_SoftwareIdentityFileCheck_FailedFlags_Last_Modification_Time);
        }
    } else {
        pthread_mutex_unlock(&ts_mutex);
        LMI_SoftwareIdentityFileCheck_Init_FailedFlags(&w, 0);
    }

    int algo = rpmfiDigestAlgo(fi);
    if (algo) {
        LMI_SoftwareIdentityFileCheck_Set_ChecksumType(&w, algo);
    }

    if (file_exists) {
        rpm_loff_t fsizep = 0;

        if (S_ISLNK(sb.st_mode)) {
            ssize_t r;
            char link_name[PATH_MAX];
            if ((r = readlink(file_name, link_name, PATH_MAX)) > 0) {
                link_name[r] = '\0';
                LMI_SoftwareIdentityFileCheck_Set_LinkTarget(&w, link_name);
            }
        } else if (S_ISREG(sb.st_mode)) {
            char *header_digest = NULL;
            if ((header_digest = rpmfiFDigestHex(fi, NULL))) {
                if (rpm_mode && S_ISREG(rpm_mode)) {
                    LMI_SoftwareIdentityFileCheck_Set_FileChecksumOriginal(&w,
                            header_digest);
                }
                free(header_digest);
                header_digest = NULL;

                unsigned char fdigest[BUFLEN];
                if (algo) {
                    if (rpmDoDigest(algo, file_name, 1, fdigest, &fsizep) == 0) {
                        if (*fdigest) {
                            LMI_SoftwareIdentityFileCheck_Set_FileChecksum(&w,
                                    (char *)fdigest);
                            if (fsizep) {
                                LMI_SoftwareIdentityFileCheck_Set_FileSize(&w,
                                        fsizep);
                            }
                        }
                    }
                }
                if (rpmDoDigest(PGPHASHALGO_MD5, file_name, 1, fdigest, NULL) == 0) {
                    if (*fdigest) {
                        LMI_SoftwareIdentityFileCheck_Set_MD5Checksum(&w,
                                (char *)fdigest);
                    }
                }
            }
        }

        /* RPM is using different size than sb.st_size, so use this size only
         * if the RPM size is not available */
        if (!fsizep) {
            LMI_SoftwareIdentityFileCheck_Set_FileSize(&w, sb.st_size);
        }

        LMI_SoftwareIdentityFileCheck_Set_FileMode(&w, sb.st_mode);
        LMI_SoftwareIdentityFileCheck_Init_FileModeFlags(&w,
                count_mode_flags(sb.st_mode));
        set_file_mode_flags(LMI_SoftwareIdentityFileCheck_Set_FileModeFlags, &w,
                sb.st_mode);
        set_file_type(LMI_SoftwareIdentityFileCheck_Set_FileType, &w, sb.st_mode);

        LMI_SoftwareIdentityFileCheck_Set_UserID(&w, sb.st_uid);
        LMI_SoftwareIdentityFileCheck_Set_GroupID(&w, sb.st_gid);
        LMI_SoftwareIdentityFileCheck_Set_LastModificationTime(&w, sb.st_mtime);
    }

    if (rpm_mode) {
        LMI_SoftwareIdentityFileCheck_Set_FileModeOriginal(&w, rpm_mode);
        LMI_SoftwareIdentityFileCheck_Init_FileModeFlagsOriginal(&w,
                count_mode_flags(rpm_mode));
        set_file_mode_flags(LMI_SoftwareIdentityFileCheck_Set_FileModeFlagsOriginal,
                &w, rpm_mode);
        set_file_type(LMI_SoftwareIdentityFileCheck_Set_FileTypeOriginal, &w,
                rpm_mode);
    }

    if (rpmfiFSize(fi)) {
        LMI_SoftwareIdentityFileCheck_Set_FileSizeOriginal(&w, rpmfiFSize(fi));
    }

    if (rpmfiFUser(fi)) {
        struct passwd *pwd = getpwnam(rpmfiFUser(fi));
        if (pwd) {
            LMI_SoftwareIdentityFileCheck_Set_UserIDOriginal(&w, pwd->pw_uid);
        }
    }

    if (rpmfiFGroup(fi)) {
        struct group *grp = getgrnam(rpmfiFGroup(fi));
        if (grp) {
            LMI_SoftwareIdentityFileCheck_Set_GroupIDOriginal(&w, grp->gr_gid);
        }
    }

    if (rpmfiFMtime(fi)) {
        LMI_SoftwareIdentityFileCheck_Set_LastModificationTimeOriginal(&w,
                rpmfiFMtime(fi));
    }

    const char *link_target_original = rpmfiFLink(fi);
    if (link_target_original && *link_target_original) {
        LMI_SoftwareIdentityFileCheck_Set_LinkTargetOriginal(&w, link_target_original);
    }

    KReturnInstance(cr, w);

done:
    if (fi) {
        rpmfiFree(fi);
    }
    if (iter) {
        rpmdbFreeIterator(iter);
    }

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(rc);
#endif
}

static CMPIStatus LMI_SoftwareIdentityFileCheckCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityFileCheckModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityFileCheckDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityFileCheckExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

CMInstanceMIStub(
    LMI_SoftwareIdentityFileCheck,
    LMI_SoftwareIdentityFileCheck,
    _cb,
    LMI_SoftwareIdentityFileCheckInitialize(ctx))

static CMPIStatus LMI_SoftwareIdentityFileCheckMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareIdentityFileCheckInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SoftwareIdentityFileCheck_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SoftwareIdentityFileCheck,
    LMI_SoftwareIdentityFileCheck,
    _cb,
    LMI_SoftwareIdentityFileCheckInitialize(ctx))

KUint32 LMI_SoftwareIdentityFileCheck_Invoke(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareIdentityFileCheckRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

#ifdef RPM_FOUND
    const char *elem_name;

    CMPIObjectPath *o = LMI_SoftwareIdentityFileCheckRef_ToObjectPath(self, NULL);
    elem_name = lmi_get_string_property_from_objectpath(o, "SoftwareElementID");

    if (!is_elem_name_installed(elem_name)) {
        KSetStatus(status, ERR_NOT_FOUND);
        return result;
    }

    CMPIInstance *ci = _cb->bft->getInstance(_cb, context, o, NULL, NULL);
    CMPIData d = CMGetProperty(ci, "FailedFlags", NULL);

    /*
     * Result values:
     * 0 - condition is satisfied
     * 1 - method is not supported
     * 2 - condition is not satisfied
     */
    if (CMGetArrayCount(d.value.array, NULL) == 0) {
        KUint32_Set(&result, 0);
    } else {
        KUint32_Set(&result, 2);
    }

    KSetStatus(status, OK);

#else
    KUint32_Set(&result, 1);
    KSetStatus(status, ERR_NOT_SUPPORTED);
#endif

    return result;
}

KUint32 LMI_SoftwareIdentityFileCheck_InvokeOnSystem(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_SoftwareIdentityFileCheckRef* self,
    const KRef* TargetSystem,
    CMPIStatus* status)
{
    if (TargetSystem && TargetSystem->exists) {
         CMPIStatus st = lmi_check_required_properties(cb, context,
                 TargetSystem->value, "CreationClassName", "Name");

        if (st.rc != CMPI_RC_OK) {
            KUint32 result = KUINT32_INIT;
            KSetStatus(status, ERR_NOT_FOUND);
            return result;
        }
    }

    return LMI_SoftwareIdentityFileCheck_Invoke(cb, mi, context, self, status);
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SoftwareIdentityFileCheck",
    "LMI_SoftwareIdentityFileCheck",
    "instance method")
