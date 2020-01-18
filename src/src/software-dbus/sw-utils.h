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

#ifndef SW_UTILS_H_
#define SW_UTILS_H_

#define I_KNOW_THE_PACKAGEKIT_GLIB2_API_IS_SUBJECT_TO_CHANGE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <packagekit-glib2/packagekit.h>

#include "openlmi.h"
#include "LMI_SoftwareIdentity.h"

#define PK_DETAILS_LIMIT 4999

#define EST_OF_INSTD_PKGS 1000

#define SW_IDENTITY_INSTANCE_ID_PREFIX \
    LMI_ORGID ":" LMI_SoftwareIdentity_ClassName ":"
#define SW_IDENTITY_INSTANCE_ID_PREFIX_LEN 25

#define SW_METHOD_RESULT_INSTANCE_ID_PREFIX \
    LMI_ORGID ":" LMI_SoftwareMethodResult_ClassName ":"
#define SW_METHOD_RESULT_INSTANCE_ID_PREFIX_LEN 29

#define get_sw_pkg_from_sw_identity_op(cop, sw_pkg) \
    create_sw_package_from_elem_name(get_elem_name_from_instance_id(\
            lmi_get_string_property_from_objectpath(cop, "InstanceID")), sw_pkg)

typedef enum {
    REPO_STATE_ENUM_ANY         = 0,
    REPO_STATE_ENUM_ENABLED     = 1,
    REPO_STATE_ENUM_DISABLED    = 2,
}RepoStateEnum;

const char *library_name;
const ConfigEntry *provider_config_defaults;
PkControl *pkctrl;

/******************************************************************************
 * Provider init and cleanup
 *****************************************************************************/
void software_init(const gchar *provider_name,
                   const CMPIBroker *cb,
                   const CMPIContext *ctx,
                   gboolean is_indication_provider,
                   const ConfigEntry *provider_config_defaults);

CMPIStatus software_cleanup(const gchar *provider_name);

/*******************************************************************************
 * SwPackage & related functions
 ******************************************************************************/
/* Software Package */
typedef struct _SwPackage {
    char *name;         /* Package name w/o anything else */
    char *epoch;        /* Epoch number */
    char *version;      /* Version number, w/o epoch or release */
    char *release;      /* Release number */
    char *arch;         /* Architecture */
    char *pk_version;   /* PackageKit style package version - (epoch:)?version-release */
} SwPackage;

/*
 * Initialize SwPackage structure.
 * @param pkg SwPackage
 */
void init_sw_package(SwPackage *pkg);

/*
 * Release inner data of SwPackage object and reinitialize it.
 * @param pkg SwPackage
 */
void clean_sw_package(SwPackage *pkg);

/*
 * Free SwPackage structure.
 * @param pkg SwPackage
 */
void free_sw_package(SwPackage *pkg);

/*
 * Create SwPackage from PkPackage ID.
 * @param pk_pkg PkPackage ID
 * @param sw_pkg output package
 * @return 0 on success, negative value otherwise
 */
short create_sw_package_from_pk_pkg_id(const char *pk_pkg_id, SwPackage *sw_pkg);

/*
 * Create SwPackage from PkPackage.
 * @param pk_pkg source package
 * @param sw_pkg output package
 * @return 0 on success, negative value otherwise
 */
short create_sw_package_from_pk_pkg(PkPackage *pk_pkg, SwPackage *sw_pkg);

/*
 * Create SwPackage from element name.
 * @param elem_name element name in format name-(epoch:)?version-release.arch
 * @param sw_pkg output package
 * @return 0 on success, negative value otherwise
 */
short create_sw_package_from_elem_name(const char *elem_name, SwPackage *sw_pkg);

/*
 * Construct version name in format epoch:version-release.arch for
 * SwPackage structure.
 * @param pkg SwPackage
 * @param ver_str version string
 * @param ver_str_len length of version string
 */
void sw_pkg_get_version_str(const SwPackage *pkg, char *ver_str,
        const unsigned ver_str_len);

/*
 * Construct element name in format name-epoch:version-release.arch for
 * SwPackage structure.
 * @param pkg SwPackage
 * @param elem_name element name string
 * @param elem_name_len length of element name string
 */
void sw_pkg_get_element_name(const SwPackage *pkg, char *elem_name,
        const unsigned elem_name_len);

/*******************************************************************************
 * Functions related to single PkPackage
 ******************************************************************************/
/*
 * Checks with PackageKit whether package with this element name is installed.
 * @param elem_name element name of the package
 * @return 1 if package is installed, 0 otherwise
 */
short is_elem_name_installed(const char *elem_name);

/*
 * Get PkPackage according to the SwPackage.
 * @param sw_pkg SwPackage
 * @param filters
 * @param pk_pkg Found PkPackage object will be set on success unless `NULL`.
 *      Needs to be passed to g_object_unref() when not needed.
 * @return `TRUE` if the package is found.
 */
gboolean get_pk_pkg_from_sw_pkg(const SwPackage *sw_pkg,
                                PkBitfield filters,
                                PkPackage **pk_pkg);

/*
 * Get PkDetails from PkPackage.
 * @param pk_pkg PkPackage
 * @param pk_det PkDetails, needs to be passed to g_object_unref() when not needed
 * @param task_p if supplied, this task will be used when querying PackageKit;
 *          performance optimization, can be NULL
 */
void get_pk_det_from_pk_pkg(PkPackage *pk_pkg, PkDetails **pk_det, PkTask *task_p);

/*
 * Create LMI_SoftwareIdentity instance from data from PackageKit.
 * @param pk_pkg PkPackage
 * @param pk_det PkDetails; can be NULL
 * @param sw_pkg SwPackage
 * @param cb CMPI Broker
 * @param ns CMPI namespace
 * @param w LMI_SoftwareIdentity
 */
void create_instance_from_pkgkit_data(PkPackage *pk_pkg, PkDetails *pk_det,
        SwPackage *sw_pkg, const CMPIBroker *cb, const char *ns,
        LMI_SoftwareIdentity *w);

/*
 * Get files of PkPackage.
 * @param pk_pkg PkPackage
 * @param files out array containing files of this package; needs to be passed
 *          to g_strfreev() when not needed
 * @param error_msg error message
 * @param error_msg_len error message length
 */
void get_files_of_pk_pkg(PkPackage *pk_pkg, GStrv *files, char *error_msg,
        const unsigned error_msg_len);

/*
 * Is file part of PkPackage?
 * @param file
 * @param pk_pkg PkPackage
 * @return 1 if file is part of package, 0 otherwise
 */
short is_file_part_of_pk_pkg(const char *file, PkPackage *pk_pkg);

/*
 * Is file part of package defined by element name? Only succeeds when package
 * is installed.
 * @param file
 * @param elem_name package defined by element name
 * @return 1 if file is part of package, 0 otherwise
 */
short is_file_part_of_elem_name(const char *file, const char *elem_name);

/*
 * Create a package-id for given package and *data*.
 *
 * @note This function does not check for package-id's validity. `pk_package`
 *      is expected to be valid and `data` shall identify existing repository.
 *
 * @returns Folowing result: `<pkg_name>;<pkg_version>;<pkg_arch>;<data>`
 *      where <data> is a repo-id with optional `"installed:"` prefix.
 */
gint make_package_id_with_custom_data(const PkPackage *pk_pkg,
                                      const gchar *data,
                                      gchar *buf,
                                      guint buflen);

/*
 * Extract repository id from package id.
 *
 * This leaves out potential `"installed:"` prefix.
 */
const gchar *pk_pkg_id_get_repo_id(const gchar *pkg_id);

/*
 * Extract repository id from given package.
 *
 * This leaves any potential `"installed:"` prefix from data property.
 */
const gchar *pk_pkg_get_repo_id(const PkPackage *pk_pkg);

/*
 * Does the package belong to given repository?
 *
 * @note The check takes into account just a data property of the package. Thus
 * the returned information may not be reliable if the package is installed
 * or is available at another repository. For reliable information, use
 * `get_repo_dets_for_pk_pkgs()` or `get_repo_id_from_sw_pkg()`.
 */
gboolean pk_pkg_belongs_to_repo(const PkPackage *pk_pkg, const gchar *repo_id);

/*
 * Is given package installed?
 *
 * Checks whether *data* property of the package contains `"installed:"` prefix.
 *
 * The result is reliable.
 */
gboolean pk_pkg_is_installed(const PkPackage *pk_pkg);

/*
 * Is given package available?
 *
 * Checks whether *data* property contains name of repository where this
 * package is available.
 *
 * @note This information is unreliable for installed packages.
 * @see pk_pkg_belongs_to_repo().
 */
gboolean pk_pkg_is_available(const PkPackage *pk_pkg);

/*******************************************************************************
 * Functions related to multiple PkPackages
 ******************************************************************************/
/*
 * Get all packages from PackageKit filtered according to filters. Output array
 * is sorted.
 * @param filters
 * @param garray; needs to be passed to g_ptr_array_unref() when not needed
 * @param error_msg error message
 * @param error_msg_len error message length
 */
void get_pk_packages(PkBitfield filters, GPtrArray **garray, char *error_msg,
        const unsigned error_msg_len);

/*
 * Get PkDetails for multiple package IDs.
 * @param values_p array containing package IDs
 * @param garray out array containing PkDetails; needs to be passed
 *          to g_ptr_array_unref() when not needed
 * @param task_p if supplied, this task will be used when querying PackageKit;
 *          performance optimization, can be NULL
 */
void get_pk_det_from_array(char **values_p, GPtrArray **garray,
        PkTask *task_p);

/*
 * Return single SoftwareIdentityRef instance. This function calls KReturnInstance(),
 * so instance is returned directly from within the function.
 * @param pkg_id PkPackage ID
 * @param cb CMPI Broker
 * @param ns Namespace
 * @param cr CMPI Result
 */
CMPIStatus k_return_sw_identity_op_from_pkg_id(const char *pkg_id,
        const CMPIBroker *cb, const CMPIContext *ctx, const char *ns,
        const CMPIResult* cr);

/*
 * Enumerate SoftwareIdentityRef instances. This function calls KReturnInstance(),
 * so instances are returned directly from within the function.
 * @param filters PackageKit filters
 * @param cb CMPI Broker
 * @param ns Namespace
 * @param cr CMPI Result
 * @param error_msg error message, if filled, problem occurred
 * @param error_msg_len error message length
 */
void enum_sw_identity_instance_names(PkBitfield filters, const CMPIBroker *cb,
        const CMPIContext *ctx, const char *ns, const CMPIResult* cr,
        char *error_msg, const unsigned error_msg_len);

/*
 * Enumerate SoftwareIdentity instances. This function calls KReturnInstance(),
 * so instances are returned directly from within the function.
 * @param filters PackageKit filters
 * @param cb CMPI Broker
 * @param ns Namespace
 * @param cr CMPI Result
 * @param error_msg error message, if filled, problem occurred
 * @param error_msg_len error message length
 */
CMPIStatus enum_sw_identity_instances(PkBitfield filters, const CMPIBroker *cb,
        const char *ns, const CMPIResult* cr, char *error_msg,
        const unsigned error_msg_len);

/*
 * Get all associations of packages with repositories providing them.
 *
 * @param pk_pkgs Array of `PkPackage*` objects. Only available ones will
 *      be found in resulting tree. These will have their reference counter
 *      incremented.
 * @param pk_repo_dets Optional array of `PkRepoDetail` objects. Only
 *      repositories in this array will be checked for presence of given
 *      packages. Note that disabled repositories will be ignored.
 * @param avail_pkg_repos Resulting tree where keys are pointers to `PkPackage`
 *      objects and values `GPtrArray` with `PkRepoDetail` entries.
 *      Release the result with `g_tree_unref()`.
 */
void get_repo_dets_for_pk_pkgs(GPtrArray *pk_pkgs,
                               GPtrArray *pk_repo_dets,
                               GTree **avail_pkg_repos,
                               gchar *error_msg,
                               const unsigned error_msg_len);

/*******************************************************************************
 * Functions related to PkRepos
 ******************************************************************************/
/*
 * Get PkRepoDetail object according to repo ID.
 * @param repo_id_p repo ID
 * @param repo_p PkRepoDetail, can be NULL
 * @param error_msg error message
 * @param error_msg_len error message length
 * @return true if PkRepoDetail was found
 */
gboolean get_pk_repo(const char *repo_id_p, PkRepoDetail **repo_p, char *error_msg,
        const unsigned error_msg_len);

/*
 * Get repositories from PackageKit. Output array is sorted.
 * @param repo_state Filter out repositories with unwanted state.
 * @param garray; needs to be passed to g_ptr_array_unref() when not needed
 * @param error_msg error message
 * @param error_msg_len error message length
 */
void get_pk_repos(RepoStateEnum repo_state,
                  GPtrArray **garray,
                  char *error_msg,
                  const unsigned error_msg_len);

/*
 * Get repo ID from SwPackage.
 * @param sw_pkg SwPackage
 * @param repo_id; needs to be passed to g_free() when not needed
 * @param error_msg error message
 * @param error_msg_len error message length
 */
void get_repo_id_from_sw_pkg(const SwPackage *sw_pkg, gchar **repo_id,
        char *error_msg, const unsigned error_msg_len);

/*******************************************************************************
 * Functions related to PackageKit
 ******************************************************************************/
/*
 * Analyze PkResults and construct error message if error occurred.
 * @param results from package kit call
 * @param custom_msg message preceding error message from package kit
 * @param error_msg final error message
 * @param error_msg_len length of error_msg
 * @return 0 if no error occurred, positive value if error occurred
 *      and error_msg was modified
 */
short check_and_create_error_msg(PkResults *results, GError *gerror,
        const char *custom_msg, char *error_msg, const unsigned error_msg_len);

/*
 * Compare two PkPackage packages according to the package ID.
 * @param a
 * @param b
 * @return negative integer if the a comes before the b,
 *      0 if they are equal,
 *      or a positive integer if the a comes after the b
 */
gint pk_pkg_cmp(gpointer a, gpointer b);

/*
 * Compare two PkPackage IDs (provided as char *) using name, version and arch;
 * ignoring install status and repository info.
 * @param a
 * @param b
 * @return negative integer if the a comes before the b,
 *      0 if they are equal,
 *      or a positive integer if the a comes after the b
 */
gint pk_pkg_id_cmp(gpointer a, gpointer b);

/*
 * Compare two PkDetails according to the package ID.
 * @param a
 * @param b
 * @return negative integer if the a comes before the b,
 *      0 if they are equal,
 *      or a positive integer if the a comes after the b
 */
gint pk_det_cmp(gpointer a, gpointer b);

/*
 * Compare two PkRepoDetails according to the repo ID.
 * @param a
 * @param b
 * @return negative integer if the a comes before the b,
 *      0 if they are equal,
 *      or a positive integer if the a comes after the b
 */
gint pk_repo_cmp(gpointer a, gpointer b);

/**
 * Load and cache distribution id string from PackageKit. Its format is
 * `distro;version;arch`.
 */
const gchar *get_distro_id();

/**
 * Load and cache architecture string e.g. `x86_64` or `i686`.
 */
const gchar *get_distro_arch();

/*******************************************************************************
 * Functions related to Glib
 ******************************************************************************/
/*
 * Append array b to the array a. a may be NULL. Works with GObjects only.
 * @param a first array, may be NULL
 * @param b second array
 */
void gc_gobject_ptr_array_append(GPtrArray **a, const GPtrArray *b);

/*******************************************************************************
 * Functions related to CMPI
 ******************************************************************************/
/*
 * Get SwPackage element name from InstanceID of LMI_SoftwareIdentity object.
 * @param instance_id of LMI_SoftwareIdentity object
 * @return element name or empty string in case of some problem
 */
const char *get_elem_name_from_instance_id(const char *instance_id);

/*
 * Create standard instance ID based on class name and ID.
 * @param class_name
 * @param id, may by null
 * @param instance_id output string
 * @param instance_id_len length of output string
 */
void create_instance_id(const char *class_name, const char *id,
        char *instance_id, const unsigned instance_id_len);

/*******************************************************************************
 * Object path checks
 ******************************************************************************/
/*
 * Check object path of LMI_SystemSoftwareCollection.
 * @return true if given object path is equal to local
 *	LMI_SystemSoftwareCollection instance.
 */
bool check_system_software_collection(const CMPIBroker *cb,
                                      const CMPIObjectPath *path);

/**
 * Check object path of LMI_SoftwareIdentityResource.
 * @return true if given object path is valid for this system and
 *      particular repository exists.
 */
CMPIStatus check_software_identity_resource(const CMPIBroker *cb,
                                            const CMPIContext *ctx,
                                            const CMPIObjectPath *path,
                                            PkRepoDetail **pk_repo);

/*******************************************************************************
 * General functions
 ******************************************************************************/
/*
 * Function counting Hamming weight, i.e. number of 1's in a binary
 * representation of the number x.
 * @param x number where we want to count the 1's
 * @return number of 1's
 */
short popcount(unsigned x);

#endif /* SW_UTILS_H_ */
