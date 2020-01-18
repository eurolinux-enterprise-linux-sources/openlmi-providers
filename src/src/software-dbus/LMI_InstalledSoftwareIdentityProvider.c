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
#include "LMI_InstalledSoftwareIdentity.h"
#include "sw-utils.h"

static const CMPIBroker* _cb;

static void LMI_InstalledSoftwareIdentityInitialize(const CMPIContext *ctx)
{
    software_init(LMI_InstalledSoftwareIdentity_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_InstalledSoftwareIdentityCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_InstalledSoftwareIdentity_ClassName);
}

static CMPIStatus enum_instances(const CMPIContext *ctx, const CMPIResult *cr,
        const char *ns, const short names)
{
    GPtrArray *array = NULL;
    SwPackage sw_pkg;
    unsigned i;
    char error_msg[BUFLEN] = "", elem_name[BUFLEN] = "",
            instance_id[BUFLEN] = "";

    init_sw_package(&sw_pkg);

    get_pk_packages(pk_bitfield_value(PK_FILTER_ENUM_INSTALLED), &array,
            error_msg, BUFLEN);
    if (!array) {
        goto done;
    }

    for (i = 0; i < array->len; i++) {
        if (create_sw_package_from_pk_pkg(g_ptr_array_index(array, i),
                &sw_pkg) != 0) {
            continue;
        }

        sw_pkg_get_element_name(&sw_pkg, elem_name, BUFLEN);

        create_instance_id(LMI_SoftwareIdentity_ClassName, elem_name, instance_id,
                BUFLEN);

        clean_sw_package(&sw_pkg);

        LMI_SoftwareIdentityRef si;
        LMI_SoftwareIdentityRef_Init(&si, _cb, ns);
        LMI_SoftwareIdentityRef_Set_InstanceID(&si, instance_id);

        LMI_InstalledSoftwareIdentity w;
        LMI_InstalledSoftwareIdentity_Init(&w, _cb, ns);
        LMI_InstalledSoftwareIdentity_SetObjectPath_System(&w,
                lmi_get_computer_system_safe(ctx));
        LMI_InstalledSoftwareIdentity_Set_InstalledSoftware(&w, &si);

        if (names) {
            KReturnObjectPath(cr, w);
        } else {
            KReturnInstance(cr, w);
        }
    }

done:
    if (array) {
        g_ptr_array_unref(array);
    }

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_InstalledSoftwareIdentityEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return enum_instances(cc, cr, KNameSpace(cop), 1);
}

static CMPIStatus LMI_InstalledSoftwareIdentityEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return enum_instances(cc, cr, KNameSpace(cop), 0);
}

static CMPIStatus LMI_InstalledSoftwareIdentityGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_InstalledSoftwareIdentity w;
    LMI_InstalledSoftwareIdentity_InitFromObjectPath(&w, _cb, cop);

    if (!is_elem_name_installed(get_elem_name_from_instance_id(
            lmi_get_string_property_from_objectpath(w.InstalledSoftware.value,
            "InstanceID")))) {
        CMReturn(CMPI_RC_ERR_NOT_FOUND);
    }

    KReturnInstance(cr, w);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_InstalledSoftwareIdentityCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMPIStatus st;
    CMPIData cdata;
    SwPackage sw_pkg;
    PkTask *task = NULL;
    PkPackage *pk_pkg = NULL;
    PkResults *results = NULL;
    gchar **values = NULL;
    GError *gerror = NULL;
    char error_msg[BUFLEN] = "";

    init_sw_package(&sw_pkg);

    if (!ci || !ci->ft) {
        snprintf(error_msg, BUFLEN, "Missing instance parameter");
        goto done;
    }

    cdata = CMGetProperty(ci, "InstalledSoftware", NULL);
    if (create_sw_package_from_elem_name(get_elem_name_from_instance_id(
            lmi_get_string_property_from_objectpath(cdata.value.ref,
            "InstanceID")), &sw_pkg) != 0) {
        goto done;
    }

    if (get_pk_pkg_from_sw_pkg(&sw_pkg,
            pk_bitfield_value(PK_FILTER_ENUM_INSTALLED), NULL)) {
        clean_sw_package(&sw_pkg);
        CMReturn(CMPI_RC_ERR_ALREADY_EXISTS);
    }

    if (!get_pk_pkg_from_sw_pkg(&sw_pkg, 0, &pk_pkg)) {
        clean_sw_package(&sw_pkg);
        CMReturn(CMPI_RC_ERR_NOT_FOUND);
    }

    task = pk_task_new();
    g_object_set(task, "interactive", FALSE, NULL);

    values = g_new0(gchar*, 2);
    values[0] = g_strdup(pk_package_get_id(pk_pkg));

    results = pk_task_install_packages_sync(task, values, NULL, NULL, NULL,
            &gerror);
    if (check_and_create_error_msg(results, gerror, "Installing package failed",
            error_msg, BUFLEN)) {
        goto done;
    }

    CMReturnObjectPath(cr, CMGetObjectPath(ci, &st));

done:
    clean_sw_package(&sw_pkg);
    g_clear_pointer(&values, g_strfreev);
    g_clear_error(&gerror);
    g_clear_object(&results);
    g_clear_object(&task);

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_InstalledSoftwareIdentityModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_InstalledSoftwareIdentityDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    SwPackage sw_pkg;
    PkTask *task = NULL;
    PkPackage *pk_pkg = NULL;
    PkResults *results = NULL;
    gchar **values = NULL;
    GError *gerror = NULL;
    char error_msg[BUFLEN] = "";

    init_sw_package(&sw_pkg);

    LMI_InstalledSoftwareIdentity w;
    LMI_InstalledSoftwareIdentity_InitFromObjectPath(&w, _cb, cop);

    if (create_sw_package_from_elem_name(get_elem_name_from_instance_id(
            lmi_get_string_property_from_objectpath(w.InstalledSoftware.value,
            "InstanceID")), &sw_pkg) != 0) {
        goto done;
    }

    if (!get_pk_pkg_from_sw_pkg(&sw_pkg,
            pk_bitfield_value(PK_FILTER_ENUM_INSTALLED), &pk_pkg)) {
        clean_sw_package(&sw_pkg);
        CMReturn(CMPI_RC_ERR_NOT_FOUND);
    }

    task = pk_task_new();

    values = g_new0(gchar*, 2);
    values[0] = g_strdup(pk_package_get_id(pk_pkg));

    results = pk_task_remove_packages_sync(task, values, TRUE, FALSE,
            NULL, NULL, NULL, &gerror);
    if (check_and_create_error_msg(results, gerror, "Removing package failed",
            error_msg, BUFLEN)) {
        goto done;
    }

done:
    clean_sw_package(&sw_pkg);
    g_clear_pointer(&values, g_strfreev);
    g_clear_error(&gerror);
    g_clear_object(&results);
    g_clear_object(&task);

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_InstalledSoftwareIdentityExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_InstalledSoftwareIdentityAssociationCleanup(
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
    CMPIStatus st;
    char error_msg[BUFLEN] = "";
    const char *computer_system_name;

    computer_system_name = lmi_get_string_property_from_objectpath(lmi_get_computer_system_safe(cc),
            "CreationClassName");

    st = lmi_class_path_is_a(_cb, KNameSpace(cop),
            LMI_InstalledSoftwareIdentity_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);

    if (CMClassPathIsA(_cb, cop, computer_system_name, &st)) {
        /* got PG_ComputerSystem - System */
        st = lmi_class_path_is_a(_cb, KNameSpace(cop),
                LMI_SoftwareIdentity_ClassName, resultClass);
        lmi_return_if_class_check_not_ok(st);

        if (role && strcmp(role, LMI_SYSTEM) != 0) {
            goto done;
        }
        if (resultRole && strcmp(resultRole, LMI_INSTALLED_SOFTWARE) != 0) {
            goto done;
        }

        if (names) {
            enum_sw_identity_instance_names(
                    pk_bitfield_value(PK_FILTER_ENUM_INSTALLED), _cb, cc,
                    KNameSpace(cop), cr, error_msg, BUFLEN);
        } else {
            enum_sw_identity_instances(
                    pk_bitfield_value(PK_FILTER_ENUM_INSTALLED), _cb,
                    KNameSpace(cop), cr, error_msg, BUFLEN);
        }
    } else if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentity_ClassName, &st)) {
        /* got SoftwareIdentity - InstalledSoftware */
        const char *elem_name;

        st = lmi_class_path_is_a(_cb, KNameSpace(cop), computer_system_name,
                resultClass);
        lmi_return_if_class_check_not_ok(st);

        if (role && strcmp(role, LMI_INSTALLED_SOFTWARE) != 0) {
            goto done;
        }
        if (resultRole && strcmp(resultRole, LMI_SYSTEM) != 0) {
            goto done;
        }

        /* Is this SwIdentity installed? */
        elem_name = get_elem_name_from_instance_id(
                lmi_get_string_property_from_objectpath(cop, "InstanceID"));
        if (!is_elem_name_installed(elem_name)) {
            goto done;
        }

        if (names) {
            CMReturnObjectPath(cr, lmi_get_computer_system_safe(cc));
        } else {
            CMPIObjectPath *o = lmi_get_computer_system_safe(cc);
            CMPIInstance *ci = _cb->bft->getInstance(_cb, cc, o, properties, &st);
            CMReturnInstance(cr, ci);
        }
    }

done:
    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_InstalledSoftwareIdentityAssociators(
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

static CMPIStatus LMI_InstalledSoftwareIdentityAssociatorNames(
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
    const CMPIContext *ctx,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const short names)
{
    CMPIStatus st;
    const char *computer_system_name;

    computer_system_name = lmi_get_string_property_from_objectpath(lmi_get_computer_system_safe(ctx),
            "CreationClassName");

    st = lmi_class_path_is_a(_cb, KNameSpace(cop),
            LMI_InstalledSoftwareIdentity_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);

    if (CMClassPathIsA(_cb, cop, computer_system_name, &st)) {
        /* got PG_ComputerSystem - System */
        if (role && strcmp(role, LMI_SYSTEM) != 0) {
            goto done;
        }

        return enum_instances(ctx, cr, KNameSpace(cop), names);
    } else if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentity_ClassName, &st)) {
        /* got SoftwareIdentity - InstalledSoftware */
        const char *elem_name;

        if (role && strcmp(role, LMI_INSTALLED_SOFTWARE) != 0) {
            goto done;
        }

        /* Is this SwIdentity installed? */
        elem_name = get_elem_name_from_instance_id(
                lmi_get_string_property_from_objectpath(cop, "InstanceID"));
        if (!is_elem_name_installed(elem_name)) {
            goto done;
        }

        LMI_SoftwareIdentityRef si;
        LMI_SoftwareIdentityRef_InitFromObjectPath(&si, _cb, cop);

        LMI_InstalledSoftwareIdentity w;
        LMI_InstalledSoftwareIdentity_Init(&w, _cb, KNameSpace(cop));
        LMI_InstalledSoftwareIdentity_SetObjectPath_System(&w,
                lmi_get_computer_system_safe(ctx));
        LMI_InstalledSoftwareIdentity_Set_InstalledSoftware(&w, &si);

        if (names) {
            KReturnObjectPath(cr, w);
        } else {
            KReturnInstance(cr, w);
        }
    }

done:
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_InstalledSoftwareIdentityReferences(
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

static CMPIStatus LMI_InstalledSoftwareIdentityReferenceNames(
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
    LMI_InstalledSoftwareIdentity,
    LMI_InstalledSoftwareIdentity,
    _cb,
    LMI_InstalledSoftwareIdentityInitialize(ctx))

CMAssociationMIStub(
    LMI_InstalledSoftwareIdentity,
    LMI_InstalledSoftwareIdentity,
    _cb,
    LMI_InstalledSoftwareIdentityInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_InstalledSoftwareIdentity",
    "LMI_InstalledSoftwareIdentity",
    "instance association")
