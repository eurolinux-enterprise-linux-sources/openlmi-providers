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
 * Authors: Roman Rakus <rrakus@redhat.com>
 */

#include <shadow.h>
#include <konkret/konkret.h>
#include "LMI_Identity.h"

#include <libuser/entity.h>
#include <libuser/user.h>

#include "macros.h"
#include "aux_lu.h"
#include "account_globals.h"

static const CMPIBroker* _cb = NULL;

static void LMI_IdentityInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_IdentityCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IdentityEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_IdentityEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_Identity li;
    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    GPtrArray *accounts = NULL;
    struct lu_ent *lue = NULL;
    char *instanceid = NULL;
    unsigned int i = 0;
    const char *nameSpace = KNameSpace(cop);

    luc = lu_start(NULL, lu_user, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc)
      {
        KReturn2(_cb, ERR_FAILED,
                 "Unable to initialize libuser: %s\n", lu_strerror(error));
      }


    /* USERS */
    accounts = lu_users_enumerate_full(luc, "*", &error);
    for (i = 0;  (accounts != NULL) && (i < accounts->len); i++)
      {
        lue = g_ptr_array_index(accounts, i);
        LMI_Identity_Init(&li, _cb, nameSpace);

        /* Need to convert long int UID to the string */
        asprintf(&instanceid, LMI_ORGID":UID:%ld",
          aux_lu_get_long(lue, LU_UIDNUMBER));
        LMI_Identity_Set_InstanceID(&li, instanceid);
          free(instanceid);
        LMI_Identity_Set_ElementName(&li, aux_lu_get_str(lue, LU_GECOS));
        KReturnInstance(cr, li);
        lu_ent_free(lue);
      }

    if (accounts)
      {
        g_ptr_array_free(accounts, TRUE);
      }

    /* GOUPRS */
    accounts = lu_groups_enumerate_full(luc, "*", &error);
    for (i = 0;  (accounts != NULL) && (i < accounts->len); i++)
      {
        lue = g_ptr_array_index(accounts, i);
        LMI_Identity_Init(&li, _cb, nameSpace);

        /* Need to convert long int UID to the string */
        asprintf(&instanceid, LMI_ORGID":GID:%ld",
          aux_lu_get_long(lue, LU_GIDNUMBER));
        LMI_Identity_Set_InstanceID(&li, instanceid);
        free(instanceid);
        LMI_Identity_Set_ElementName(&li, aux_lu_get_str(lue, LU_GROUPNAME));
        KReturnInstance(cr, li);
        lu_ent_free(lue);
      }

    if (accounts)
      {
        g_ptr_array_free(accounts, TRUE);
      }
    lu_end(luc);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IdentityGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_IdentityCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IdentityModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IdentityDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    LMI_Identity identity;
    const char* instance_id = NULL;
    id_t id;
    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    struct lu_ent *lue = NULL;
    char *errmsg = NULL;
    CMPIrc rc = CMPI_RC_OK;
    int pwdlockres;

    LMI_Identity_InitFromObjectPath(&identity, _cb, cop);
    instance_id = identity.InstanceID.chars;
    id = atol(rindex(instance_id, ':') + 1);

    pwdlockres = lckpwdf();
    if (pwdlockres != 0)
        lmi_warn("Cannot acquire passwd file lock\n");

    luc = lu_start(NULL, 0, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc)
      {
        if (pwdlockres == 0)
            ulckpwdf();
        KReturn2(_cb, ERR_FAILED,
                 "Unable to initialize libuser: %s\n", lu_strerror(error));
      }

    lue = lu_ent_new();
    if (strstr(instance_id, ":GID:"))
      { /* It's a group */
        if (!lu_group_lookup_id(luc, id, lue, &error))
          { /* User with that ID is not present */
            asprintf(&errmsg, "Non existing group id: %d\n", id);
            rc = CMPI_RC_ERR_NOT_FOUND;
            goto fail;
          }
        else
          {
            if (!lu_group_delete(luc, lue, &error))
              { /* user delete error */
                rc = CMPI_RC_ERR_FAILED;
                asprintf(&errmsg,
                  "Group with id %d could not be deleted: %s\n", id,
                  lu_strerror(error));
                goto fail;
              }
          }
      }
    else
      { /* It's a user */
        if (!lu_user_lookup_id(luc, id, lue, &error))
          { /* User with that ID is not present */
            rc = CMPI_RC_ERR_NOT_FOUND;
            asprintf(&errmsg, "Non existing user id: %d\n", id);
            goto fail;
          }
        else
          {
            if (!lu_user_delete(luc, lue, &error))
              { /* user delete error */
                rc = CMPI_RC_ERR_FAILED;
                asprintf(&errmsg, "User with id %d could not be deleted: %s\n",
                                  id, lu_strerror(error));
                goto fail;
              }
          }
      }
fail:
    lu_ent_free(lue);
    lu_end(luc);
    if (pwdlockres == 0)
        ulckpwdf();
    if (errmsg) {
        CMPIString *errstr = CMNewString(_cb, errmsg, NULL);
        free(errmsg);
        CMReturnWithString(rc, errstr);
    } else {
        CMReturn(rc);
    }
}

static CMPIStatus LMI_IdentityExecQuery(
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
    LMI_Identity,
    LMI_Identity,
    _cb,
    LMI_IdentityInitialize(ctx))

static CMPIStatus LMI_IdentityMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IdentityInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Identity_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_Identity,
    LMI_Identity,
    _cb,
    LMI_IdentityInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_Identity",
    "LMI_Identity",
    "instance method")
