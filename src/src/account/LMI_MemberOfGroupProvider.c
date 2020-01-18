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
#include "LMI_MemberOfGroup.h"
#include "LMI_Group.h"
#include "LMI_Identity.h"
#include "LMI_AssignedAccountIdentity.h"

#include "aux_lu.h"
#include "macros.h"
#include "account_globals.h"
#include "globals.h"

#include <libuser/entity.h>
#include <libuser/user.h>

static const CMPIBroker* _cb;

static void LMI_MemberOfGroupInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_MemberOfGroupCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemberOfGroupEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_MemberOfGroupEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_GroupRef lgref;
    LMI_IdentityRef liref;
    LMI_MemberOfGroup lmog;

    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    GPtrArray *groups = NULL;
    GValueArray *accounts = NULL;
    struct lu_ent *lueg = NULL;
    struct lu_ent *luea = NULL;

    size_t i, j;
    const char *nameSpace = KNameSpace(cop);
    char *uid = NULL;

    luc = lu_start(NULL, 0, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc)
      {
        KReturn2(_cb, ERR_FAILED,
                 "Unable to initialize libuser: %s\n", lu_strerror(error));
      }
    /* Go through each group */
    groups = lu_groups_enumerate_full(luc, "*", &error);
    for (i = 0;  (groups != NULL) && (i < groups->len); i++)
      {
        lueg = g_ptr_array_index(groups, i);
        LMI_GroupRef_Init(&lgref, _cb, nameSpace);
        LMI_GroupRef_Set_CreationClassName(&lgref, LMI_Group_ClassName);
        LMI_GroupRef_Set_Name(&lgref, aux_lu_get_str(lueg, LU_GROUPNAME));

        /* For each user in the group */
        accounts = lu_users_enumerate_by_group(luc,
          aux_lu_get_str(lueg, LU_GROUPNAME), &error);
        for (j = 0;  (accounts != NULL) && (j < accounts->n_values); j++)
          {
            luea = lu_ent_new();
            lu_user_lookup_name(luc,
              g_value_get_string(g_value_array_get_nth(accounts, j)),
              luea, &error);
            asprintf(&uid, ORGID":UID:%ld",
              aux_lu_get_long(luea, LU_UIDNUMBER));
            LMI_IdentityRef_Init(&liref, _cb, nameSpace);
            LMI_IdentityRef_Set_InstanceID(&liref, uid);
            free(uid);

            LMI_MemberOfGroup_Init(&lmog, _cb, nameSpace);
            LMI_MemberOfGroup_Set_Collection(&lmog, &lgref);
            LMI_MemberOfGroup_Set_Member(&lmog, &liref);

            KReturnInstance(cr, lmog);
            lu_ent_free(luea);
          } /* for users */
    if (accounts)
      {
        g_value_array_free(accounts);
      }


        lu_ent_free(lueg);
      } /* for groups */

    if (groups)
      {
        g_ptr_array_free(groups, TRUE);
      }

    lu_end(luc);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemberOfGroupGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_MemberOfGroupCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMPIStatus status;
    CMPIEnumeration *instances = NULL;

    LMI_GroupRef lg_ref;
    LMI_IdentityRef li_ref;
    LMI_MemberOfGroup lmog;
    LMI_Account la;

    const char *group_name = NULL;
    const char *user_name = NULL;
    GValue val;

    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;

    CMPIrc rc = CMPI_RC_OK;
    char *errmsg = NULL;

    LMI_MemberOfGroup_InitFromObjectPath(&lmog, _cb, cop);
    LMI_GroupRef_InitFromObjectPath(&lg_ref, _cb,
        CMGetProperty(ci, "Collection", NULL).value.ref);
    LMI_IdentityRef_InitFromObjectPath(&li_ref, _cb,
        CMGetProperty(ci, "Member", NULL).value.ref);

    if (!(instances = CBAssociators(_cb, cc,
        LMI_IdentityRef_ToObjectPath(&li_ref, NULL),
        LMI_AssignedAccountIdentity_ClassName, LMI_Account_ClassName, NULL,
        NULL, NULL, &status)) || !CMHasNext(instances, &status)) {
        KReturn2(_cb, ERR_FAILED, "Unable to find user: %s\n",
          status.msg ? CMGetCharsPtr(status.msg, NULL) : "" );
    }
    LMI_Account_InitFromInstance(&la, _cb, CMGetNext(instances, &status).value.inst);

    user_name = la.Name.chars;
    group_name = lg_ref.Name.chars;

    luc = lu_start(NULL, 0, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc) {
        KReturn2(_cb, ERR_FAILED,
                 "Unable to initialize libuser: %s\n", lu_strerror(error));
    }

    struct lu_ent *lue = lu_ent_new();
    if (!lu_group_lookup_name(luc, group_name, lue, &error)) {
        asprintf(&errmsg, "Group with name %s not found: %s\n", group_name,
                          lu_strerror(error));
        rc = CMPI_RC_ERR_FAILED;
        goto fail;
    }

    memset(&val, 0, sizeof(val));
    g_value_init(&val, G_TYPE_STRING);
    g_value_set_string(&val, user_name);
    lu_ent_add(lue, LU_MEMBERNAME, &val);
    if(!lu_group_modify(luc, lue, &error)) {
        asprintf(&errmsg, "Modification of group %s failed: %s\n", group_name,
                          lu_strerror(error));
        rc = CMPI_RC_ERR_FAILED;
        goto fail;
    }

    g_value_unset(&val);
    lu_ent_free(lue);
    lu_end(luc);

    LMI_MemberOfGroup_Set_Collection(&lmog, &lg_ref);
    LMI_MemberOfGroup_Set_Member(&lmog, &li_ref);


    CMReturnObjectPath(cr, LMI_MemberOfGroup_ToObjectPath(&lmog, NULL));
    CMReturn(CMPI_RC_OK);

fail:
    lu_ent_free(lue);
    lu_end(luc);
    g_value_unset(&val);

    CMPIString *errstr = CMNewString(_cb, errmsg, NULL);
    free(errmsg);
    CMReturnWithString(rc, errstr);
}

static CMPIStatus LMI_MemberOfGroupModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemberOfGroupDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    LMI_MemberOfGroup lmog;
    LMI_GroupRef lg_ref;
    LMI_IdentityRef li_ref;

    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;

    const char *group_name = NULL;
    const char *member_name = NULL;
    uid_t user_id = -1;
    GValueArray *groups = NULL;
    unsigned int i = 0; /* iterator */
    int found = 0; /* indicator */

    CMPIrc rc = CMPI_RC_OK;
    char *errmsg = NULL;

    LMI_MemberOfGroup_InitFromObjectPath(&lmog, _cb, cop);
    LMI_GroupRef_InitFromObjectPath(&lg_ref, _cb, lmog.Collection.value);
    LMI_IdentityRef_InitFromObjectPath(&li_ref, _cb, lmog.Member.value);

    group_name = lg_ref.Name.chars;
    user_id = (uid_t)atol(strrchr(li_ref.InstanceID.chars, ':') + 1);

    luc = lu_start(NULL, 0, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc) {
        KReturn2(_cb, ERR_FAILED,
                 "Unable to initialize libuser: %s\n", lu_strerror(error));
    }
    struct lu_ent *lue_g = lu_ent_new();
    struct lu_ent *lue_u = lu_ent_new();

    if (!lu_user_lookup_id(luc, user_id, lue_u, &error)) {
        asprintf(&errmsg, "User with id %d not found: %s\n", user_id,
                          lu_strerror(error));
        rc = CMPI_RC_ERR_FAILED;
        goto fail;
    }
    member_name = aux_lu_get_str(lue_u, LU_USERNAME);

    if (!lu_group_lookup_name(luc, group_name, lue_g, &error)) {
        rc = CMPI_RC_ERR_FAILED;
        asprintf(&errmsg, "Group with name %s not found: %s\n",
                          group_name, lu_strerror(error));
        goto fail;
    }
    groups = lu_ent_get(lue_g, LU_MEMBERNAME);
    for (found = 0, i = 0; groups && i < groups->n_values; i++) {
        if (0 == strcmp(member_name, g_value_get_string(g_value_array_get_nth(
            groups, i)))) {
            found = 1;
            break;
        }
    }
    if (!found) {
        rc = CMPI_RC_ERR_FAILED;
        asprintf(&errmsg,
            "User with id %d is not in group %s or is users' primary group\n",
            user_id, group_name);
        goto fail;
    } else {
        /* And now remove the user from the group */
        lu_ent_del(lue_g, LU_MEMBERNAME, g_value_array_get_nth(groups, i));
        if(!lu_group_modify(luc, lue_g, &error)) {
            rc = CMPI_RC_ERR_FAILED;
            asprintf(&errmsg, "Modification of group %s failed: %s\n",
                              group_name, lu_strerror(error));
            goto fail;
        }
    }

fail:
    lu_ent_free(lue_u);
    lu_ent_free(lue_g);
    lu_end(luc);
    if (errmsg) {
        CMPIString *errstr = CMNewString(_cb, errmsg, NULL);
        free(errmsg);
        CMReturnWithString(rc, errstr);
    } else {
        CMReturn(CMPI_RC_OK);
    }
}

static CMPIStatus LMI_MemberOfGroupExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_MemberOfGroupAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_MemberOfGroupAssociators(
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
    return KDefaultAssociators(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_MemberOfGroup_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_MemberOfGroupAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return KDefaultAssociatorNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_MemberOfGroup_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_MemberOfGroupReferences(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    return KDefaultReferences(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_MemberOfGroup_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_MemberOfGroupReferenceNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role)
{
    return KDefaultReferenceNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_MemberOfGroup_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_MemberOfGroup,
    LMI_MemberOfGroup,
    _cb,
    LMI_MemberOfGroupInitialize(ctx))

CMAssociationMIStub( 
    LMI_MemberOfGroup,
    LMI_MemberOfGroup,
    _cb,
    LMI_MemberOfGroupInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_MemberOfGroup",
    "LMI_MemberOfGroup",
    "instance association")
