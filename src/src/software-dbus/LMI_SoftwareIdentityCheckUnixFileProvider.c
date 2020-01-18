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
#include "LMI_SoftwareIdentityCheckUnixFile.h"
#include "sw-utils.h"
#include "openlmi-utils.h"
#include "config.h"

#ifdef RPM_FOUND
#include <rpm/rpmlib.h>
#include <rpm/rpmdb.h>
#include <rpm/rpmts.h>
#endif

static const CMPIBroker* _cb;

#ifdef RPM_FOUND
/* Mutex for transaction set */
static pthread_mutex_t ts_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static void LMI_SoftwareIdentityCheckUnixFileInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SoftwareIdentityCheckUnixFile_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_SoftwareIdentityCheckUnixFile_ClassName);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
#ifndef RPM_FOUND
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
#else
    const char *elem_name, *file_name, *file_name_unix_file;

    LMI_SoftwareIdentityCheckUnixFile w;
    LMI_SoftwareIdentityCheckUnixFile_InitFromObjectPath(&w, _cb, cop);

    elem_name = lmi_get_string_property_from_objectpath(w.Check.value, "SoftwareElementID");
    file_name = lmi_get_string_property_from_objectpath(w.Check.value, "Name");
    file_name_unix_file = lmi_get_string_property_from_objectpath(w.File.value, "LFName");

    if (strcmp(file_name, file_name_unix_file) != 0) {
        CMReturn(CMPI_RC_ERR_NOT_FOUND);
    }

    if (!is_file_part_of_elem_name(file_name, elem_name)) {
        CMReturn(CMPI_RC_ERR_NOT_FOUND);
    }

    KReturnInstance(cr, w);
    CMReturn(CMPI_RC_OK);
#endif
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

#ifdef RPM_FOUND
static CMPIStatus get_unixfile_ref(const char *ns, const CMPIContext* cc,
        const char *path, LMI_UnixFileRef *uf)
{
    CMPIStatus st;
    char fileclass[BUFLEN], *fsname = NULL, *fsclassname = NULL;

    get_logfile_class_from_path(path, fileclass);
    st = get_fsinfo_from_path(_cb, path, &fsclassname, &fsname);
    lmi_return_if_status_not_ok(st);

    LMI_UnixFileRef_Init(uf, _cb, ns);
    LMI_UnixFileRef_Set_CSCreationClassName(uf,
            lmi_get_system_creation_class_name());
    LMI_UnixFileRef_Set_CSName(uf, lmi_get_system_name_safe(cc));
    LMI_UnixFileRef_Set_FSCreationClassName(uf, fsclassname);
    LMI_UnixFileRef_Set_FSName(uf, fsname);
    LMI_UnixFileRef_Set_LFCreationClassName(uf, fileclass);
    LMI_UnixFileRef_Set_LFName(uf, path);

    free(fsname);

    CMReturn(CMPI_RC_OK);
}

static short get_swidentityfilecheck_ref(const char *ns, const char *file_name,
        GPtrArray **garray, char *error_msg, const unsigned error_msg_len)
{
    rpmts ts = NULL;
    Header rpm_header;
    rpmdbMatchIterator iter = NULL;
    SwPackage sw_pkg;
    short ret = -1;
    char *nevra, sw_pkg_ver_str[BUFLEN] = "", instance_id[BUFLEN] = "";
    LMI_SoftwareIdentityFileCheckRef *sifc;

    init_sw_package(&sw_pkg);

    create_instance_id(LMI_SoftwareIdentityFileCheck_ClassName, NULL,
            instance_id, BUFLEN);

    pthread_mutex_lock(&ts_mutex);
    if (rpmReadConfigFiles(NULL, NULL) != 0) {
        pthread_mutex_unlock(&ts_mutex);
        snprintf(error_msg, error_msg_len, "Failed to read RPM configuration files.");
        goto done;
    }

    if (!(ts = rpmtsCreate())) {
        pthread_mutex_unlock(&ts_mutex);
        snprintf(error_msg, error_msg_len, "Failed to create RPM transaction set.");
        goto done;
    }

    iter = rpmtsInitIterator(ts, RPMDBI_INSTFILENAMES, file_name, 0);
    if (!iter || rpmdbGetIteratorCount(iter) <= 0) {
        pthread_mutex_unlock(&ts_mutex);
        goto done;
    }
    pthread_mutex_unlock(&ts_mutex);

    *garray = g_ptr_array_new_with_free_func(g_free);

    while ((rpm_header = rpmdbNextIterator(iter))) {
        if (!(nevra = headerGetAsString(rpm_header, RPMTAG_NEVRA))) {
            continue;
        }

        if (create_sw_package_from_elem_name(nevra, &sw_pkg) != 0) {
            free(nevra);
            nevra = NULL;
            continue;
        }

        sw_pkg_get_version_str(&sw_pkg, sw_pkg_ver_str, BUFLEN);

        sifc = g_new0(LMI_SoftwareIdentityFileCheckRef, 1);
        LMI_SoftwareIdentityFileCheckRef_Init(sifc, _cb, ns);
        LMI_SoftwareIdentityFileCheckRef_Set_CheckID(sifc, instance_id);
        LMI_SoftwareIdentityFileCheckRef_Set_SoftwareElementState(sifc,
                LMI_SoftwareIdentityFileCheckRef_SoftwareElementState_Executable);
        LMI_SoftwareIdentityFileCheckRef_Set_TargetOperatingSystem(sifc,
                LMI_SoftwareIdentityFileCheckRef_TargetOperatingSystem_LINUX);
        LMI_SoftwareIdentityFileCheckRef_Set_SoftwareElementID(sifc, nevra);
        LMI_SoftwareIdentityFileCheckRef_Set_Version(sifc, sw_pkg_ver_str);
        LMI_SoftwareIdentityFileCheckRef_Set_Name(sifc, file_name);

        g_ptr_array_add(*garray, sifc);

        free(nevra);
        nevra = NULL;
        clean_sw_package(&sw_pkg);
    }

    ret = 0;

done:
    if (iter) {
        rpmdbFreeIterator(iter);
    }
    pthread_mutex_lock(&ts_mutex);
    if (ts) {
        rpmtsFree(ts);
        ts = NULL;
    }
    pthread_mutex_unlock(&ts_mutex);

    return ret;
}
#endif

static CMPIStatus associators(
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole,
    const char** properties,
    const short names)
{
#ifdef RPM_FOUND
    CMPIStatus st;
    const char *file_name;
    char error_msg[BUFLEN] = "";

    st = lmi_class_path_is_a(_cb, KNameSpace(cop),
            LMI_SoftwareIdentityCheckUnixFile_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);

    if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentityFileCheck_ClassName, &st)) {
        /* got SoftwareIdentityFileCheck - Check */
        const char *elem_name;

        st = lmi_class_path_is_a(_cb, KNameSpace(cop),
                LMI_UnixFile_ClassName, resultClass);
        lmi_return_if_class_check_not_ok(st);

        if (role && strcmp(role, LMI_CHECK) != 0) {
            goto done;
        }
        if (resultRole && strcmp(resultRole, LMI_FILE) != 0) {
            goto done;
        }

        /* Is file part of this package? */
        elem_name = lmi_get_string_property_from_objectpath(cop,
                "SoftwareElementID");
        file_name = lmi_get_string_property_from_objectpath(cop, "Name");
        if (!is_file_part_of_elem_name(file_name, elem_name)) {
            goto done;
        }

        LMI_UnixFileRef uf;
        st = get_unixfile_ref(KNameSpace(cop), cc, file_name, &uf);
        lmi_return_if_status_not_ok(st);

        if (names) {
            KReturnObjectPath(cr, uf);
        } else {
            CMPIObjectPath *o = LMI_UnixFileRef_ToObjectPath(&uf, &st);
            CMPIInstance *ci = _cb->bft->getInstance(_cb, cc, o, properties, &st);
            CMReturnInstance(cr, ci);
        }
    } else if (CMClassPathIsA(_cb, cop, LMI_UnixFile_ClassName, &st)) {
        /* got UnixFile - File */
        unsigned i;
        GPtrArray *garray = NULL;
        LMI_SoftwareIdentityFileCheckRef *sifc;

        st = lmi_class_path_is_a(_cb, KNameSpace(cop),
                LMI_SoftwareIdentityFileCheck_ClassName, resultClass);
        lmi_return_if_class_check_not_ok(st);

        if (role && strcmp(role, LMI_FILE) != 0) {
            goto done;
        }
        if (resultRole && strcmp(resultRole, LMI_CHECK) != 0) {
            goto done;
        }

        file_name = lmi_get_string_property_from_objectpath(cop, "LFName");

        if (get_swidentityfilecheck_ref(KNameSpace(cop), file_name, &garray,
                error_msg, BUFLEN) != 0 || !garray) {
            goto done;
        }

        for (i = 0; i < garray->len; i++) {
            sifc = (LMI_SoftwareIdentityFileCheckRef *)g_ptr_array_index(garray, i);

            if (names) {
                KReturnObjectPath(cr, *sifc);
            } else {
                CMPIObjectPath *o = LMI_SoftwareIdentityFileCheckRef_ToObjectPath(
                        sifc, &st);
                CMPIInstance *ci = _cb->bft->getInstance(_cb, cc, o, properties,
                        &st);
                CMReturnInstance(cr, ci);
            }
        }

        g_ptr_array_free(garray, TRUE);
    }

done:
    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }
#endif

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileAssociators(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole,
    const char** properties)
{
    return associators(cc, cr, cop, assocClass, resultClass, role,
            resultRole, properties, 0);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return associators(cc, cr, cop, assocClass, resultClass, role,
            resultRole, NULL, 1);
}

static CMPIStatus references(
    const CMPIContext *cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const short names)
{
#ifdef RPM_FOUND
    CMPIStatus st;
    const char *file_name;
    char error_msg[BUFLEN] = "";

    st = lmi_class_path_is_a(_cb, KNameSpace(cop),
            LMI_SoftwareIdentityCheckUnixFile_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);

    if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentityFileCheck_ClassName, &st)) {
        /* got SoftwareIdentityFileCheck - Check */
        const char *elem_name;

        if (role && strcmp(role, LMI_CHECK) != 0) {
            goto done;
        }

        /* Is file part of this package? */
        elem_name = lmi_get_string_property_from_objectpath(cop,
                "SoftwareElementID");
        file_name = lmi_get_string_property_from_objectpath(cop, "Name");
        if (!is_file_part_of_elem_name(file_name, elem_name)) {
            goto done;
        }

        LMI_UnixFileRef uf;
        st = get_unixfile_ref(KNameSpace(cop), cc, file_name, &uf);
        lmi_return_if_status_not_ok(st);

        LMI_SoftwareIdentityCheckUnixFile w;
        LMI_SoftwareIdentityCheckUnixFile_Init(&w, _cb, KNameSpace(cop));
        LMI_SoftwareIdentityCheckUnixFile_SetObjectPath_Check(&w, cop);
        LMI_SoftwareIdentityCheckUnixFile_Set_File(&w, &uf);

        if (names) {
            KReturnObjectPath(cr, w);
        } else {
            KReturnInstance(cr, w);
        }
    } else if (CMClassPathIsA(_cb, cop, LMI_UnixFile_ClassName, &st)) {
        /* got UnixFile - File */
        unsigned i;
        GPtrArray *garray = NULL;
        LMI_SoftwareIdentityFileCheckRef *sifc;

        if (role && strcmp(role, LMI_FILE) != 0) {
            goto done;
        }

        file_name = lmi_get_string_property_from_objectpath(cop, "LFName");

        if (get_swidentityfilecheck_ref(KNameSpace(cop), file_name, &garray,
                error_msg, BUFLEN) != 0 || !garray) {
            goto done;
        }

        for (i = 0; i < garray->len; i++) {
            sifc = (LMI_SoftwareIdentityFileCheckRef *)g_ptr_array_index(garray, i);

            LMI_SoftwareIdentityCheckUnixFile w;
            LMI_SoftwareIdentityCheckUnixFile_Init(&w, _cb, KNameSpace(cop));
            LMI_SoftwareIdentityCheckUnixFile_Set_Check(&w, sifc);
            LMI_SoftwareIdentityCheckUnixFile_SetObjectPath_File(&w, cop);

            if (names) {
                KReturnObjectPath(cr, w);
            } else {
                KReturnInstance(cr, w);
            }
        }

        g_ptr_array_free(garray, TRUE);
    }

done:
    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }
#endif

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileReferences(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    return references(cc, cr, cop, assocClass, role, 0);
}

static CMPIStatus LMI_SoftwareIdentityCheckUnixFileReferenceNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role)
{
    return references(cc, cr, cop, assocClass, role, 1);
}

CMInstanceMIStub(
    LMI_SoftwareIdentityCheckUnixFile,
    LMI_SoftwareIdentityCheckUnixFile,
    _cb,
    LMI_SoftwareIdentityCheckUnixFileInitialize(ctx))

CMAssociationMIStub(
    LMI_SoftwareIdentityCheckUnixFile,
    LMI_SoftwareIdentityCheckUnixFile,
    _cb,
    LMI_SoftwareIdentityCheckUnixFileInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SoftwareIdentityCheckUnixFile",
    "LMI_SoftwareIdentityCheckUnixFile",
    "instance association")
