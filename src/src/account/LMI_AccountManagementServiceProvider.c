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
#include "LMI_AccountManagementService.h"
#include "LMI_HostedAccountManagementService.h"
#include "CIM_ComputerSystem.h"
#include "LMI_Account.h"
#include "LMI_Identity.h"
#include "LMI_Group.h"

#include "macros.h"
#include "globals.h"

#include "aux_lu.h"
#include "account_globals.h"
#include <libuser/entity.h>
#include <libuser/user.h>

#include <sys/stat.h>
#include <unistd.h>

// Return values of functions
// common
#define RET_OK          0
#define RET_UNSUPPORTED 1
#define RET_FAILED      2
// Create account
#define RET_ACC_PWD  4096
#define RET_ACC_HOME 4097
// Create group


static const CMPIBroker* _cb = NULL;

static void LMI_AccountManagementServiceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_AccountManagementServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountManagementServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_AccountManagementServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_AccountManagementService lams;

    const char *hostname = get_system_name();

    LMI_AccountManagementService_Init(&lams, _cb, KNameSpace(cop));
    LMI_AccountManagementService_Set_CreationClassName(&lams,
      LMI_AccountManagementService_ClassName);
    LMI_AccountManagementService_Set_SystemName(&lams, hostname);
    LMI_AccountManagementService_Set_Name(&lams, LAMSNAME);
    LMI_AccountManagementService_Set_ElementName(&lams, LAMSNAME);
    LMI_AccountManagementService_Set_SystemCreationClassName(&lams,
      get_system_creation_class_name());
    LMI_AccountManagementService_Set_RequestedState(&lams,
      LMI_AccountManagementService_RequestedState_Not_Applicable);
    LMI_AccountManagementService_Set_EnabledState(&lams,
      LMI_AccountManagementService_EnabledState_Enabled);

    KReturnInstance(cr, lams);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountManagementServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_AccountManagementServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountManagementServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountManagementServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountManagementServiceExecQuery(
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
    LMI_AccountManagementService,
    LMI_AccountManagementService,
    _cb,
    LMI_AccountManagementServiceInitialize(ctx))

static CMPIStatus LMI_AccountManagementServiceMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountManagementServiceInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_AccountManagementService_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_AccountManagementService,
    LMI_AccountManagementService,
    _cb,
    LMI_AccountManagementServiceInitialize(ctx))

KUint32 LMI_AccountManagementService_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountManagementServiceRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_AccountManagementService_StartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountManagementServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_AccountManagementService_StopService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountManagementServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_AccountManagementService_CreateGroup(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountManagementServiceRef* self,
    const KRef* System,
    const KString* Name,
    const KUint32* GID,
    const KBoolean* SystemAccount,
    KRef* Group,
    KRefA* Identities,
    CMPIStatus* status)
{
    char *errmsg = NULL, *instanceid = NULL;
    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    struct lu_ent *lue = NULL;
    GValue value;
    const char *nameSpace = LMI_AccountManagementServiceRef_NameSpace(
        (LMI_AccountManagementServiceRef *) self);
    CMPIEnumeration *instances = NULL;
    LMI_GroupRef Groupref;
    LMI_IdentityRef Identityref;
    CMPIObjectPath *GroupOP = NULL, *IdentityOP = NULL;
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, OK);
    KUint32_Set(&result, 0);
#define FAIL(MSG, ERROR, STATUS, RETVAL)\
    asprintf(&errmsg, (MSG), (ERROR));\
    KSetStatus2(cb, status, STATUS, errmsg);\
    free(errmsg);\
    KUint32_Set(&result, (RETVAL));\

    if (!(Name->exists && !Name->null) || !(System->exists && !System->null))
      {
        FAIL("Required parameters not specified%s\n", "", ERR_FAILED,
             RET_FAILED);
        goto clean;
      }

    luc = lu_start(NULL, lu_user, NULL, NULL, lu_prompt_console_quiet, NULL,
      &error);
    if (!luc)
      {
        FAIL("Error initializing: %s\n", lu_strerror(error), ERR_FAILED,
             RET_FAILED);
        goto clean;
      }

    instances = cb->bft->associatorNames(cb, context,
      LMI_AccountManagementServiceRef_ToObjectPath(self, NULL),
      LMI_HostedAccountManagementService_ClassName,
      NULL, NULL, NULL, NULL);
    if (!instances ||
        !instances->ft->hasNext(instances, NULL) ||
        !KMatch(System->value,
        instances->ft->getNext(instances,NULL).value.ref))
      { /* This service is not linked with provided system */
        FAIL("Unable to create group on the given System%s\n", "",
          ERR_FAILED, RET_FAILED);
        goto clean;
      }

    lue = lu_ent_new();
    lu_group_default(luc, Name->chars,
      SystemAccount->exists && !SystemAccount->null && SystemAccount->value,
      lue);

    if (GID->exists && !GID->null)
      { /* GID number passed */
        memset(&value, 0, sizeof(value));
        lu_value_init_set_id(&value, GID->value);
        lu_ent_clear(lue, LU_GIDNUMBER);
        lu_ent_add(lue, LU_GIDNUMBER, &value);
        g_value_unset(&value);
      }

    if (!lu_group_add(luc, lue, &error))
      { /* Add group failed */
        FAIL("Group Creation failed: %s\n", lu_strerror(error), ERR_FAILED,
             RET_FAILED);
        goto clean;
      }

    /* Output created Group reference */
    LMI_GroupRef_Init(&Groupref, cb, nameSpace);
    LMI_GroupRef_Set_Name(&Groupref, Name->chars);
    LMI_GroupRef_Set_CreationClassName(&Groupref, LMI_Group_ClassName);
    GroupOP = LMI_GroupRef_ToObjectPath(&Groupref, NULL);
    KRef_SetObjectPath(Group, GroupOP);

    /* Output created group identity */
    KRefA_Init(Identities, cb, 1);
    LMI_IdentityRef_Init(&Identityref, cb, nameSpace);
    asprintf(&instanceid, ORGID":GID:%ld", aux_lu_get_long(lue, LU_GIDNUMBER));
    LMI_IdentityRef_Set_InstanceID(&Identityref, instanceid);
    free(instanceid);
    IdentityOP = LMI_IdentityRef_ToObjectPath(&Identityref, NULL);
    KRefA_Set(Identities, 0, IdentityOP);

clean:
#undef FAIL
    if (lue) lu_ent_free(lue);
    if (luc) lu_end(luc);
    return result;
}

KUint32 LMI_AccountManagementService_CreateAccount(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountManagementServiceRef* self,
    const KRef* System,
    const KString* Name,
    const KString* GECOS,
    const KString* HomeDirectory,
    const KBoolean* DontCreateHome,
    const KString* Shell,
    const KUint32* UID,
    const KUint32* GID,
    const KBoolean* SystemAccount,
    const KString* Password,
    const KBoolean* DontCreateGroup,
    const KBoolean* PasswordIsPlain,
    KRef* Account,
    KRefA* Identities,
    CMPIStatus* status)
{
/* TODO - Use embedded instance? */

    char *errmsg = NULL;

#define FAIL(MSG, ERROR, STATUS, RETVAL)\
    asprintf(&errmsg, (MSG), (ERROR));\
    KSetStatus2(cb, status, STATUS, errmsg);\
    free(errmsg);\
    KUint32_Set(&result, (RETVAL));\

    KUint32 result = KUINT32_INIT;
    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    struct lu_ent *lue = NULL, *lue_group = NULL;

    GValue val;
    int create_group = 0;
    gid_t gid = LU_VALUE_INVALID_ID, uid = LU_VALUE_INVALID_ID;
    char *group_name = NULL, *instanceid = NULL;
    const char *home = NULL;

    const char *nameSpace = LMI_AccountManagementServiceRef_NameSpace(
        (LMI_AccountManagementServiceRef *) self);
    const char *hostname = get_system_name();
    CMPIStatus st;
    CMPIEnumeration *instances = NULL;
    LMI_AccountRef Accountref;
    LMI_IdentityRef Identityref;
    CMPIObjectPath *AccountOP = NULL, *IdentityOP = NULL;

    KSetStatus(status, OK);
    KUint32_Set(&result, 0);

    if (!(Name->exists && !Name->null) || !(System->exists && !System->null))
      {
        FAIL("Required parameters not specified%s\n", "", ERR_FAILED,
             RET_FAILED);
        goto clean;
      }

    luc = lu_start(NULL, lu_user, NULL, NULL, lu_prompt_console_quiet, NULL,
      &error);
    if (!luc)
      {
        FAIL("Error initializing: %s\n", lu_strerror(error), ERR_FAILED,
             RET_FAILED);
        goto clean;
      }

    instances = cb->bft->associatorNames(cb, context,
      LMI_AccountManagementServiceRef_ToObjectPath(self, NULL),
      LMI_HostedAccountManagementService_ClassName,
      NULL, NULL, NULL, &st);
    if (!instances ||
        !instances->ft->hasNext(instances, NULL) ||
        !KMatch(System->value,
          instances->ft->getNext(instances,NULL).value.ref))
      { /* This service is not linked with provided system */
        FAIL("Unable to create account on the given System%s\n", "",
          ERR_FAILED, RET_FAILED);
        goto clean;
      }

    lue = lu_ent_new();
    lu_user_default(luc, Name->chars,
      SystemAccount->exists && !SystemAccount->null && SystemAccount->value,
      lue);

    memset(&val, 0, sizeof(val));

    /* UID */
    if (UID->exists && !UID->null)
      {
        lu_value_init_set_id(&val, UID->value);
        lu_ent_clear(lue, LU_UIDNUMBER);
        lu_ent_add(lue, LU_UIDNUMBER, &val);
        g_value_unset(&val);
      }

    /* GID */
    /* if specified GID, the group should exists and don't create it
     * if unspecified GID, check dontcreategroup, dont create "users" group,
     * create group with same name as user name
     */
    lue_group = lu_ent_new();

    if (GID->exists && !GID->null)
      { /* Specified GID */
        gid = GID->value;
        if (!lu_group_lookup_id(luc, gid, lue_group, &error))
          {
            FAIL("Non existing group: %d\n", gid, ERR_FAILED, RET_FAILED);
            goto clean;
          }
      }
    else
      { /* Not specified GID */
        if (DontCreateGroup->exists && !DontCreateGroup->null &&
            DontCreateGroup->value)
          {
            /* add user to "users" group */
            group_name = strdup("users");
          }
        else
          {
            /* add user to the group with same name as user name */
            group_name = strdup(Name->chars);
          }
        if (lu_group_lookup_name(luc, group_name, lue_group, &error))
          {
            gid = aux_lu_get_long(lue_group, LU_GIDNUMBER);
          }
        else
          {
            create_group = 1;
          }
        if (create_group)
          {
            lu_group_default(luc, group_name, 0, lue_group);
            if (!lu_group_add(luc, lue_group, &error))
              {
                FAIL("Error creating group: %s\n", lu_strerror(error),
                  ERR_FAILED, RET_FAILED);
                goto clean;
              }
          }
      }


    gid = aux_lu_get_long(lue_group, LU_GIDNUMBER);

    lu_value_init_set_id(&val, gid);
    lu_ent_clear(lue, LU_GIDNUMBER);
    lu_ent_add(lue, LU_GIDNUMBER, &val);
    g_value_unset(&val);

    g_value_init(&val, G_TYPE_STRING);
#define PARAM(ATTR, VAR)\
    if ((VAR)->exists && !(VAR)->null){\
      g_value_set_string(&val, (VAR)->chars);\
      lu_ent_clear(lue, (ATTR));\
      lu_ent_add(lue, (ATTR), &val);\
      }
    PARAM(LU_GECOS, GECOS);
    PARAM(LU_HOMEDIRECTORY, HomeDirectory);
    PARAM(LU_LOGINSHELL, Shell);
#undef PARAM
    g_value_unset(&val);

    if (!lu_user_add(luc, lue, &error))
      {
        FAIL("Account Creation failed: %s\n", lu_strerror(error), ERR_FAILED,
          RET_FAILED);
        goto clean;
      }

    /* Setup password */
    if (Password->exists && !Password->null)
      {
        bool isplain = TRUE;
        if (PasswordIsPlain->exists && !PasswordIsPlain->null &&
            PasswordIsPlain->value)
          {
            isplain = FALSE;
          }
        if (!lu_user_setpass(luc, lue, Password->chars, isplain, &error))
          {
            FAIL("Error setting password: %s\n", lu_strerror(error),
              OK, RET_ACC_PWD);
            goto output;
          }
      }

    /* Finally create home dir */
    if (!(DontCreateHome->exists && !DontCreateHome->null &&
        DontCreateHome->value) && !(SystemAccount->exists &&
        !SystemAccount->null && SystemAccount->value))
      { /* Yes, create home */
        uid = aux_lu_get_long(lue, LU_UIDNUMBER);
        gid = aux_lu_get_long(lue, LU_GIDNUMBER);
        home = aux_lu_get_str(lue, LU_HOMEDIRECTORY);

        if(!lu_homedir_populate(luc, NULL, home, uid, gid, 0700, &error))
          {
            FAIL("Error creating homedir: %s\n", lu_strerror(error),
              OK, RET_ACC_HOME);
            goto output;
          }
      }

output:
      /* Output created Account reference */
      LMI_AccountRef_Init(&Accountref, cb, nameSpace);
      LMI_AccountRef_Set_Name(&Accountref, Name->chars);
      LMI_AccountRef_Set_SystemName(&Accountref, hostname);
      LMI_AccountRef_Set_SystemCreationClassName(&Accountref,
        get_system_creation_class_name());
      LMI_AccountRef_Set_CreationClassName(&Accountref, LMI_Account_ClassName);
      AccountOP = LMI_AccountRef_ToObjectPath(&Accountref, &st);
      KRef_SetObjectPath(Account, AccountOP);

      /* Output created (or already existed) identities references
       * it means reference to Identity of Account and Group
       */
      KRefA_Init(Identities, cb, 2);

      /* Identity of Account */
      LMI_IdentityRef_Init(&Identityref, cb, nameSpace);
      asprintf(&instanceid, ORGID":UID:%ld",
        aux_lu_get_long(lue, LU_UIDNUMBER));
      LMI_IdentityRef_Set_InstanceID(&Identityref, instanceid);
      free(instanceid);
      IdentityOP = LMI_IdentityRef_ToObjectPath(&Identityref, &st);
      KRefA_Set(Identities, 0, IdentityOP);

      /* Identity of Group */
      asprintf(&instanceid, ORGID":GID:%ld",
        aux_lu_get_long(lue, LU_GIDNUMBER));
      LMI_IdentityRef_Set_InstanceID(&Identityref, instanceid);
      free(instanceid);
      IdentityOP = LMI_IdentityRef_ToObjectPath(&Identityref, &st);
      KRefA_Set(Identities, 1, IdentityOP);

clean:
#undef FAIL
    free(group_name);
    if (lue) lu_ent_free(lue);
    if (lue_group) lu_ent_free(lue_group);
    if (luc) lu_end(luc);

    return result;
}

KUint32 LMI_AccountManagementService_ChangeAffectedElementsAssignedSequence(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountManagementServiceRef* self,
    const KRefA* ManagedElements,
    const KUint16A* AssignedSequence,
    KRef* Job,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_AccountManagementService_CreateUserContact(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountManagementServiceRef* self,
    const KRef* System,
    const KString* UserContactTemplate,
    KRef* UserContact,
    KRefA* Identities,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_AccountManagementService_getAccount(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountManagementServiceRef* self,
    const KString* UserID,
    KRef* Account,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KEXTERN KUint32 LMI_AccountManagementService_getUserContact(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountManagementServiceRef* self,
    const KString* UserID,
    KRef* UserContact,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AccountManagementService",
    "LMI_AccountManagementService",
    "instance method")
