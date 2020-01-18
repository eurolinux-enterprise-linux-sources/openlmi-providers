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
#include <konkret/konkret.h>
#include <sys/time.h>
#include "LMI_UnixFile.h"
#include "file.h"

static const CMPIBroker* _cb = NULL;

#ifdef LOGICALFILE_SELINUX
#include <selinux/selinux.h>
#include <selinux/label.h>
#include <pthread.h>

static pthread_mutex_t selinux_mutex;

static struct selabel_handle *_selabel_hnd = NULL;
/* XXX: selabel_close() and freecon() do no work as expected
 * see bug #1008924 */
static struct selabel_handle *get_selabel_handle()
{
    static struct timeval timestamp = {.tv_sec = 0, .tv_usec = 0};
    const unsigned int CHECK_PERIOD = 20; /* seconds */
    const char *err = "gettimeofday() failed, selinux handle might not get re-initialized";

    pthread_mutex_lock(&selinux_mutex);
    if (_selabel_hnd == NULL) {
        _selabel_hnd = selabel_open(SELABEL_CTX_FILE, NULL, 0);
        if (gettimeofday(&timestamp, NULL) < 0) {
            lmi_warn(err);
        }
    } else {
        struct timeval now;
        if (gettimeofday(&now, NULL) < 0) {
            lmi_warn(err);
        }
        /* reinit handle if it's too old */
        if (now.tv_sec - timestamp.tv_sec >= CHECK_PERIOD) {
            selabel_close(_selabel_hnd);
            _selabel_hnd = selabel_open(SELABEL_CTX_FILE, NULL, 0);
            if (gettimeofday(&timestamp, NULL) < 0) {
                lmi_warn(err);
            }
        }
    }
    pthread_mutex_unlock(&selinux_mutex);

    return _selabel_hnd;
}
#endif

static void LMI_UnixFileInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
#ifdef LOGICALFILE_SELINUX
    pthread_mutex_init(&selinux_mutex, NULL);
#endif
}

static CMPIStatus LMI_UnixFileCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
#ifdef LOGICALFILE_SELINUX
    if (_selabel_hnd != NULL) {
        selabel_close(_selabel_hnd);
    }
    pthread_mutex_destroy(&selinux_mutex);
#endif
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_UnixFileEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_UnixFileEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_UnixFileGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_UnixFile lmi_file;
    CMPIStatus st;
    struct stat sb;
    char aux[BUFLEN];
    const char *path;
    char *fsname;
    char *fsclassname;

    st = lmi_check_required(_cb, cc, cop);
    if (st.rc != CMPI_RC_OK) {
        return st;
    }

    LMI_UnixFile_InitFromObjectPath(&lmi_file, _cb, cop);
    path = KChars(lmi_file.LFName.value);

    if (lstat(path, &sb) < 0) {
        snprintf(aux, BUFLEN, "Can't stat file: %s", path);
        CMReturnWithChars(_cb, CMPI_RC_ERR_NOT_FOUND, aux);
    }
    /* set ignored stuff */
    st = get_fsinfo_from_stat(_cb, &sb, path, &fsclassname, &fsname);
    check_status(st);
    LMI_UnixFile_Set_FSCreationClassName(&lmi_file, fsclassname);
    LMI_UnixFile_Set_FSName(&lmi_file, fsname);
    free(fsname);
    get_class_from_stat(&sb, aux);
    LMI_UnixFile_Set_LFCreationClassName(&lmi_file, aux);

    /* set unix-specific stuff */
    LMI_UnixFile_Set_Name(&lmi_file, path);
    sprintf(aux, "%u", sb.st_uid);
    LMI_UnixFile_Set_UserID(&lmi_file, aux);
    sprintf(aux, "%u", sb.st_gid);
    LMI_UnixFile_Set_GroupID(&lmi_file, aux);
    LMI_UnixFile_Set_SetUid(&lmi_file, sb.st_mode & S_IFMT & S_ISUID);
    LMI_UnixFile_Set_SetGid(&lmi_file, sb.st_mode & S_IFMT & S_ISGID);
    sprintf(aux, "%u", (unsigned int)sb.st_ino);
    LMI_UnixFile_Set_FileInodeNumber(&lmi_file, aux);
    LMI_UnixFile_Set_LinkCount(&lmi_file, sb.st_nlink);
    /* sticky bit */
    LMI_UnixFile_Set_SaveText(&lmi_file, sb.st_mode & S_IFMT & S_ISVTX);
#ifdef LOGICALFILE_SELINUX
    /* selinux */
    security_context_t context;
    struct selabel_handle *hnd;
    if (lgetfilecon(path, &context) < 0) {
        lmi_warn("Can't get selinux file context: %s", path);
        context = strdup("<<none>>");
    }
    LMI_UnixFile_Set_SELinuxCurrentContext(&lmi_file, context);
    freecon(context);
    hnd = get_selabel_handle();
    if (hnd == NULL) {
        CMReturnWithChars(_cb, CMPI_RC_ERR_NOT_FOUND, "Can't get selabel handle");
    }
    if (selabel_lookup(hnd, &context, path, 0) < 0) {
        lmi_warn("Can't look up expected selinux file context: %s", path);
        context = strdup("<<none>>");
    }
    LMI_UnixFile_Set_SELinuxExpectedContext(&lmi_file, context);
    freecon(context);
#endif

    KReturnInstance(cr, lmi_file);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_UnixFileCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_UnixFileModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_UnixFileDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_UnixFileExecQuery(
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
    LMI_UnixFile,
    LMI_UnixFile,
    _cb,
    LMI_UnixFileInitialize(ctx))

static CMPIStatus LMI_UnixFileMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_UnixFileInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_UnixFile_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_UnixFile,
    LMI_UnixFile,
    _cb,
    LMI_UnixFileInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_UnixFile",
    "LMI_UnixFile",
    "instance method")
/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* End: */
