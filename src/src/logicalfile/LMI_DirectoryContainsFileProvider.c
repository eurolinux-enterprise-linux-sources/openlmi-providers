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
#include <konkret/konkret.h>
#include <glib.h>
#include "LMI_DirectoryContainsFile.h"
#include "LMI_UnixDirectory.h"
#include "CIM_LogicalFile.h"
#include "CIM_Directory.h"
#include "file.h"

static const CMPIBroker* _cb;

/* XXX if a directory contains more than MAX_REFS files,
 * broker will abort with an error */
/* const unsigned int MAX_REFS = 65536; */
const unsigned int MAX_REFS = 4096;

static CMPIStatus logicalfile_objectpath_from_path(
    const CMPIContext *ctx,
    const char *abspath,
    const char *resultClass,
    const char *namespace,
    CMPIObjectPath **o)
{
    struct stat sb;
    CMPIStatus st = {.rc = CMPI_RC_OK};

    char fileclass[BUFLEN];
    char *fsname = NULL;
    char *fsclassname = NULL;

    if (lstat(abspath, &sb) < 0) {
        char err[BUFLEN];
        snprintf(err, BUFLEN, "Can't stat file: %s", abspath);
        CMReturnWithChars(_cb, CMPI_RC_ERR_NOT_FOUND, err);
    } else {
        get_logfile_class_from_stat(&sb, fileclass);
        st = lmi_class_path_is_a(_cb, namespace, fileclass, resultClass);
        if (st.rc == LMI_RC_ERR_CLASS_CHECK_FAILED) {
            st.rc = CMPI_RC_OK;
            *o = NULL;
            return st;
        }
        /* check status again for other possible errors */
        lmi_return_if_status_not_ok(st);

        st = get_fsinfo_from_stat(_cb, &sb, abspath, &fsclassname, &fsname);
        lmi_return_if_status_not_ok(st);
    }

    CIM_LogicalFileRef cim_lfr;
    CIM_LogicalFileRef_Init(&cim_lfr, _cb, namespace);
    fill_logicalfile(ctx, CIM_LogicalFileRef, &cim_lfr, abspath, fsclassname, fsname, fileclass);
    *o = CIM_LogicalFileRef_ToObjectPath(&cim_lfr, &st);
    CMSetClassName(*o, fileclass);

    return st;
}

static CMPIStatus dir_file_objectpaths(
    const CMPIContext* cc,
    const CMPIResult* cr,
    const char* resultClass,
    int group,
    int rgroup,
    const char** properties,
    const char *namespace,
    const char *path,
    CMPIObjectPath **ops,
    unsigned int *count)
{
    CMPIObjectPath *o = NULL;
    CMPIStatus st = {.rc = CMPI_RC_OK};
    unsigned int i = 0;

    GError *gerror = NULL;
    GDir *gdir;
    const gchar *gde;

    gdir = g_dir_open(path, 0, &gerror);
    if (gerror) {
        st.rc = CMPI_RC_ERR_NOT_FOUND;
        return st;
    }

    /* since g_dir_read_name() doesn't go over '.' and '..', get the parent first */
    /* to get the parent directory, if either role or result role is set, role
     * must be GroupComponent, or result role must be PartComponent */
    if (!(group == 1 || rgroup == 0)) {
        gchar *dirname;
        dirname = g_path_get_dirname(path);
        st = logicalfile_objectpath_from_path(cc, dirname, resultClass, namespace, &o);
        g_free(dirname);
        lmi_return_if_status_not_ok(st);
        if (o) {
            ops[i++] = o;
        }
    }

    while ((gde = g_dir_read_name(gdir))) {
        /* to get the content of a directory, if either role or result role
         * is set, role must be PartComponent, or result role must be
         * GroupComponent */
        if (group == 0 || rgroup == 1) {
            continue;
        }

        gchar *rpath = g_build_filename(path, gde, NULL);
        st = logicalfile_objectpath_from_path(cc, rpath, resultClass, namespace, &o);
        g_free(rpath);

        if (st.rc != CMPI_RC_OK) {
            break;
        }
        if (o) {
            ops[i++] = o;
        }
    }
    *count = i;

    g_dir_close(gdir);
    return st;
}

static CMPIStatus associators(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole,
    const char** properties,
    int names)
{
    CMPIObjectPath *o;
    CMPIInstance *ci;
    CMPIStatus st;
    const char *ns = KNameSpace(cop);
    const char *path;
    CMPIObjectPath *refs[MAX_REFS];
    unsigned int count = 0;
    int group = -1;
    int rgroup = -1;

    st = lmi_class_path_is_a(_cb, ns, LMI_DirectoryContainsFile_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);
    /* role && resultRole checks */
    if (role) {
        if (strcmp(role, LMI_GROUP_COMPONENT) != 0 && strcmp(role, LMI_PART_COMPONENT) != 0) {
            CMReturn(CMPI_RC_OK);
        } else if (strcmp(role, LMI_GROUP_COMPONENT) == 0) {
            group = 1;
        } else if (strcmp(role, LMI_PART_COMPONENT) == 0) {
            group = 0;
        }
    }
    if (resultRole) {
        if (strcmp(resultRole, LMI_GROUP_COMPONENT) != 0 && strcmp(resultRole, LMI_PART_COMPONENT) != 0) {
            CMReturn(CMPI_RC_OK);
        } else if (strcmp(resultRole, LMI_GROUP_COMPONENT) == 0) {
            rgroup = 1;
        } else if (strcmp(resultRole, LMI_PART_COMPONENT) == 0) {
            rgroup = 0;
        }
    }
    /* if role and resultRole are the same, there is nothing left to return */
    if ((group == 1 && rgroup == 1) ||
        (group == 0 && rgroup == 0)) {
        CMReturn(CMPI_RC_OK);
    }

    if (CMClassPathIsA(_cb, cop, LMI_UnixDirectory_ClassName, &st)) {
        /* got UnixDirectory - GroupComponent */
        st = lmi_check_required_properties(_cb, cc, cop, "CSCreationClassName", "CSName");
        if (st.rc != CMPI_RC_OK) {
            return st;
        }
        path = lmi_get_string_property_from_objectpath(cop, "Name");
        if (!path)
            CMReturnWithChars(_cb, CMPI_RC_ERR_NOT_FOUND, "Cannot find Name property in provided LMI_UnixDirectory");
        st = dir_file_objectpaths(cc, cr, resultClass, group, rgroup, properties, ns, path, refs, &count);
        if (st.rc != CMPI_RC_OK) {
            return st;
        }
        if (count > MAX_REFS) {
            CMReturnWithChars(_cb, CMPI_RC_ERR_NOT_FOUND, "Too many files in a single directory...");
        }

        /* TODO: directories are walked in two passes
         * rewrite using only one pass */
        for (unsigned int i = 0; i < count; i++) {
            if (names) {
                CMReturnObjectPath(cr, refs[i]);
            } else {
                ci = _cb->bft->getInstance(_cb, cc, refs[i], properties, &st);
                CMReturnInstance(cr, ci);
            }
        }
    } else if (CMClassPathIsA(_cb, cop, CIM_LogicalFile_ClassName, &st)) {
        /* got LogicalFile - PartComponent */
        st = lmi_check_required_properties(_cb, cc, cop, "CSCreationClassName", "CSName");
        if (st.rc != CMPI_RC_OK) {
            return st;
        }
        if (group == 1 || rgroup == 0) {
            CMReturn(CMPI_RC_OK);
        }
        path = lmi_get_string_property_from_objectpath(cop, "Name");
        if (!path)
            CMReturnWithChars(_cb, CMPI_RC_ERR_NOT_FOUND, "Cannot find Name property in provided CIM_logicalFile");
        st = lmi_class_path_is_a(_cb, ns, LMI_UnixDirectory_ClassName, resultClass);
        lmi_return_if_class_check_not_ok(st);

        CIM_DirectoryRef lmi_dr;
        CIM_DirectoryRef_Init(&lmi_dr, _cb, ns);

        char *fsname = NULL;
        char *fsclassname = NULL;

        st = get_fsinfo_from_path(_cb, path, &fsclassname, &fsname);
        lmi_return_if_status_not_ok(st);

        gchar *dirname = g_path_get_dirname(path);
        fill_logicalfile(cc, CIM_DirectoryRef, &lmi_dr, dirname, fsclassname, fsname, LMI_UnixDirectory_ClassName);
        g_free(dirname);
        o = CIM_DirectoryRef_ToObjectPath(&lmi_dr, &st);
        CMSetClassName(o, LMI_UnixDirectory_ClassName);

        if (names) {
            CMReturnObjectPath(cr, o);
        } else {
            ci = _cb->bft->getInstance(_cb, cc, o, properties, &st);
            CMReturnInstance(cr, ci);
        }
    } else {
        /* this association does not associate with given 'cop' class */
        CMReturn(CMPI_RC_OK);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus references(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties,
    int names)
{
    LMI_DirectoryContainsFile lmi_dcf;
    CMPIStatus st;
    const char *ns = KNameSpace(cop);
    const char *path;
    char *fsname = NULL;
    char *fsclassname = NULL;
    char ccname[BUFLEN];

    /* GroupComponent */
    CIM_DirectoryRef lmi_dr;
    /* PartComponent */
    CIM_LogicalFileRef lmi_lfr;
    CMPIObjectPath *o;
    CMPIInstance *ci;
    int group = -1;

    st = lmi_class_path_is_a(_cb, ns, LMI_DirectoryContainsFile_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);
    if (role) {
        if (strcmp(role, LMI_GROUP_COMPONENT) != 0 && strcmp(role, LMI_PART_COMPONENT) != 0) {
            CMReturn(CMPI_RC_OK);
        } else if (strcmp(role, LMI_GROUP_COMPONENT) == 0) {
            group = 1;
        } else if (strcmp(role, LMI_PART_COMPONENT) == 0) {
            group = 0;
        }
    }

    CIM_DirectoryRef_Init(&lmi_dr, _cb, ns);
    CIM_LogicalFileRef_Init(&lmi_lfr, _cb, ns);
    LMI_DirectoryContainsFile_Init(&lmi_dcf, _cb, ns);


    if (CMClassPathIsA(_cb, cop, LMI_UnixDirectory_ClassName, &st)) {
        st = lmi_check_required_properties(_cb, cc, cop, "CSCreationClassName", "CSName");
        lmi_return_if_status_not_ok(st);
        path = lmi_get_string_property_from_objectpath(cop, "Name");
        if (!path)
            CMReturnWithChars(_cb, CMPI_RC_ERR_NOT_FOUND, "Cannot find Name property in provided LMI_UnixDirectory");
        st = get_fsinfo_from_path(_cb, path, &fsclassname, &fsname);
        lmi_return_if_status_not_ok(st);
        /* got GroupComponent - DirectoryRef */
        fill_logicalfile(cc, CIM_DirectoryRef, &lmi_dr, path, fsclassname, fsname, LMI_UnixDirectory_ClassName);
        o = CIM_DirectoryRef_ToObjectPath(&lmi_dr, &st);
        CMSetClassName(o, LMI_UnixDirectory_ClassName);
        LMI_DirectoryContainsFile_SetObjectPath_GroupComponent(&lmi_dcf, o);

        /* PartComponent */
        CMPIObjectPath *refs[MAX_REFS];
        unsigned int count = 0;
        st = dir_file_objectpaths(cc, cr, NULL, group, -1, properties, ns, path, refs, &count);
        lmi_return_if_status_not_ok(st);
        if (count > MAX_REFS) {
            CMReturnWithChars(_cb, CMPI_RC_ERR_NOT_FOUND, "Too many files in a single directory...");
        }
        /* TODO: directories are walked in two passes
         * rewrite using only one pass */
        for (unsigned int i = 0; i < count; i++) {
            LMI_DirectoryContainsFile_SetObjectPath_PartComponent(&lmi_dcf, refs[i]);
            o = LMI_DirectoryContainsFile_ToObjectPath(&lmi_dcf, &st);
            if (names) {
                CMReturnObjectPath(cr, o);
            } else {
                ci = LMI_DirectoryContainsFile_ToInstance(&lmi_dcf, &st);
                CMReturnInstance(cr, ci);
            }
        }
    } else if (CMClassPathIsA(_cb, cop, CIM_LogicalFile_ClassName, &st)) {
        st = lmi_check_required_properties(_cb, cc, cop, "CSCreationClassName", "CSName");
        lmi_return_if_status_not_ok(st);
        path = lmi_get_string_property_from_objectpath(cop, "Name");
        if (!path)
            CMReturnWithChars(_cb, CMPI_RC_ERR_NOT_FOUND, "Cannot find Name property in provided CIM_LogicalFile");
        get_logfile_class_from_path(path, ccname);
        st = get_fsinfo_from_path(_cb, path, &fsclassname, &fsname);
        lmi_return_if_status_not_ok(st);
        /* got PartComponent - LogicalFileRef */
        if (group == 1) {
            CMReturn(CMPI_RC_OK);
        }

        fill_logicalfile(cc, CIM_LogicalFileRef, &lmi_lfr, path, fsclassname, fsname, ccname);
        o = CIM_LogicalFileRef_ToObjectPath(&lmi_lfr, &st);
        CMSetClassName(o, ccname);
        LMI_DirectoryContainsFile_SetObjectPath_PartComponent(&lmi_dcf, o);

        /* GroupComponent */
        gchar *dirname = g_path_get_dirname(path);
        st = get_fsinfo_from_path(_cb, dirname, &fsclassname, &fsname);
        if (st.rc != CMPI_RC_OK) {
            g_free(dirname);
            return st;
        }

        fill_logicalfile(cc, CIM_DirectoryRef, &lmi_dr, dirname, fsclassname, fsname, LMI_UnixDirectory_ClassName);
        o = CIM_DirectoryRef_ToObjectPath(&lmi_dr, &st);
        CMSetClassName(o, LMI_UnixDirectory_ClassName);
        LMI_DirectoryContainsFile_SetObjectPath_GroupComponent(&lmi_dcf, o);
        o = LMI_DirectoryContainsFile_ToObjectPath(&lmi_dcf, &st);
        if (names) {
            CMReturnObjectPath(cr, o);
        } else {
            ci = LMI_DirectoryContainsFile_ToInstance(&lmi_dcf, &st);
            CMReturnInstance(cr, ci);
        }
        g_free(dirname);
    } else {
        /* this association does not associate with given 'cop' class */
        CMReturn(CMPI_RC_OK);
    }

    CMReturn(CMPI_RC_OK);
}

static void LMI_DirectoryContainsFileInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_DirectoryContainsFileCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DirectoryContainsFileEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DirectoryContainsFileEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DirectoryContainsFileGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    /* TODO TBI */
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DirectoryContainsFileCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DirectoryContainsFileModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DirectoryContainsFileDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DirectoryContainsFileExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DirectoryContainsFileAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}


static CMPIStatus LMI_DirectoryContainsFileAssociators(
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
    return associators(mi, cc, cr, cop, assocClass, resultClass, role, resultRole,
                       properties, 0);
}

static CMPIStatus LMI_DirectoryContainsFileAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return associators(mi, cc, cr, cop, assocClass, resultClass, role, resultRole,
                       NULL, 1);
}

static CMPIStatus LMI_DirectoryContainsFileReferences(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    return references(mi, cc, cr, cop, assocClass, role, properties, 0);
}

static CMPIStatus LMI_DirectoryContainsFileReferenceNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role)
{
    return references(mi, cc, cr, cop, assocClass, role, NULL, 1);
}

CMInstanceMIStub(
    LMI_DirectoryContainsFile,
    LMI_DirectoryContainsFile,
    _cb,
    LMI_DirectoryContainsFileInitialize(ctx))

CMAssociationMIStub(
    LMI_DirectoryContainsFile,
    LMI_DirectoryContainsFile,
    _cb,
    LMI_DirectoryContainsFileInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DirectoryContainsFile",
    "LMI_DirectoryContainsFile",
    "instance association")
/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4 */
/* End: */
