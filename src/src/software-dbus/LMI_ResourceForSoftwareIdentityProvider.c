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
#include "LMI_ResourceForSoftwareIdentity.h"
#include "sw-utils.h"

static const CMPIBroker* _cb;

static void LMI_ResourceForSoftwareIdentityInitialize(const CMPIContext *ctx)
{
    software_init(LMI_ResourceForSoftwareIdentity_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_ResourceForSoftwareIdentityCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_ResourceForSoftwareIdentity_ClassName);
}

static CMPIStatus k_return_rfsi(const gchar *pkg_id,
        const LMI_SoftwareIdentityResourceRef *sir, const CMPIResult *cr,
        const char *ns, const short names)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    SwPackage sw_pkg;
    char elem_name[BUFLEN] = "", instance_id[BUFLEN] = "";

    init_sw_package(&sw_pkg);

    if (create_sw_package_from_pk_pkg_id(pkg_id, &sw_pkg) != 0) {
        KSetStatus(&status, ERR_FAILED);
        goto done;
    }

    sw_pkg_get_element_name(&sw_pkg, elem_name, BUFLEN);
    clean_sw_package(&sw_pkg);

    create_instance_id(LMI_SoftwareIdentity_ClassName, elem_name, instance_id,
            BUFLEN);

    LMI_SoftwareIdentityRef si;
    LMI_SoftwareIdentityRef_Init(&si, _cb, ns);
    LMI_SoftwareIdentityRef_Set_InstanceID(&si, instance_id);

    LMI_ResourceForSoftwareIdentity rfsi;
    LMI_ResourceForSoftwareIdentity_Init(&rfsi, _cb, ns);
    LMI_ResourceForSoftwareIdentity_Set_AvailableSAP(&rfsi, sir);
    LMI_ResourceForSoftwareIdentity_Set_ManagedElement(&rfsi, &si);

    if (names) {
        KReturnObjectPath(cr, rfsi);
    } else {
        KReturnInstance(cr, rfsi);
    }

done:
    return status;
}

/**
 * Additional arguments container for `traverse_avail_pkg_repos()`.
 */
struct TraverseAvailPkgReposData {
    /* repo-id -> object path of LMI_SoftwareIdentityResource */
    GHashTable *repo_hash;
    /* result of EnumInstance(Name)s method */
    const CMPIResult *cr;
    const gchar *namespace;
    /* whether to yield instance names or full instances */
    short names;
    /* points to allocated status which will be modified on any error */
    CMPIStatus *status;
};

/**
 * Traversing function for a hash/tree with `PkPackages *` being keys and
 * NULL-terminated array of `PkRepoDetail *` being their associated values.
 *
 * It yields instance or instance name of `LMI_ResourceForSoftwareIdentity` for
 * given pair key/value.
 *
 * @param data Pointer to `TraverseAvailPkgReposData`.
 */
static gboolean traverse_avail_pkg_repos(gpointer key,
                                         gpointer value,
                                         gpointer data)
{
    struct TraverseAvailPkgReposData *taprd = data;
    GHashTable *repo_hash = taprd->repo_hash;
    PkPackage *pk_pkg = PK_PACKAGE(key);
    GPtrArray *array = value;
    PkRepoDetail *pk_repo_det;
    gchar *repo_id = NULL;
    LMI_SoftwareIdentityResourceRef *sir;
    char buf[BUFLEN];

    for (guint i = 0; i < array->len; ++i) {
        pk_repo_det = PK_REPO_DETAIL(g_ptr_array_index(array, i));
        g_object_get(pk_repo_det, "repo-id", &repo_id, NULL);
        sir = g_hash_table_lookup(repo_hash, repo_id);
        if (sir != NULL) {
            make_package_id_with_custom_data(pk_pkg, repo_id, buf, BUFLEN);
            *taprd->status = k_return_rfsi(buf, sir, taprd->cr,
                    taprd->namespace, taprd->names);
        } else {
            lmi_error("Got unexpected repo-id \"%s\" associated to package \"%s\"!",
                    repo_id, pk_package_get_id(pk_pkg));
        }
        g_clear_pointer(&repo_id, g_free);
    }
    return taprd->status->rc != CMPI_RC_OK;
}

/**
 * Enumerate instances and instance names of `LMI_ResourceForSoftwareIdentity`.
 *
 * @param names Whether to yield just instance names instead of full instances.
 * @param repo_id_p Yield instances refering just this particular repository.
 */
static CMPIStatus enum_instances(const CMPIContext *ctx, const CMPIResult *cr,
        const char *ns, const short names, const char *repo_id_p)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    LMI_SoftwareIdentityResourceRef *sir;
    struct TraverseAvailPkgReposData taprd = {NULL, cr, ns, names, &status};
    GPtrArray *pkg_array = NULL, *array = NULL;
    gchar *repo_id = NULL;
    GTree *avail_pkg_repos = NULL;
    unsigned i;
    char error_msg[BUFLEN] = "";

    /* Get all enabled repositories. */
    get_pk_repos(REPO_STATE_ENUM_ENABLED, &array, error_msg, BUFLEN);
    if (!array)
        goto err;

    if (repo_id_p != NULL) {
        /* Make the repo details array contain just one repo detail matching
         * given `repo_id_p`. */
        PkRepoDetail *repo_det;
        for (i = 0; i < array->len; ++i) {
            repo_det = g_ptr_array_index(array, i);
            g_object_get(repo_det, "repo-id", &repo_id, NULL);
            if (g_strcmp0(repo_id, repo_id_p) == 0) {
                g_object_ref(repo_det);
                g_ptr_array_unref(array);
                array = g_ptr_array_new_full(1, g_object_unref);
                if (array == NULL)
                    goto mem_err;
                g_ptr_array_add(array, repo_det);
                break;
            }
        }
        if (i >= array->len) {  /* repo_id_p haven't been found */
            g_snprintf(error_msg, BUFLEN, "Repository \"%s\" not found among enabled.",
                    repo_id_p);
            goto err;
        }
    }

    /* Create LMI_SoftwareIdentityResourceRef for all repo details and store
     * them in a hash table. */
    taprd.repo_hash = g_hash_table_new_full(
            g_str_hash, g_str_equal, g_free, g_free);

    for (i = 0; i < array->len; i++) {
        g_object_get(g_ptr_array_index(array, i), "repo-id", &repo_id, NULL);

        sir = g_new0(LMI_SoftwareIdentityResourceRef, 1);
        LMI_SoftwareIdentityResourceRef_Init(sir, _cb, ns);
        LMI_SoftwareIdentityResourceRef_Set_SystemName(sir, lmi_get_system_name_safe(ctx));
        LMI_SoftwareIdentityResourceRef_Set_CreationClassName(sir,
                LMI_SoftwareIdentityResource_ClassName);
        LMI_SoftwareIdentityResourceRef_Set_SystemCreationClassName(sir,
                lmi_get_system_creation_class_name());
        LMI_SoftwareIdentityResourceRef_Set_Name(sir, repo_id);

        g_hash_table_insert(taprd.repo_hash, repo_id, sir);
    }

    /* Get all packages (available and installed). */
    get_pk_packages(0, &pkg_array, error_msg, BUFLEN);
    if (!pkg_array)
        goto err;

    /* Associate packages to repositories providing them.
     * `avail_pkg_repos` will contain just available packages. */
    get_repo_dets_for_pk_pkgs(pkg_array, array, &avail_pkg_repos,
            error_msg, BUFLEN);
    if (!avail_pkg_repos)
        goto err;

    /* Generated instances/instance names for obtained associations. */
    g_tree_foreach(avail_pkg_repos, traverse_avail_pkg_repos, &taprd);

    g_tree_unref(avail_pkg_repos);
    g_clear_pointer(&taprd.repo_hash, g_hash_table_unref);
    g_clear_pointer(&array, g_ptr_array_unref);
    return status;

mem_err:
    lmi_error("Memory allocation failed");
err:
    g_clear_pointer(&avail_pkg_repos, g_tree_unref);
    g_clear_pointer(&taprd.repo_hash, g_hash_table_unref);
    g_clear_pointer(&array, g_ptr_array_unref);
    if (!status.rc) {
        if (error_msg[0]) {
            KSetStatus2(_cb, &status, ERR_FAILED, error_msg);
        } else {
            KSetStatus(&status, ERR_FAILED);
        }
    }
    lmi_error("Failed to enumerate instance%s of %s: %s",
            names? " names" : "s",
            LMI_ResourceForSoftwareIdentity_ClassName,
            error_msg[0] ? error_msg : "failed");
    return status;
}

static CMPIStatus LMI_ResourceForSoftwareIdentityEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return enum_instances(cc, cr, KNameSpace(cop), 1, NULL);
}

static CMPIStatus LMI_ResourceForSoftwareIdentityEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return enum_instances(cc, cr, KNameSpace(cop), 0, NULL);
}

static CMPIStatus LMI_ResourceForSoftwareIdentityGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    const char *repo_id = NULL;
    gchar *pkg_id = NULL, **values = NULL;
    GPtrArray *array = NULL;
    PkRepoDetail *pk_repo = NULL;
    PkPackage *pk_pkg = NULL;
    SwPackage sw_pkg;
    CMPIrc rc_stat = CMPI_RC_ERR_NOT_FOUND;
    char error_msg[BUFLEN] = "";

    init_sw_package(&sw_pkg);

    LMI_ResourceForSoftwareIdentity w;
    LMI_ResourceForSoftwareIdentity_InitFromObjectPath(&w, _cb, cop);

    /* Check repo */
    if (strcmp(lmi_get_string_property_from_objectpath(w.AvailableSAP.value,
            "CreationClassName"), LMI_SoftwareIdentityResource_ClassName) != 0) {
        goto done;
    }
    if (strcmp(lmi_get_string_property_from_objectpath(w.AvailableSAP.value,
            "SystemCreationClassName"), lmi_get_system_creation_class_name()) != 0) {
        goto done;
    }
    if (strcmp(lmi_get_string_property_from_objectpath(w.AvailableSAP.value,
            "SystemName"), lmi_get_system_name_safe(cc)) != 0) {
        goto done;
    }
    repo_id = lmi_get_string_property_from_objectpath(w.AvailableSAP.value, "Name");
    get_pk_repo(repo_id, &pk_repo, error_msg, BUFLEN);
    if (!pk_repo) {
        goto done;
    }

    /* Check package */
    if (get_sw_pkg_from_sw_identity_op(w.ManagedElement.value, &sw_pkg) != 0) {
        goto done;
    }
    get_pk_pkg_from_sw_pkg(&sw_pkg, 0, &pk_pkg);
    if (!pk_pkg) {
        goto done;
    }

    if (!pk_pkg_belongs_to_repo(pk_pkg, repo_id)) {
        /* For installed packages and identical packages provided by multiple
         * repositories, check whether the package is available
         * from given repository. */
        pkg_id = pk_package_id_build(pk_package_get_name(pk_pkg),
                pk_package_get_version(pk_pkg), pk_package_get_arch(pk_pkg),
                repo_id);
        values = g_new0(gchar*, 2);
        values[0] = pkg_id;
        get_pk_det_from_array(values, &array, NULL);
        if (!array || array->len < 1)
            goto done;
    }

    KReturnInstance(cr, w);

    rc_stat = CMPI_RC_OK;

done:
    clean_sw_package(&sw_pkg);

    if (values) {
        g_strfreev(values);
    }
    if (array) {
        g_ptr_array_unref(array);
    }
    if (pk_repo) {
        g_object_unref(pk_repo);
    }
    if (pk_pkg) {
        g_object_unref(pk_pkg);
    }

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(rc_stat);
}

static CMPIStatus LMI_ResourceForSoftwareIdentityCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ResourceForSoftwareIdentityModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ResourceForSoftwareIdentityDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ResourceForSoftwareIdentityExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ResourceForSoftwareIdentityAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static void enum_sw_instance_names_for_repo(const CMPIContext *ctx,
        const char *repo_id_p, const CMPIResult *cr, const char *ns,
        char *error_msg, const unsigned error_msg_len)
{
    PkPackage *pkg = NULL;
    GPtrArray *pkg_array = NULL, *det_array = NULL, *repo_array = NULL,
            *array = NULL;
    gchar *repo_id = NULL, *pkg_id = NULL, **values = NULL;
    short found;
    unsigned i, j;

    /* Get all repos and packages. */
    get_pk_repos(REPO_STATE_ENUM_ANY, &repo_array, error_msg, error_msg_len);
    if (!repo_array) {
        goto done;
    }

    get_pk_packages(0, &pkg_array, error_msg, error_msg_len);
    if (!pkg_array) {
        goto done;
    }

    /* Lets assume that there is about EST_OF_INSTD_PKGS installed packages
     * (to avoid too many allocations) */
    array = g_ptr_array_new_full(EST_OF_INSTD_PKGS, g_object_unref);

    /* For every package, check repo. If found, and it is required repo, return
     * SwIdentityRef instance. If repo for pkg is not found, save the pkg
     * for next processing. */
    for (i = 0; i < pkg_array->len; i++) {
        pkg = g_ptr_array_index(pkg_array, i);
        found = 0;
        for (j = 0; j < repo_array->len; j++) {
            g_object_get(g_ptr_array_index(repo_array, j), "repo-id", &repo_id,
                    NULL);
            if (pk_pkg_belongs_to_repo(pkg, repo_id)) {
                found = 1;
                if (g_strcmp0(repo_id, repo_id_p) == 0) {
                    k_return_sw_identity_op_from_pkg_id(pk_package_get_id(pkg),
                            _cb, ctx, ns, cr);
                }
                g_free(repo_id);
                repo_id = NULL;
                break;
            }
            g_free(repo_id);
            repo_id = NULL;
        }
        if (!found) {
            g_ptr_array_add(array, g_object_ref(pkg));
        }
    }
    g_ptr_array_unref(pkg_array);
    pkg_array = NULL;
    g_ptr_array_unref(repo_array);
    repo_array = NULL;

    /* For all saved packages, create package ID with required repo.
     * Then, get package details for all those package IDs. Package details
     * are returned only for valid package IDs... */
    j = 0;
    values = g_new0(gchar*, array->len + 1);
    for (i = 0; i < array->len; i++) {
        pkg = g_ptr_array_index(array, i);
        pkg_id = pk_package_id_build(pk_package_get_name(pkg),
                pk_package_get_version(pkg), pk_package_get_arch(pkg),
                repo_id_p);
        values[j++] = pkg_id;
    }
    g_ptr_array_unref(array);
    array = NULL;

    get_pk_det_from_array(values, &det_array, NULL);
    g_strfreev(values);
    values = NULL;

    if (det_array && det_array->len > 0) {
        for (i = 0; i < det_array->len; i++) {
            g_object_get(g_ptr_array_index(det_array, i), "package-id", &pkg_id, NULL);
            k_return_sw_identity_op_from_pkg_id(pkg_id, _cb, ctx, ns, cr);
            g_free(pkg_id);
        }
        g_ptr_array_unref(det_array);
        det_array = NULL;
    }

done:
    return;
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

    st = lmi_class_path_is_a(_cb, KNameSpace(cop),
            LMI_ResourceForSoftwareIdentity_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);

    if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentityResource_ClassName, &st)) {
        /* got SoftwareIdentityResource - AvailableSAP;
         * where listing only associators names is supported */
        if (!names) {
            CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
        }

        st = lmi_class_path_is_a(_cb, KNameSpace(cop),
                LMI_SoftwareIdentity_ClassName, resultClass);
        lmi_return_if_class_check_not_ok(st);

        if (role && strcmp(role, LMI_AVAILABLE_SAP) != 0) {
            goto done;
        }
        if (resultRole && strcmp(resultRole, LMI_MANAGED_ELEMENT) != 0) {
            goto done;
        }

        enum_sw_instance_names_for_repo(cc,
                lmi_get_string_property_from_objectpath(cop, "Name"), cr,
                KNameSpace(cop), error_msg, BUFLEN);
    } else if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentity_ClassName, &st)) {
        /* got SoftwareIdentity - ManagedElement */
        SwPackage sw_pkg;
        gchar *repo_id = NULL;

        st = lmi_class_path_is_a(_cb, KNameSpace(cop),
                LMI_SoftwareIdentityResource_ClassName, resultClass);
        lmi_return_if_class_check_not_ok(st);

        if (role && strcmp(role, LMI_MANAGED_ELEMENT) != 0) {
            goto done;
        }
        if (resultRole && strcmp(resultRole, LMI_AVAILABLE_SAP) != 0) {
            goto done;
        }

        init_sw_package(&sw_pkg);
        get_sw_pkg_from_sw_identity_op(cop, &sw_pkg);
        get_repo_id_from_sw_pkg(&sw_pkg, &repo_id, error_msg, BUFLEN);
        clean_sw_package(&sw_pkg);

        if (*error_msg) {
            goto done;
        }
        if (!repo_id) {
            /* We don't know the repository - package can be installed from
             * local source, etc.. I don't think this is an error. */
            goto done;
        }

        LMI_SoftwareIdentityResourceRef sir;
        LMI_SoftwareIdentityResourceRef_Init(&sir, _cb, KNameSpace(cop));
        LMI_SoftwareIdentityResourceRef_Set_SystemName(&sir, lmi_get_system_name_safe(cc));
        LMI_SoftwareIdentityResourceRef_Set_CreationClassName(&sir,
                LMI_SoftwareIdentityResource_ClassName);
        LMI_SoftwareIdentityResourceRef_Set_SystemCreationClassName(&sir,
                lmi_get_system_creation_class_name());
        LMI_SoftwareIdentityResourceRef_Set_Name(&sir, repo_id);

        g_free(repo_id);
        repo_id = NULL;

        if (names) {
            KReturnObjectPath(cr, sir);
        } else {
            CMPIObjectPath *o = LMI_SoftwareIdentityResourceRef_ToObjectPath(&sir, &st);
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

static CMPIStatus LMI_ResourceForSoftwareIdentityAssociators(
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

static CMPIStatus LMI_ResourceForSoftwareIdentityAssociatorNames(
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
    const CMPIContext* ctx,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const short names)
{
    CMPIStatus st;
    char error_msg[BUFLEN] = "";

    st = lmi_class_path_is_a(_cb, KNameSpace(cop),
            LMI_ResourceForSoftwareIdentity_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);

    if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentityResource_ClassName, &st)) {
        /* got SoftwareIdentityResource - AvailableSAP */
        if (role && strcmp(role, LMI_AVAILABLE_SAP) != 0) {
            goto done;
        }

        return enum_instances(ctx, cr, KNameSpace(cop), names,
                lmi_get_string_property_from_objectpath(cop, "Name"));
    } else if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentity_ClassName, &st)) {
        /* got SoftwareIdentity - ManagedElement */
        SwPackage sw_pkg;
        gchar *repo_id = NULL;

        if (role && strcmp(role, LMI_MANAGED_ELEMENT) != 0) {
            goto done;
        }

        init_sw_package(&sw_pkg);
        get_sw_pkg_from_sw_identity_op(cop, &sw_pkg);
        get_repo_id_from_sw_pkg(&sw_pkg, &repo_id, error_msg, BUFLEN);
        clean_sw_package(&sw_pkg);

        if (*error_msg) {
            goto done;
        }
        if (!repo_id) {
            /* We don't know the repository - package can be installed from
             * local source, etc.. I don't think this is an error. */
            goto done;
        }

        LMI_SoftwareIdentityResourceRef sir;
        LMI_SoftwareIdentityResourceRef_Init(&sir, _cb, KNameSpace(cop));
        LMI_SoftwareIdentityResourceRef_Set_SystemName(&sir,
                lmi_get_system_name_safe(ctx));
        LMI_SoftwareIdentityResourceRef_Set_CreationClassName(&sir,
                LMI_SoftwareIdentityResource_ClassName);
        LMI_SoftwareIdentityResourceRef_Set_SystemCreationClassName(&sir,
                lmi_get_system_creation_class_name());
        LMI_SoftwareIdentityResourceRef_Set_Name(&sir, repo_id);

        g_free(repo_id);
        repo_id = NULL;

        LMI_SoftwareIdentityRef si;
        LMI_SoftwareIdentityRef_InitFromObjectPath(&si, _cb, cop);

        LMI_ResourceForSoftwareIdentity rfsi;
        LMI_ResourceForSoftwareIdentity_Init(&rfsi, _cb, KNameSpace(cop));
        LMI_ResourceForSoftwareIdentity_Set_AvailableSAP(&rfsi, &sir);
        LMI_ResourceForSoftwareIdentity_Set_ManagedElement(&rfsi, &si);

        if (names) {
            KReturnObjectPath(cr, rfsi);
        } else {
            KReturnInstance(cr, rfsi);
        }
    }

done:
    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ResourceForSoftwareIdentityReferences(
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

static CMPIStatus LMI_ResourceForSoftwareIdentityReferenceNames(
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
    LMI_ResourceForSoftwareIdentity,
    LMI_ResourceForSoftwareIdentity,
    _cb,
    LMI_ResourceForSoftwareIdentityInitialize(ctx))

CMAssociationMIStub(
    LMI_ResourceForSoftwareIdentity,
    LMI_ResourceForSoftwareIdentity,
    _cb,
    LMI_ResourceForSoftwareIdentityInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ResourceForSoftwareIdentity",
    "LMI_ResourceForSoftwareIdentity",
    "instance association")
