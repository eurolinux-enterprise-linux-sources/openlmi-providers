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
#include "CIM_LogicalFile.h"
#include "CIM_UnixFile.h"
#include "LMI_FileIdentity.h"
#include "LMI_UnixFile.h"
#include "LMI_DataFile.h"
#include "file.h"

static const CMPIBroker* _cb;

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
    CMPIStatus st, res;
    CMPIInstance *ci;
    CMPIObjectPath *o;
    const char *ns = KNameSpace(cop);
    const char *path;
    char fileclass[BUFLEN];
    char *fsname;
    char *fsclassname;

    st = check_assoc_class(_cb, ns, assocClass, LMI_FileIdentity_ClassName);
    check_class_check_status(st);

    if (CMClassPathIsA(_cb, cop, LMI_UnixFile_ClassName, &st)) {
        /* got UnixFile - SameElement */
        st = lmi_check_required(_cb, cc, cop);
        check_status(st);

        path = get_string_property_from_op(cop, "LFName");
        get_class_from_path(path, fileclass);
        st = get_fsinfo_from_path(_cb, path, &fsclassname, &fsname);
        check_status(st);

        st = check_assoc_class(_cb, ns, resultClass, fileclass);
        check_class_check_status(st);
        if (role && strcmp(role, SAME_ELEMENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }
        if (resultRole && strcmp(resultRole, SYSTEM_ELEMENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }

        CIM_LogicalFileRef cim_lfr;
        CIM_LogicalFileRef_Init(&cim_lfr, _cb, ns);
        fill_logicalfile(CIM_LogicalFileRef, &cim_lfr, path, fsclassname, fsname, fileclass);
        o = CIM_LogicalFileRef_ToObjectPath(&cim_lfr, &st);
        CMSetClassName(o, fileclass);
    } else if (CMClassPathIsA(_cb, cop, CIM_LogicalFile_ClassName, &st)) {
        /* got LogicalFile - SystemElement */
        st = lmi_check_required(_cb, cc, cop);
        check_status(st);

        path = get_string_property_from_op(cop, "Name");

        st = check_assoc_class(_cb, ns, resultClass, LMI_UnixFile_ClassName);
        check_class_check_status(st);
        if (role && strcmp(role, SYSTEM_ELEMENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }
        if (resultRole && strcmp(resultRole, SAME_ELEMENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }

        get_class_from_path(path, fileclass);
        st = get_fsinfo_from_path(_cb, path, &fsclassname, &fsname);
        check_status(st);

        LMI_UnixFile lmi_uf;
        LMI_UnixFile_Init(&lmi_uf, _cb, ns);
        LMI_UnixFile_Set_LFName(&lmi_uf, path);
        LMI_UnixFile_Set_CSCreationClassName(&lmi_uf, get_system_creation_class_name());
        LMI_UnixFile_Set_CSName(&lmi_uf, get_system_name());
        LMI_UnixFile_Set_FSCreationClassName(&lmi_uf, fsclassname);
        LMI_UnixFile_Set_FSName(&lmi_uf, fsname);
        LMI_UnixFile_Set_LFCreationClassName(&lmi_uf, fileclass);
        o = LMI_UnixFile_ToObjectPath(&lmi_uf, &st);
    } else {
        /* this association does not associate with given 'cop' class */
        CMReturn(CMPI_RC_OK);
    }

    if (names) {
        res = CMReturnObjectPath(cr, o);
    } else {
        ci = _cb->bft->getInstance(_cb, cc, o, properties, &st);
        res = CMReturnInstance(cr, ci);
    }
    free(fsname);
    return res;
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
    LMI_FileIdentity lmi_fi;
    CMPIStatus st, res;
    const char *ns = KNameSpace(cop);
    CMPIInstance *ci;
    CMPIObjectPath *o;
    const char *path;
    char fileclass[BUFLEN];
    char *fsname;
    char *fsclassname;

    st = check_assoc_class(_cb, ns, assocClass, LMI_FileIdentity_ClassName);
    check_class_check_status(st);

    LMI_FileIdentity_Init(&lmi_fi, _cb, ns);

    if (CMClassPathIsA(_cb, cop, LMI_UnixFile_ClassName, &st)) {
        /* got UnixFile - SameElement */
        LMI_FileIdentity_SetObjectPath_SameElement(&lmi_fi, cop);

        st = lmi_check_required(_cb, cc, cop);
        if (st.rc != CMPI_RC_OK) {
            return st;
        }

        path = get_string_property_from_op(cop, "LFName");
        get_class_from_path(path, fileclass);
        st = get_fsinfo_from_path(_cb, path, &fsclassname, &fsname);
        check_status(st);

        if (role && strcmp(role, SAME_ELEMENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }

        /* SystemElement */
        CIM_LogicalFileRef lmi_lfr;
        CIM_LogicalFileRef_Init(&lmi_lfr, _cb, ns);
        fill_logicalfile(CIM_LogicalFileRef, &lmi_lfr, path, fsclassname, fsname, fileclass);

        o = CIM_LogicalFileRef_ToObjectPath(&lmi_lfr, &st);
        CMSetClassName(o, fileclass);
        LMI_FileIdentity_SetObjectPath_SystemElement(&lmi_fi, o);
    } else if (CMClassPathIsA(_cb, cop, CIM_LogicalFile_ClassName, &st)) {
        /* got LogicalFile - SystemElement */
        LMI_FileIdentity_SetObjectPath_SystemElement(&lmi_fi, cop);

        st = lmi_check_required(_cb, cc, cop);
        if (st.rc != CMPI_RC_OK) {
            return st;
        }

        if (role && strcmp(role, SYSTEM_ELEMENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }
        path = get_string_property_from_op(cop, "Name");
        get_class_from_path(path, fileclass);
        st = get_fsinfo_from_path(_cb, path, &fsclassname, &fsname);
        check_status(st);

        /* SameElement */
        LMI_UnixFile lmi_uf;
        LMI_UnixFile_Init(&lmi_uf, _cb, ns);
        LMI_UnixFile_Set_LFName(&lmi_uf, path);
        LMI_UnixFile_Set_CSCreationClassName(&lmi_uf, get_system_creation_class_name());
        LMI_UnixFile_Set_CSName(&lmi_uf, get_system_name());
        LMI_UnixFile_Set_FSCreationClassName(&lmi_uf, fsclassname);
        LMI_UnixFile_Set_FSName(&lmi_uf, fsname);
        LMI_UnixFile_Set_LFCreationClassName(&lmi_uf, fileclass);
        o = LMI_UnixFile_ToObjectPath(&lmi_uf, &st);
        LMI_FileIdentity_SetObjectPath_SameElement(&lmi_fi, o);
    } else {
        /* this association does not associate with given 'cop' class */
        CMReturn(CMPI_RC_OK);
    }

    if (names) {
        o = LMI_FileIdentity_ToObjectPath(&lmi_fi, &st);
        res = CMReturnObjectPath(cr, o);
    } else {
        ci = LMI_FileIdentity_ToInstance(&lmi_fi, &st);
        res = CMReturnInstance(cr, ci);
    }
    free(fsname);
    return res;
}

static void LMI_FileIdentityInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_FileIdentityCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FileIdentityEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FileIdentityEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FileIdentityGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    /* TODO TBI */
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FileIdentityCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FileIdentityModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FileIdentityDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FileIdentityExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FileIdentityAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FileIdentityAssociators(
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
    return associators(mi, cc, cr, cop, assocClass, resultClass,
                       role, resultRole, properties, 0);
}

static CMPIStatus LMI_FileIdentityAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return associators(mi, cc, cr, cop, assocClass, resultClass,
                       role, resultRole, NULL, 1);
}

static CMPIStatus LMI_FileIdentityReferences(
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

static CMPIStatus LMI_FileIdentityReferenceNames(
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
    LMI_FileIdentity,
    LMI_FileIdentity,
    _cb,
    LMI_FileIdentityInitialize(ctx))

CMAssociationMIStub(
    LMI_FileIdentity,
    LMI_FileIdentity,
    _cb,
    LMI_FileIdentityInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_FileIdentity",
    "LMI_FileIdentity",
    "instance association")
/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* End: */
