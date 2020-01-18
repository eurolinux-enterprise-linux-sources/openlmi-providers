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
#include "LMI_RootDirectory.h"
#include "LMI_UnixDirectory.h"
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
    const int names)
{
    CMPIObjectPath *o;
    CMPIInstance *ci;
    CMPIStatus st;
    const char *ns = KNameSpace(cop);
    const char *comp_ccname = get_system_creation_class_name();
    const char *path = get_string_property_from_op(cop, "Name");
    char *fsname;

    st = check_assoc_class(_cb, ns, assocClass, LMI_RootDirectory_ClassName);
    check_class_check_status(st);

    st = get_fsname_from_path(_cb, "/", &fsname);
    check_status(st);

    if (CMClassPathIsA(_cb, cop, LMI_UnixDirectory_ClassName, &st)) {
        /* got LMI_UnixDirectory - PartComponent */
        st = check_assoc_class(_cb, ns, resultClass, comp_ccname);
        check_class_check_status(st);
        if (role && strcmp(role, PART_COMPONENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }
        if (resultRole && strcmp(resultRole, GROUP_COMPONENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }
        /* ignore this association if the directory is not root */
        if (strcmp(path, "/") != 0) {
            CMReturn(CMPI_RC_OK);
        }

        if (names) {
            CMReturnObjectPath(cr, lmi_get_computer_system());
        } else {
            ci = _cb->bft->getInstance(_cb, cc, lmi_get_computer_system(), properties, &st);
            CMReturnInstance(cr, ci);
        }
    } else {
        /* got CIM_ComputerSystem - GroupComponent */
        st = check_assoc_class(_cb, ns, resultClass, LMI_UnixDirectory_ClassName);
        check_class_check_status(st);
        if (role && strcmp(role, GROUP_COMPONENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }
        if (resultRole && strcmp(resultRole, PART_COMPONENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }

        LMI_UnixDirectory lmi_ud;
        LMI_UnixDirectory_Init(&lmi_ud, _cb, ns);
        fill_logicalfile(LMI_UnixDirectory, &lmi_ud, "/", fsname, LMI_UnixDirectory_ClassName);
        o = LMI_UnixDirectory_ToObjectPath(&lmi_ud, &st);
        if (names) {
            CMReturnObjectPath(cr, o);
        } else {
            ci = _cb->bft->getInstance(_cb, cc, o, properties, &st);
            CMReturnInstance(cr, ci);
        }
    }
    free(fsname);
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
    const int names)
{
    LMI_RootDirectory lmi_rd;
    CMPIObjectPath *o;
    CMPIInstance *ci;
    CMPIStatus st;
    const char *ns = KNameSpace(cop);
    const char *path = get_string_property_from_op(cop, "Name");
    char ccname[BUFLEN];
    get_class_from_path(path, ccname);

    st = check_assoc_class(_cb, ns, assocClass, LMI_RootDirectory_ClassName);
    check_class_check_status(st);

    char *fsname;
    st = get_fsname_from_path(_cb, "/", &fsname);
    check_status(st);

    LMI_RootDirectory_Init(&lmi_rd, _cb, ns);

    if (strcmp(ccname, LMI_UnixDirectory_ClassName) == 0) {
        /* got UnixDirectory - PartComponent */
        if (role && strcmp(role, PART_COMPONENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }
        /* ignore this association if the directory is not root */
        if (strcmp(path, "/") != 0) {
            CMReturn(CMPI_RC_OK);
        }
        st = lmi_check_required(_cb, cc, cop);
        check_status(st);
        LMI_RootDirectory_SetObjectPath_PartComponent(&lmi_rd, cop);

        LMI_RootDirectory_SetObjectPath_GroupComponent(&lmi_rd,
                lmi_get_computer_system());
    } else {
        /* got CIM_ComputerSystem - GroupComponent */
        if (role && strcmp(role, GROUP_COMPONENT) != 0) {
            CMReturn(CMPI_RC_OK);
        }
        LMI_RootDirectory_SetObjectPath_GroupComponent(&lmi_rd, cop);

        LMI_UnixDirectory lmi_ud;
        LMI_UnixDirectory_Init(&lmi_ud, _cb, ns);
        fill_logicalfile(LMI_UnixDirectory, &lmi_ud, "/", fsname, LMI_UnixDirectory_ClassName);
        o = LMI_UnixDirectory_ToObjectPath(&lmi_ud, &st);
        LMI_RootDirectory_SetObjectPath_PartComponent(&lmi_rd, o);
    }

    if (names) {
        o = LMI_RootDirectory_ToObjectPath(&lmi_rd, &st);
        CMReturnObjectPath(cr, o);
    } else {
        ci = LMI_RootDirectory_ToInstance(&lmi_rd, &st);
        CMReturnInstance(cr, ci);
    }
    free(fsname);
    CMReturn(CMPI_RC_OK);
}

static void LMI_RootDirectoryInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_RootDirectoryCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_RootDirectoryEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_RootDirectoryEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIObjectPath *o;
    CMPIStatus st;
    char *fsname;
    const char *ns = KNameSpace(cop);

    LMI_RootDirectory lmi_rd;
    LMI_RootDirectory_Init(&lmi_rd, _cb, ns);

    LMI_RootDirectory_SetObjectPath_GroupComponent(&lmi_rd, lmi_get_computer_system());

    LMI_UnixDirectory lmi_ud;
    LMI_UnixDirectory_Init(&lmi_ud, _cb, ns);
    st = get_fsname_from_path(_cb, "/", &fsname);
    check_status(st);
    fill_logicalfile(LMI_UnixDirectory, &lmi_ud, "/", fsname, LMI_UnixDirectory_ClassName);
    o = LMI_UnixDirectory_ToObjectPath(&lmi_ud, NULL);
    LMI_RootDirectory_SetObjectPath_PartComponent(&lmi_rd, o);

    free(fsname);
    return CMReturnInstance(cr, LMI_RootDirectory_ToInstance(&lmi_rd, NULL));
}

static CMPIStatus LMI_RootDirectoryGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_RootDirectoryCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_RootDirectoryModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_RootDirectoryDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_RootDirectoryExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_RootDirectoryAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_RootDirectoryAssociators(
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
    return associators(mi, cc, cr, cop, assocClass, resultClass, role,
                       resultRole, properties, 0);
}

static CMPIStatus LMI_RootDirectoryAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return associators(mi, cc, cr, cop, assocClass, resultClass, role,
                       resultRole, NULL, 1);
}

static CMPIStatus LMI_RootDirectoryReferences(
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

static CMPIStatus LMI_RootDirectoryReferenceNames(
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
    LMI_RootDirectory,
    LMI_RootDirectory,
    _cb,
    LMI_RootDirectoryInitialize(ctx))

CMAssociationMIStub(
    LMI_RootDirectory,
    LMI_RootDirectory,
    _cb,
    LMI_RootDirectoryInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_RootDirectory",
    "LMI_RootDirectory",
    "instance association")
/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* End: */
