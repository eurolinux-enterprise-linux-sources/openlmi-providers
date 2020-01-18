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
#include "LMI_MemberOfSoftwareCollection.h"
#include "sw-utils.h"

static const CMPIBroker* _cb;

static void LMI_MemberOfSoftwareCollectionInitialize(const CMPIContext *ctx)
{
    software_init(LMI_MemberOfSoftwareCollection_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_MemberOfSoftwareCollectionCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_MemberOfSoftwareCollection_ClassName);
}

static CMPIStatus k_return_mosc(PkPackage *pk_pkg,
        const LMI_SystemSoftwareCollectionRef *ssc, const CMPIResult *cr,
        const char *ns, const short names)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    SwPackage sw_pkg;
    char elem_name[BUFLEN] = "", instance_id[BUFLEN] = "";

    init_sw_package(&sw_pkg);

    if (create_sw_package_from_pk_pkg(pk_pkg, &sw_pkg) != 0) {
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

    LMI_MemberOfSoftwareCollection w;
    LMI_MemberOfSoftwareCollection_Init(&w, _cb, ns);
    LMI_MemberOfSoftwareCollection_Set_Collection(&w, ssc);
    LMI_MemberOfSoftwareCollection_Set_Member(&w, &si);

    if (names) {
        KReturnObjectPath(cr, w);
    } else {
        KReturnInstance(cr, w);
    }

done:
    return status;
}

/**
 * Additional arguments container for `traverse_avail_pkg_repos()`.
 */
struct TraverseAvailPkgReposData {
    const CMPIBroker *cb;
    const CMPIContext *ctx;
    const gchar *ns;
    const CMPIResult *cr;
    /* whether to yield instance names or full instances */
    short names;
    LMI_SystemSoftwareCollectionRef *ssc;
};

/**
 * Traversing function for a hash/tree with `PkPackages *` being keys and
 * NULL-terminated array of `PkRepoDetail *` being their associated values.
 *
 * It yields instance or instance name of `LMI_MemberOfSoftwareCollection` for
 * given pair key/value.
 *
 * @param data Pointer to `TraverseAvailPkgReposData`.
 */
static gboolean traverse_avail_pkg_repos(gpointer key,
                                         gpointer value,
                                         gpointer data)
{
    struct TraverseAvailPkgReposData *taprd = data;
    PkPackage *pk_pkg = PK_PACKAGE(key);
    CMPIStatus st;

    st = k_return_mosc(pk_pkg, taprd->ssc, taprd->cr, taprd->ns, taprd->names);

    return st.rc != CMPI_RC_OK;
}

/**
 * Traversing function for a hash/tree with `PkPackages *` being keys and
 * NULL-terminated array of `PkRepoDetail *` being their associated values.
 *
 * It yields instance name of `LMI_SoftwareIdentity` for
 * given pair key/value.
 *
 * @param data Pointer to `TraverseAvailPkgReposData`.
 */
static gboolean traverse_avail_pkg_repos_for_sw(gpointer key,
                                                gpointer value,
                                                gpointer data)
{
    struct TraverseAvailPkgReposData *taprd = data;
    PkPackage *pk_pkg = PK_PACKAGE(key);
    CMPIStatus st;

    st = k_return_sw_identity_op_from_pkg_id(pk_package_get_id(pk_pkg),
            taprd->cb, taprd->ctx, taprd->ns, taprd->cr);

    return st.rc != CMPI_RC_OK;
}

static CMPIStatus enum_instances(const CMPIResult *cr, const char *ns,
        const short names)
{
    struct TraverseAvailPkgReposData taprd = {NULL, NULL, ns, cr, names, NULL};
    GTree *avail_pkg_repos = NULL;
    GPtrArray *array = NULL;
    char error_msg[BUFLEN] = "", instance_id[BUFLEN] = "";

    create_instance_id(LMI_SystemSoftwareCollection_ClassName, NULL,
            instance_id, BUFLEN);

    LMI_SystemSoftwareCollectionRef *ssc = g_new0(LMI_SystemSoftwareCollectionRef, 1);
    LMI_SystemSoftwareCollectionRef_Init(ssc, _cb, ns);
    LMI_SystemSoftwareCollectionRef_Set_InstanceID(ssc, instance_id);
    taprd.ssc = ssc;

    get_pk_packages(0, &array, error_msg, BUFLEN);
    if (!array) {
        goto done;
    }

    /* Associate packages to repositories providing them.
     * `avail_pkg_repos` will contain just available packages. */
    get_repo_dets_for_pk_pkgs(array, NULL, &avail_pkg_repos, error_msg, BUFLEN);
    if (!avail_pkg_repos) {
        goto done;
    }

    /* Generate instances/instance names for obtained associations. */
    g_tree_foreach(avail_pkg_repos, traverse_avail_pkg_repos, &taprd);

done:
    if (avail_pkg_repos) {
        g_tree_unref(avail_pkg_repos);
    }
    if (array) {
        g_ptr_array_unref(array);
    }
    g_free(ssc);

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemberOfSoftwareCollectionEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return enum_instances(cr, KNameSpace(cop), 1);
}

static CMPIStatus LMI_MemberOfSoftwareCollectionEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return enum_instances(cr, KNameSpace(cop), 0);
}

static CMPIStatus LMI_MemberOfSoftwareCollectionGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    SwPackage sw_pkg;
    gchar *repo_id = NULL;
    char error_msg[BUFLEN] = "", instance_id[BUFLEN] = "";

    init_sw_package(&sw_pkg);

    create_instance_id(LMI_SystemSoftwareCollection_ClassName, NULL,
            instance_id, BUFLEN);

    LMI_MemberOfSoftwareCollection w;
    LMI_MemberOfSoftwareCollection_InitFromObjectPath(&w, _cb, cop);

    if (strcmp(lmi_get_string_property_from_objectpath(w.Collection.value,
            "InstanceID"), instance_id) != 0) {
        KSetStatus(&status, ERR_NOT_FOUND);
        goto done;
    }

    if (get_sw_pkg_from_sw_identity_op(w.Member.value, &sw_pkg) != 0) {
        KSetStatus(&status, ERR_NOT_FOUND);
        goto done;
    }

    get_repo_id_from_sw_pkg(&sw_pkg, &repo_id, error_msg, BUFLEN);
    if (!repo_id) {
        KSetStatus(&status, ERR_NOT_FOUND);
        goto done;
    }

    KReturnInstance(cr, w);

done:
    clean_sw_package(&sw_pkg);
    g_free(repo_id);

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    return status;
}

static CMPIStatus LMI_MemberOfSoftwareCollectionCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemberOfSoftwareCollectionModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemberOfSoftwareCollectionDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemberOfSoftwareCollectionExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemberOfSoftwareCollectionAssociationCleanup(
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
    SwPackage sw_pkg;
    gchar *repo_id = NULL;
    GPtrArray *array = NULL;
    GTree *avail_pkg_repos = NULL;
    char error_msg[BUFLEN] = "";

    st = lmi_class_path_is_a(_cb, KNameSpace(cop),
            LMI_MemberOfSoftwareCollection_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);

    init_sw_package(&sw_pkg);

    if (CMClassPathIsA(_cb, cop, LMI_SystemSoftwareCollection_ClassName, &st)) {
        /* got SystemSoftwareCollection - Collection;
         * where listing only associators names is supported */
        struct TraverseAvailPkgReposData taprd = {_cb, cc, KNameSpace(cop), cr,
                names, NULL};

        if (!names) {
            CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
        }

        st = lmi_class_path_is_a(_cb, KNameSpace(cop),
                LMI_SoftwareIdentity_ClassName, resultClass);
        lmi_return_if_class_check_not_ok(st);

        if (role && strcmp(role, LMI_COLLECTION) != 0) {
            goto done;
        }
        if (resultRole && strcmp(resultRole, LMI_MEMBER) != 0) {
            goto done;
        }

        get_pk_packages(0, &array, error_msg, BUFLEN);
        if (!array) {
            goto done;
        }

        /* Associate packages to repositories providing them.
         * `avail_pkg_repos` will contain just available packages. */
        get_repo_dets_for_pk_pkgs(array, NULL, &avail_pkg_repos, error_msg, BUFLEN);
        if (!avail_pkg_repos) {
            goto done;
        }

        /* Generate instance names for obtained associations. */
        g_tree_foreach(avail_pkg_repos, traverse_avail_pkg_repos_for_sw, &taprd);
    } else if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentity_ClassName, &st)) {
        /* got SoftwareIdentity - Member */
        char instance_id[BUFLEN] = "";

        st = lmi_class_path_is_a(_cb, KNameSpace(cop),
                LMI_SystemSoftwareCollection_ClassName, resultClass);
        lmi_return_if_class_check_not_ok(st);

        if (role && strcmp(role, LMI_MEMBER) != 0) {
            goto done;
        }
        if (resultRole && strcmp(resultRole, LMI_COLLECTION) != 0) {
            goto done;
        }

        if (get_sw_pkg_from_sw_identity_op(cop, &sw_pkg) != 0) {
            goto done;
        }

        get_repo_id_from_sw_pkg(&sw_pkg, &repo_id, error_msg, BUFLEN);
        if (!repo_id) {
            goto done;
        }

        create_instance_id(LMI_SystemSoftwareCollection_ClassName, NULL,
                instance_id, BUFLEN);

        LMI_SystemSoftwareCollectionRef ssc;
        LMI_SystemSoftwareCollectionRef_Init(&ssc, _cb, KNameSpace(cop));
        LMI_SystemSoftwareCollectionRef_Set_InstanceID(&ssc, instance_id);

        if (names) {
            KReturnObjectPath(cr, ssc);
        } else {
            CMPIObjectPath *o = LMI_SystemSoftwareCollectionRef_ToObjectPath(&ssc, &st);
            CMPIInstance *ci = _cb->bft->getInstance(_cb, cc, o, properties, &st);
            CMReturnInstance(cr, ci);
        }
    }

done:
    clean_sw_package(&sw_pkg);
    if (avail_pkg_repos) {
        g_tree_unref(avail_pkg_repos);
    }
    if (array) {
        g_ptr_array_unref(array);
    }
    g_free(repo_id);

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    return st;
}

static CMPIStatus LMI_MemberOfSoftwareCollectionAssociators(
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

static CMPIStatus LMI_MemberOfSoftwareCollectionAssociatorNames(
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
    CMPIStatus st;
    SwPackage sw_pkg;
    gchar *repo_id = NULL;
    char error_msg[BUFLEN] = "";

    st = lmi_class_path_is_a(_cb, KNameSpace(cop),
            LMI_MemberOfSoftwareCollection_ClassName, assocClass);
    lmi_return_if_class_check_not_ok(st);

    init_sw_package(&sw_pkg);

    if (CMClassPathIsA(_cb, cop, LMI_SystemSoftwareCollection_ClassName, &st)) {
        /* got SystemSoftwareCollection - Collection */
        if (role && strcmp(role, LMI_COLLECTION) != 0) {
            goto done;
        }

        return enum_instances(cr, KNameSpace(cop), names);
    } else if (CMClassPathIsA(_cb, cop, LMI_SoftwareIdentity_ClassName, &st)) {
        /* got SoftwareIdentity - Member */
        char instance_id[BUFLEN] = "";

        if (role && strcmp(role, LMI_MEMBER) != 0) {
            goto done;
        }

        if (get_sw_pkg_from_sw_identity_op(cop, &sw_pkg) != 0) {
            goto done;
        }

        get_repo_id_from_sw_pkg(&sw_pkg, &repo_id, error_msg, BUFLEN);
        if (!repo_id) {
            goto done;
        }

        create_instance_id(LMI_SystemSoftwareCollection_ClassName, NULL,
                instance_id, BUFLEN);

        LMI_SystemSoftwareCollectionRef ssc;
        LMI_SystemSoftwareCollectionRef_Init(&ssc, _cb, KNameSpace(cop));
        LMI_SystemSoftwareCollectionRef_Set_InstanceID(&ssc, instance_id);

        LMI_SoftwareIdentityRef si;
        LMI_SoftwareIdentityRef_InitFromObjectPath(&si, _cb, cop);

        LMI_MemberOfSoftwareCollection w;
        LMI_MemberOfSoftwareCollection_Init(&w, _cb, KNameSpace(cop));
        LMI_MemberOfSoftwareCollection_Set_Collection(&w, &ssc);
        LMI_MemberOfSoftwareCollection_Set_Member(&w, &si);

        if (names) {
            KReturnObjectPath(cr, w);
        } else {
            KReturnInstance(cr, w);
        }
    }

done:
    clean_sw_package(&sw_pkg);
    g_free(repo_id);

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemberOfSoftwareCollectionReferences(
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

static CMPIStatus LMI_MemberOfSoftwareCollectionReferenceNames(
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
    LMI_MemberOfSoftwareCollection,
    LMI_MemberOfSoftwareCollection,
    _cb,
    LMI_MemberOfSoftwareCollectionInitialize(ctx))

CMAssociationMIStub(
    LMI_MemberOfSoftwareCollection,
    LMI_MemberOfSoftwareCollection,
    _cb,
    LMI_MemberOfSoftwareCollectionInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_MemberOfSoftwareCollection",
    "LMI_MemberOfSoftwareCollection",
    "instance association")
