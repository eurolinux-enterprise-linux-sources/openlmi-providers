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
#include "LMI_SoftwareIdentityChecks.h"
#include "sw-utils.h"
#include "config.h"

static const CMPIBroker* _cb;

static void LMI_SoftwareIdentityChecksInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SoftwareIdentityChecks_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_SoftwareIdentityChecksCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_SoftwareIdentityChecks_ClassName);
}

static CMPIStatus LMI_SoftwareIdentityChecksEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityChecksEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityChecksGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
#ifndef RPM_FOUND
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
#else
    const char *elem_name, *file_name, *sw_elem_name;

    LMI_SoftwareIdentityChecks w;
    LMI_SoftwareIdentityChecks_InitFromObjectPath(&w, _cb, cop);

    elem_name = lmi_get_string_property_from_objectpath(w.Check.value,
            "SoftwareElementID");
    file_name = lmi_get_string_property_from_objectpath(w.Check.value, "Name");

    sw_elem_name = get_elem_name_from_instance_id(
            lmi_get_string_property_from_objectpath(w.Element.value,
                    "InstanceID"));

    if (strcmp(elem_name, sw_elem_name) != 0) {
        CMReturn(CMPI_RC_ERR_NOT_FOUND);
    }

    if (!is_file_part_of_elem_name(file_name, elem_name)) {
        CMReturn(CMPI_RC_ERR_NOT_FOUND);
    }

    KReturnInstance(cr, w);

    CMReturn(CMPI_RC_OK);
#endif
}

static CMPIStatus LMI_SoftwareIdentityChecksCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityChecksModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityChecksDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityChecksExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityChecksAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

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
    SwPackage sw_pkg;
    PkPackage *pk_pkg = NULL;
    GStrv files = NULL;
    char error_msg[BUFLEN] = "", instance_id[BUFLEN] = "";

    init_sw_package(&sw_pkg);

    st = lmi_class_path_is_a(_cb, KNameSpace(cop),
            LMI_SoftwareIdentityChecks_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);

    if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentity_ClassName, &st)) {
        /* got SoftwareIdentity - Element */
        unsigned i = 0;
        char sw_pkg_elem_name[BUFLEN] = "", sw_pkg_ver_str[BUFLEN] = "";

        st = lmi_class_path_is_a(_cb, KNameSpace(cop),
                LMI_SoftwareIdentityFileCheck_ClassName, resultClass);
        lmi_return_if_class_check_not_ok(st);

        if (role && strcmp(role, LMI_ELEMENT) != 0) {
            goto done;
        }
        if (resultRole && strcmp(resultRole, LMI_CHECK) != 0) {
            goto done;
        }

        create_instance_id(LMI_SoftwareIdentityFileCheck_ClassName, NULL,
                instance_id, BUFLEN);

        if (get_sw_pkg_from_sw_identity_op(cop, &sw_pkg) != 0) {
            snprintf(error_msg, BUFLEN, "Failed to create software identity.");
            goto done;
        }
        sw_pkg_get_element_name(&sw_pkg, sw_pkg_elem_name, BUFLEN);
        sw_pkg_get_version_str(&sw_pkg, sw_pkg_ver_str, BUFLEN);

        get_pk_pkg_from_sw_pkg(&sw_pkg,
                pk_bitfield_value(PK_FILTER_ENUM_INSTALLED), &pk_pkg);
        if (!pk_pkg) {
            goto done;
        }

        get_files_of_pk_pkg(pk_pkg, &files, error_msg, BUFLEN);
        if (!files) {
            goto done;
        }

        while (files[i]) {
            LMI_SoftwareIdentityFileCheckRef sifc;
            LMI_SoftwareIdentityFileCheckRef_Init(&sifc, _cb, KNameSpace(cop));
            LMI_SoftwareIdentityFileCheckRef_Set_CheckID(&sifc, instance_id);
            LMI_SoftwareIdentityFileCheckRef_Set_SoftwareElementState(&sifc,
                    LMI_SoftwareIdentityFileCheckRef_SoftwareElementState_Executable);
            LMI_SoftwareIdentityFileCheckRef_Set_TargetOperatingSystem(&sifc,
                    LMI_SoftwareIdentityFileCheckRef_TargetOperatingSystem_LINUX);
            LMI_SoftwareIdentityFileCheckRef_Set_SoftwareElementID(&sifc,
                    sw_pkg_elem_name);
            LMI_SoftwareIdentityFileCheckRef_Set_Version(&sifc, sw_pkg_ver_str);
            LMI_SoftwareIdentityFileCheckRef_Set_Name(&sifc, files[i++]);

            if (names) {
                KReturnObjectPath(cr, sifc);
            } else {
                CMPIObjectPath *o = LMI_SoftwareIdentityFileCheckRef_ToObjectPath(
                        &sifc, &st);
                CMPIInstance *ci = _cb->bft->getInstance(_cb, cc, o, properties,
                        &st);
                CMReturnInstance(cr, ci);
            }
        }
    } else if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentityFileCheck_ClassName,
            &st)) {
        /* got SoftwareIdentityFileCheck - Check */
        const char *elem_name, *file_name;

        st = lmi_class_path_is_a(_cb, KNameSpace(cop),
                LMI_SoftwareIdentity_ClassName, resultClass);
        lmi_return_if_class_check_not_ok(st);

        if (role && strcmp(role, LMI_CHECK) != 0) {
            goto done;
        }
        if (resultRole && strcmp(resultRole, LMI_ELEMENT) != 0) {
            goto done;
        }

        /* Is file part of this package? */
        elem_name = lmi_get_string_property_from_objectpath(cop,
                "SoftwareElementID");
        file_name = lmi_get_string_property_from_objectpath(cop, "Name");
        if (!is_file_part_of_elem_name(file_name, elem_name)) {
            goto done;
        }

        create_instance_id(LMI_SoftwareIdentity_ClassName, elem_name,
                instance_id, BUFLEN);

        LMI_SoftwareIdentityRef si;
        LMI_SoftwareIdentityRef_Init(&si, _cb, KNameSpace(cop));
        LMI_SoftwareIdentityRef_Set_InstanceID(&si, instance_id);

        if (names) {
            KReturnObjectPath(cr, si);
        } else {
            CMPIObjectPath *o = LMI_SoftwareIdentityRef_ToObjectPath(&si, &st);
            CMPIInstance *ci = _cb->bft->getInstance(_cb, cc, o, properties,&st);
            CMReturnInstance(cr, ci);
        }
    }

done:
    clean_sw_package(&sw_pkg);

    if (files) {
        g_strfreev(files);
    }
    if (pk_pkg) {
        g_object_unref(pk_pkg);
    }

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

#endif
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareIdentityChecksAssociators(
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

static CMPIStatus LMI_SoftwareIdentityChecksAssociatorNames(
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
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const short names)
{
#ifdef RPM_FOUND
    CMPIStatus st;
    SwPackage sw_pkg;
    PkPackage *pk_pkg = NULL;
    GStrv files = NULL;
    char error_msg[BUFLEN] = "", instance_id[BUFLEN] = "";

    init_sw_package(&sw_pkg);

    st = lmi_class_path_is_a(_cb, KNameSpace(cop),
            LMI_SoftwareIdentityChecks_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);

    if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentity_ClassName, &st)) {
        /* got SoftwareIdentity - Element */
        unsigned i = 0;
        char sw_pkg_elem_name[BUFLEN] = "", sw_pkg_ver_str[BUFLEN] = "";

        if (role && strcmp(role, LMI_ELEMENT) != 0) {
            goto done;
        }

        create_instance_id(LMI_SoftwareIdentityFileCheck_ClassName, NULL,
                instance_id, BUFLEN);

        if (get_sw_pkg_from_sw_identity_op(cop, &sw_pkg) != 0) {
            snprintf(error_msg, BUFLEN, "Failed to create software identity.");
            goto done;
        }
        sw_pkg_get_element_name(&sw_pkg, sw_pkg_elem_name, BUFLEN);
        sw_pkg_get_version_str(&sw_pkg, sw_pkg_ver_str, BUFLEN);

        get_pk_pkg_from_sw_pkg(&sw_pkg,
                pk_bitfield_value(PK_FILTER_ENUM_INSTALLED), &pk_pkg);
        if (!pk_pkg) {
            goto done;
        }

        get_files_of_pk_pkg(pk_pkg, &files, error_msg, BUFLEN);
        if (!files) {
            goto done;
        }

        LMI_SoftwareIdentityRef si;
        LMI_SoftwareIdentityRef_InitFromObjectPath(&si, _cb, cop);

        while (files[i]) {
            LMI_SoftwareIdentityFileCheckRef sifc;
            LMI_SoftwareIdentityFileCheckRef_Init(&sifc, _cb, KNameSpace(cop));
            LMI_SoftwareIdentityFileCheckRef_Set_CheckID(&sifc, instance_id);
            LMI_SoftwareIdentityFileCheckRef_Set_SoftwareElementState(&sifc,
                    LMI_SoftwareIdentityFileCheckRef_SoftwareElementState_Executable);
            LMI_SoftwareIdentityFileCheckRef_Set_TargetOperatingSystem(&sifc,
                    LMI_SoftwareIdentityFileCheckRef_TargetOperatingSystem_LINUX);
            LMI_SoftwareIdentityFileCheckRef_Set_SoftwareElementID(&sifc,
                    sw_pkg_elem_name);
            LMI_SoftwareIdentityFileCheckRef_Set_Version(&sifc, sw_pkg_ver_str);
            LMI_SoftwareIdentityFileCheckRef_Set_Name(&sifc, files[i++]);

            LMI_SoftwareIdentityChecks w;
            LMI_SoftwareIdentityChecks_Init(&w, _cb, KNameSpace(cop));
            LMI_SoftwareIdentityChecks_Set_Element(&w, &si);
            LMI_SoftwareIdentityChecks_Set_Check(&w, &sifc);

            if (names) {
                KReturnObjectPath(cr, w);
            } else {
                KReturnInstance(cr, w);
            }
        }
    } else if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentityFileCheck_ClassName,
            &st)) {
        /* got SoftwareIdentityFileCheck - Check */
        const char *elem_name, *file_name;

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

        create_instance_id(LMI_SoftwareIdentity_ClassName, elem_name,
                instance_id, BUFLEN);

        LMI_SoftwareIdentityFileCheckRef sifc;
        LMI_SoftwareIdentityFileCheckRef_InitFromObjectPath(&sifc, _cb, cop);

        LMI_SoftwareIdentityRef si;
        LMI_SoftwareIdentityRef_Init(&si, _cb, KNameSpace(cop));
        LMI_SoftwareIdentityRef_Set_InstanceID(&si, instance_id);

        LMI_SoftwareIdentityChecks w;
        LMI_SoftwareIdentityChecks_Init(&w, _cb, KNameSpace(cop));
        LMI_SoftwareIdentityChecks_Set_Element(&w, &si);
        LMI_SoftwareIdentityChecks_Set_Check(&w, &sifc);

        if (names) {
            KReturnObjectPath(cr, w);
        } else {
            KReturnInstance(cr, w);
        }
    }

done:
    clean_sw_package(&sw_pkg);

    if (files) {
        g_strfreev(files);
    }
    if (pk_pkg) {
        g_object_unref(pk_pkg);
    }

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

#endif
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareIdentityChecksReferences(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    return references(cr, cop, assocClass, role, 0);
}

static CMPIStatus LMI_SoftwareIdentityChecksReferenceNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role)
{
    return references(cr, cop, assocClass, role, 1);
}

CMInstanceMIStub(
    LMI_SoftwareIdentityChecks,
    LMI_SoftwareIdentityChecks,
    _cb,
    LMI_SoftwareIdentityChecksInitialize(ctx))

CMAssociationMIStub(
    LMI_SoftwareIdentityChecks,
    LMI_SoftwareIdentityChecks,
    _cb,
    LMI_SoftwareIdentityChecksInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SoftwareIdentityChecks",
    "LMI_SoftwareIdentityChecks",
    "instance association")
