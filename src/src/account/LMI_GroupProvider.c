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
 * Authors: Roman Rakus <rrakus@redhat.com>
 */
#include <konkret/konkret.h>
#include "LMI_Group.h"

// Disable GLib deprecation warnings - GValueArray is deprecated but we
// need it because libuser uses it
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <libuser/entity.h>
#include <libuser/user.h>

#include "aux_lu.h"
#include "macros.h"
#include "account_globals.h"
#include "globals.h"

// Return values of functions
// Delete group
#define GROUP_NOT_EXIST      4096
#define GROUP_IS_PRIMARY     4097

static const CMPIBroker* _cb = NULL;

static void LMI_GroupInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_GroupCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_GroupEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_GroupEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_Group lg;
    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    GPtrArray *groups = NULL;
    struct lu_ent *lue = NULL;
    size_t i;
    const char *nameSpace = KNameSpace(cop);
    char *instanceid = NULL;

    luc = lu_start(NULL, lu_group, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc)
      {
        KReturn2(_cb, ERR_FAILED,
                 "Unable to initialize libuser: %s\n", lu_strerror(error));
      }

    groups = lu_groups_enumerate_full(luc, "*", &error);
    for (i = 0;  (groups != NULL) && (i < groups->len); i++)
      {
        lue = g_ptr_array_index(groups, i);

        LMI_Group_Init(&lg, _cb, nameSpace);
        LMI_Group_Set_CreationClassName(&lg, LMI_Group_ClassName);
        LMI_Group_Set_Name(&lg, aux_lu_get_str(lue, LU_GROUPNAME));
        LMI_Group_Set_ElementName(&lg, aux_lu_get_str(lue, LU_GROUPNAME));
        LMI_Group_Set_CommonName(&lg, aux_lu_get_str(lue, LU_GROUPNAME));
        asprintf(&instanceid, ORGID":GID:%ld",
          aux_lu_get_long(lue, LU_GIDNUMBER));
        LMI_Group_Set_InstanceID(&lg, instanceid);
        free(instanceid);
        KReturnInstance(cr, lg);
        lu_ent_free(lue);
      } /* for */

    if (groups)
      {
        g_ptr_array_free(groups, TRUE);
      }

    lu_end(luc);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_GroupGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_GroupCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_GroupModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

/*
 * Internal function to delete group
 */
static CMPIrc delete_group(
    const char *groupname,
    char *errormsg)
{
    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    struct lu_ent *lueg = NULL;
    struct lu_ent *lueu = NULL;
    GValueArray *users = NULL;
    const char *username = NULL;
    long group_gid = 0;
    CMPIrc rc = CMPI_RC_OK;

    luc = lu_start(NULL, 0, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc) {
        asprintf(&errormsg, "Unable to initialize libuser: %s\n", lu_strerror(error));
        return CMPI_RC_ERR_FAILED;
    }

    lueg = lu_ent_new();
    if (!lu_group_lookup_name(luc, groupname, lueg, &error)) { /* Group not found */
        asprintf(&errormsg, "Non existing group: %s\n", groupname);
        rc = GROUP_NOT_EXIST;
        goto clean;
    }

    /* check if the group is not primary group of any user */
    group_gid = aux_lu_get_long(lueg, LU_GIDNUMBER);
    users = lu_users_enumerate_by_group(luc, groupname, &error);
    unsigned int j;
    for (j = 0;  (users != NULL) && (j < users->n_values); j++) {
        lueu = lu_ent_new();
        username = g_value_get_string(g_value_array_get_nth(users, j));
        lu_user_lookup_name(luc, username, lueu, &error);
        if (aux_lu_get_long(lueu, LU_GIDNUMBER) == group_gid) {
            asprintf(&errormsg,
                "Cannot delete group %s, it is primary group of user %s\n",
                groupname, username);
            rc = GROUP_IS_PRIMARY;
            goto clean;
        }
        lu_ent_free(lueu);
    }

    if (!lu_group_delete(luc, lueg, &error)) {
        asprintf(&errormsg, "Group %s could not be deleted: %s\n", groupname,
                          lu_strerror(error));
        rc = CMPI_RC_ERR_FAILED;
        goto clean;
    }

clean:
    if (lueg)
        lu_ent_free(lueg);
    if (lueu)
        lu_ent_free(lueu);
    lu_end(luc);
    return rc;
}

/*
 * DEPRECATED
 */
static CMPIStatus LMI_GroupDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    LMI_Group lg;
    const char *name = NULL;
    CMPIrc rc = CMPI_RC_OK;
    char *errmsg = NULL;

    LMI_Group_InitFromObjectPath(&lg, _cb, cop);
    name = lg.Name.chars;

    rc = delete_group(name, errmsg);
    if (rc != CMPI_RC_OK) {
        CMPIStatus st = {CMPI_RC_ERR_FAILED, NULL};
        CMSetStatusWithChars(_cb, &st, CMPI_RC_ERR_FAILED, errmsg);
        free(errmsg);
        return st;
    }

    CMReturn(rc);
}

static CMPIStatus LMI_GroupExecQuery(
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
    LMI_Group,
    LMI_Group,
    _cb,
    LMI_GroupInitialize(ctx))

static CMPIStatus LMI_GroupMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_GroupInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Group_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

KUint32 LMI_Group_DeleteGroup(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_GroupRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    char *errmsg = NULL;

    CMPIrc rc = delete_group(
        self->Name.chars,
        errmsg);

    KUint32_Set(&result, rc);

    if (rc > 0 && rc < GROUP_NOT_EXIST) {
        CMSetStatusWithChars(_cb, status, rc, errmsg);
    }
    free(errmsg);

    return result;
}



CMMethodMIStub(
    LMI_Group,
    LMI_Group,
    _cb,
    LMI_GroupInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_Group",
    "LMI_Group",
    "instance method")
