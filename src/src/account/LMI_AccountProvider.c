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
#include "LMI_Account.h"

#include <time.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#include <utmp.h>

// Disable GLib deprecation warnings - GValueArray is deprecated but we
// need it because libuser uses it
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <glib.h>

#undef GLIB_DISABLE_DEPRECATION_WARNINGS

#include <libuser/entity.h>
#include <libuser/user.h>

#include <cmpi/cmpidt.h>
#include <cmpi/cmpift.h>
#include <cmpi/cmpimacs.h>


#include "aux_lu.h"
#include "macros.h"
#include "globals.h"
#include "lock.h"
#include "account_globals.h"

// Return values of functions
// Delete user
#define USER_NOT_EXIST      4096
#define CANNOT_DELETE_HOME  4097
#define CANNOT_DELETE_USER  4098
#define CANNOT_DELETE_GROUP 4099
// Change password
#define CHANGE_PASSWORD_OK   0
#define CHANGE_PASSWORD_FAIL 1

static const CMPIBroker* _cb = NULL;

static void LMI_AccountInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
    if (init_lock_pools() == 0) {
        error("Unable to initialize lock pool.");
        exit (1);
    }
}

static CMPIStatus LMI_AccountCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    destroy_lock_pools();
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_AccountEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_Account la;
    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    GPtrArray *accounts = NULL;
    struct lu_ent *lue = NULL;

    size_t i;
    const char *nameSpace = KNameSpace(cop);
    const char *hostname = get_system_name();
    char *uid = NULL;
    long last_change, min_lifetime, max_lifetime, warn, inactive, expire;
    time_t last_login;
    CMPIStatus *rc = NULL;
    const char *password = NULL;

    luc = lu_start(NULL, lu_user, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc)
      {
        KReturn2(_cb, ERR_FAILED,
                 "Unable to initialize libuser: %s\n", lu_strerror(error));
      }

    accounts = lu_users_enumerate_full(luc, "*", &error);
    for (i = 0;  (accounts != NULL) && (i < accounts->len); i++)
      {
        lue = g_ptr_array_index(accounts, i);

        LMI_Account_Init(&la, _cb, nameSpace);
        LMI_Account_Set_CreationClassName(&la, LMI_Account_ClassName);
        LMI_Account_Set_SystemName(&la, hostname);
        LMI_Account_Set_SystemCreationClassName(&la, get_system_creation_class_name());
        LMI_Account_Set_Name(&la, aux_lu_get_str(lue, LU_USERNAME));

        LMI_Account_Init_OrganizationName(&la, 1); /* XXX */
        LMI_Account_Set_OrganizationName(&la, 0, ""); /* XXX */

        /* Need to convert long int UID to the string */
        asprintf(&uid, "%ld", aux_lu_get_long(lue, LU_UIDNUMBER));
        LMI_Account_Set_UserID(&la, uid);
        free(uid);

        LMI_Account_Init_Host(&la, 1);
        LMI_Account_Set_Host(&la, 0, hostname);
        LMI_Account_Set_ElementName(&la, aux_lu_get_str(lue, LU_GECOS));
        LMI_Account_Set_HomeDirectory(&la, aux_lu_get_str(lue,
          LU_HOMEDIRECTORY));
        LMI_Account_Set_LoginShell(&la, aux_lu_get_str(lue, LU_LOGINSHELL));

        last_change = aux_lu_get_long(lue, LU_SHADOWLASTCHANGE);
        if (last_change != SHADOW_VALUE_EMPTY)
          {
            LMI_Account_Set_PasswordLastChange(&la,
              CMNewDateTimeFromBinary(_cb, DAYSTOMS(last_change), false, rc));
          }
        min_lifetime = aux_lu_get_long(lue, LU_SHADOWMIN);
        max_lifetime = aux_lu_get_long(lue, LU_SHADOWMAX);

        if (min_lifetime != SHADOW_VALUE_EMPTY)
          {
            LMI_Account_Set_PasswordPossibleChange(&la,
              CMNewDateTimeFromBinary(_cb, DAYSTOMS(min_lifetime), true, rc));
          }

        if (max_lifetime != SHADOW_VALUE_EMPTY &&
            max_lifetime != SHADOW_MAX_DISABLED)
          {
            LMI_Account_Set_PasswordExpiration(&la,
              CMNewDateTimeFromBinary(_cb, DAYSTOMS(max_lifetime), true, rc));
            warn = aux_lu_get_long(lue, LU_SHADOWWARNING);
              LMI_Account_Set_PasswordExpirationWarning(&la,
                CMNewDateTimeFromBinary(_cb, DAYSTOMS(warn), true, rc));
            inactive = aux_lu_get_long(lue, LU_SHADOWINACTIVE);
            if (inactive != SHADOW_VALUE_EMPTY)
              {
                LMI_Account_Set_PasswordInactivation(&la,
                  CMNewDateTimeFromBinary(_cb, DAYSTOMS(inactive), true, rc));
              }
          }
        else
          {
            LMI_Account_Null_PasswordExpiration(&la);
            LMI_Account_Null_PasswordExpirationWarning(&la);
            LMI_Account_Null_PasswordInactivation(&la);
          }

        expire = aux_lu_get_long(lue, LU_SHADOWEXPIRE);
        if (expire != SHADOW_VALUE_EMPTY)
          {
            LMI_Account_Set_AccountExpiration(&la,
              CMNewDateTimeFromBinary(_cb, DAYSTOMS(expire), false, rc));
          }
        else
          {
            LMI_Account_Null_AccountExpiration(&la);
          }

        last_login = aux_utmp_latest(aux_lu_get_str(lue, LU_USERNAME));

        if (last_login != SHADOW_VALUE_EMPTY)
          {
            LMI_Account_Set_LastLogin(&la,
              CMNewDateTimeFromBinary(_cb, STOMS(last_login), false, rc));
          }

        password = aux_lu_get_str(lue, LU_SHADOWPASSWORD);
        LMI_Account_Init_UserPassword(&la, 1);
        LMI_Account_Set_UserPassword(&la, 0, password);
        /* Assume all passwords (encrypted or not) are in ascii encoding */
        LMI_Account_Set_UserPasswordEncoding(&la, 2);

        KReturnInstance(cr, la);
        lu_ent_free(lue);
      } /* for */

    if (accounts)
      {
        g_ptr_array_free(accounts, TRUE);
      }

    lu_end(luc);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_AccountCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

/* Need better view on set date time properties */
typedef struct _date_time_prop
{
  long value;
  bool null; /* True if property is set to null - has no value */
  bool interval; /* True if value is interval */
} date_time_prop;

static CMPIStatus LMI_AccountModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    /* TODO:
     *
     * Allow to modify the following:
     * Set up account expiration
     * Set up password expiration
     */
    const char* value = NULL, *password = NULL;
    CMPIString* vs = NULL;
    CMPIArray* ar = NULL; int arsize;/* used for password */
    CMPIData data;

    struct lu_context *luc = NULL;
    struct lu_ent *lue = NULL;
    struct lu_error *error = NULL;
    GValue val;
    guint i = 0;

    long last_change;
    date_time_prop expiration, warning, inactive_password, inactive_account;
    date_time_prop possible_change;

    CMPIrc rc = CMPI_RC_OK;
    char *errmsg = NULL;

    LMI_Account la;

    LMI_Account_InitFromObjectPath(&la, _cb, cop);

    /* boilerplate code */
    char userlock[USERNAME_LEN_MAX] = {0};
    /* -1 for NULL char */
    strncpy(userlock, la.Name.chars, sizeof(userlock) - 1);
    lmi_debug("Getting lock for user: %s", userlock);
    if (get_user_lock(userlock) == 0) {
        KReturn2(_cb, ERR_FAILED, "Unable to obtain lock.");
    }

    luc = lu_start(NULL, lu_user, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc)
      {
        const int ret = release_user_lock(userlock);
        lmi_debug("Releasing lock for user: %s. Result: %d", userlock, ret);
        KReturn2(_cb, ERR_FAILED,
                 "Unable to initialize libuser: %s\n", lu_strerror(error));
      }

    lue = lu_ent_new();
    if (!lu_user_lookup_name(luc, la.Name.chars, lue, &error))
      {
        rc = CMPI_RC_ERR_NOT_FOUND;
        asprintf(&errmsg, "User %s not found: %s",
                          la.Name.chars, lu_strerror(error));
        goto fail;
      }

    /* get list of groups and lock them. userlock variable is our stored username */
    GValueArray *groups = lu_groups_enumerate_by_user (luc, userlock, &error);
    if (groups == NULL)
      {
        rc = CMPI_RC_ERR_NOT_FOUND;
        asprintf(&errmsg, "Get groups for user %s failed with: %s",
                          userlock, lu_strerror(error));
        goto fail;
      }

    for (i = 0; i < groups->n_values; ++i) {
        GValue *group_val = g_value_array_get_nth (groups, i);
        const gchar *const group_str = g_value_get_string(group_val);
        const int ret = get_group_lock((const char *const) group_str);
        lmi_debug("Getting lock for group: %s. Result: %d", group_str, ret);
    }

    data = ci->ft->getProperty(ci, "UserPassword", NULL);
    ar = data.value.array;
    if (ar && (arsize = ar->ft->getSize(ar, NULL) > 0))
      {
        vs = ar->ft->getElementAt(ar, 0, NULL).value.string;
        value = vs->ft->getCharPtr(vs, NULL);
        password = aux_lu_get_str(lue, LU_SHADOWPASSWORD);
        if (strcmp(password, value) != 0)
          {
            if (!lu_user_setpass(luc, lue, value, TRUE, &error))
              {
                rc = CMPI_RC_ERR_FAILED;
                asprintf(&errmsg, "Error setting password: %s",
                                  lu_strerror(error));
                goto fail;
              }
          }
      }
    else
      {
        if (!lu_user_removepass(luc, lue, &error))
          {
            rc = CMPI_RC_ERR_FAILED;
            asprintf(&errmsg, "Error removing password: %s",
                              lu_strerror(error));
            goto fail;
          }
      }

#define PARAMSTR(ATTR, VAR)\
    g_value_set_string(&val, (VAR));\
    lu_ent_clear(lue, (ATTR));\
    lu_ent_add(lue, (ATTR), &val);\

#define PARAMLONG(ATTR, VAR)\
    if (!(VAR).null) {\
      g_value_set_long(&val, (VAR).value);\
      lu_ent_clear(lue, (ATTR));\
      lu_ent_add(lue, (ATTR), &val);}\

/* This macro will get property named NAME and save it into `value' variable */
#define GETSTRVALUE(NAME)\
    data = ci->ft->getProperty(ci, (NAME), NULL);\
    vs = data.value.string;\
    value = vs->ft->getCharPtr(vs, NULL);\

#define GETDATEVALUE(NAME, VAR)\
    (VAR).null = CMGetProperty(ci, (NAME), NULL).state == CMPI_nullValue;\
    if (!(VAR).null)\
      {\
        (VAR).interval = CMIsInterval(\
          CMGetProperty(ci, (NAME), NULL).value.dateTime, NULL); \
        (VAR).value = MSTODAYS(CMGetBinaryFormat(\
          CMGetProperty(ci, (NAME), NULL).value.dateTime, NULL));\
      }\

    /* First string values */
    memset(&val, 0, sizeof(val));
    g_value_init(&val, G_TYPE_STRING);

    GETSTRVALUE("ElementName");
    PARAMSTR(LU_GECOS, value);

    GETSTRVALUE("HomeDirectory");
    PARAMSTR(LU_HOMEDIRECTORY, value);

    GETSTRVALUE("LoginShell");
    PARAMSTR(LU_LOGINSHELL, value);

    /* Now long values */
    memset(&val, 0, sizeof(val));
    g_value_init(&val, G_TYPE_LONG);

    last_change = aux_lu_get_long(lue, LU_SHADOWLASTCHANGE);

#define FAIL(msg) \
  for (i = 0; (groups != NULL) && (i < groups->n_values); ++i) { \
      GValue *group_val = g_value_array_get_nth (groups, i); \
      const gchar *const group_str = g_value_get_string(group_val); \
      const int ret = release_group_lock((const char *const) group_str); \
      lmi_debug("Releasing lock for group: %s. Result: %d", group_str, ret); \
  } \
  g_value_array_free (groups); \
  groups = NULL; \
  const int ret = release_user_lock(userlock); \
  lmi_debug("Releasing lock for user: %s. Result: %d", userlock, ret); \
  lu_end(luc); \
  lu_ent_free(lue); \
  KReturn2(_cb, ERR_FAILED, msg);
    GETDATEVALUE("PasswordExpiration", expiration);
    if (!expiration.null && !expiration.interval)
      {
        if ((expiration.value -= last_change) < 0)
          {
            FAIL("Wrong property setting, "
              "PasswordExpiration must be later than "
              "PasswordLastChange\n");
          }
      }

    GETDATEVALUE("PasswordExpirationWarning", warning);
    if (!warning.null && !warning.interval)
      {
        if (expiration.null)
          {
            FAIL("Wrong property setting, "
              "PasswordExpiration must be set if you want to set "
              "PasswordExpirationWarning as time of date\n");
          }
        warning.value = last_change + expiration.value - warning.value;
        if (warning.value < 0)
          {
            FAIL("Wrong property setting, "
              "PasswordExpirationWarning must be earlier than "
              "PasswordExpiration\n");
          }
      }

    GETDATEVALUE("PasswordInactivation", inactive_password);
    if (!inactive_password.null && !inactive_password.interval)
      {
        if (expiration.null)
          {
            FAIL("Wrong property setting, "
              "PasswordExpiration must be set if you want to set "
              "PasswordInactivation as time of date\n");
          }
        inactive_password.value -= last_change + expiration.value;
        if (inactive_password.value < 0)
          {
            FAIL("Wrong property setting, "
              "PasswordInactivation must be later than "
              "PasswordExpiration\n");
          }
      }

    GETDATEVALUE("PasswordPossibleChange", possible_change);
    if (!possible_change.null && !possible_change.interval)
      {
        if ((possible_change.value -= last_change) < 0)
          {
            FAIL("Wrong property setting, "
              "PasswordPossibleChange must be later than "
              "PasswordLastChange\n");
          }
      }

    GETDATEVALUE("AccountExpiration", inactive_account);
    if (!inactive_account.null && inactive_account.interval)
      {
        FAIL("Wrong property setting, "
          "AccountExpiration must be set as interval\n");
      }
#undef FAIL

    PARAMLONG(LU_SHADOWMIN, possible_change);
    PARAMLONG(LU_SHADOWMAX, expiration);
    PARAMLONG(LU_SHADOWWARNING, warning);
    PARAMLONG(LU_SHADOWINACTIVE, inactive_password);
    PARAMLONG(LU_SHADOWEXPIRE, inactive_account);

#undef PARAMSTR
#undef PARAMLONG
#undef GETSTRVALUE
#undef GETDATEVALUE
    g_value_unset(&val);

    if (!lu_user_modify(luc, lue, &error))
      {
        rc = CMPI_RC_ERR_FAILED;
        asprintf(&errmsg, "User modification failed: %s",
                          lu_strerror(error));
        goto fail;
      }

fail:
    for (i = 0; (groups != NULL) && (i < groups->n_values); ++i) {
        GValue *group_val = g_value_array_get_nth (groups, i);
        const gchar *const group_str = g_value_get_string(group_val);
        const int ret = release_group_lock((const char *const) group_str);
        lmi_debug("Releasing lock for group: %s. Result: %d", group_str, ret);
    }
    g_value_array_free (groups);
    groups = NULL;
    const int ret = release_user_lock(userlock);
    lmi_debug("Releasing lock for user: %s. Result: %d", userlock, ret);
    lu_ent_free(lue);
    lu_end(luc);

    if (errmsg) {
        CMPIString *errstr = CMNewString(_cb, errmsg, NULL);
        free(errmsg);
        CMReturnWithString(rc, errstr);
    } else {
        CMReturn(rc);
    }
}

/*
 * Internal function to delete user
 * By default don't delete home if the user is not owner of dir
 * params:
 * username    - name of user to delete
 * deletehome  - if true try to delete home
 * deletegroup - if true try to delete primary group of user
 * force       - force to delete home dir
 * errormsg    - on return contains error message, called should free memory
 */
static CMPIrc delete_user(
    const char *username,
    const bool deletehome,
    const bool deletegroup,
    const bool force,
    char *errormsg)
{
    CMPIrc rc = CMPI_RC_OK;
    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    struct lu_ent *lue = NULL;
    struct lu_ent *lueg = NULL;

    /* boilerplate code */
    char userlock[USERNAME_LEN_MAX] = {0};
    /* -1 for NULL char */
    strncpy(userlock, username, sizeof(userlock) - 1);
    lmi_debug("Getting lock for user: %s. Result: %d", userlock);
    if (get_user_lock(userlock) == 0) {
        asprintf(&errormsg, "Unable to obtain lock.");
        return CMPI_RC_ERR_FAILED;
    }

    luc = lu_start(NULL, 0, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc) {
        const int ret = release_user_lock(userlock);
        lmi_debug("Releasing lock for user: %s. Result: %d", userlock, ret);
        asprintf(&errormsg, "Unable to initialize libuser: %s\n", lu_strerror(error));
        return CMPI_RC_ERR_FAILED;
    }

    lue = lu_ent_new();
    lueg = lu_ent_new();

    if (!lu_user_lookup_name(luc, username, lue, &error)) {
        asprintf(&errormsg, "Non existing user: %s\n", username);
        rc = USER_NOT_EXIST;
        goto clean;
    }

    /* Be really safe here, it can delete ANY directory */
    /* Actually, if user have / set as home directory
        and removing is forced... */
    if (deletehome) {
        gboolean ret = FALSE;
        if (force) {
            /* Force remove home dir even if the user is NOT owner */
            ret = lu_homedir_remove_for_user(lue, &error);
        } else {
            /* Delete home dir only if the directory is owned by the user */
            ret = lu_homedir_remove_for_user_if_owned (lue, &error);
        }

        if (!ret) {
            const char *const home = lu_ent_get_first_string(lue, LU_HOMEDIRECTORY);
            /* If null is returned then asprintf handle it. */
            asprintf(&errormsg,
                     "User's homedir %s could not be deleted: %s\n", home,
                     lu_strerror(error));
            rc = CANNOT_DELETE_HOME;
            goto clean;
        }
    }

    if (!lu_user_delete(luc, lue, &error)) {
        asprintf(&errormsg, "User %s could not be deleted: %s\n",
                 username, lu_strerror(error));
        rc = CANNOT_DELETE_USER;
        goto clean;
    }

    if (deletegroup) {
        gid_t gid;
        gid = lu_ent_get_first_id(lue, LU_GIDNUMBER);

        if (gid == LU_VALUE_INVALID_ID) {
            asprintf(&errormsg, "%s did not have a gid number.\n", username);
            rc = CANNOT_DELETE_GROUP;
            goto clean;
        }

        if (lu_group_lookup_id(luc, gid, lueg, &error) == FALSE) {
            asprintf(&errormsg, "No group with GID %jd exists, not removing.\n",
                     (intmax_t)gid);
            rc = CANNOT_DELETE_GROUP;
            goto clean;
        }
        const char *tmp = lu_ent_get_first_string(lueg, LU_GROUPNAME);
        if (!tmp) {
            asprintf(&errormsg,
                     "Group with GID %jd did not have a group name.\n",
                     (intmax_t)gid);
            rc = CANNOT_DELETE_GROUP;
            goto clean;
        }
        if (strcmp(tmp, username) == 0) {
            if (lu_group_delete(luc, lueg, &error) == FALSE) {
                asprintf(&errormsg, "Group %s could not be deleted: %s.\n",
                         tmp, lu_strerror(error));
                rc = CANNOT_DELETE_GROUP;
                goto clean;
            }
        }
    }

clean:
    /* Because C specification... */
    {
        const int ret = release_user_lock(userlock);
        lmi_debug("Releasing lock for user: %s. Result: %d", userlock, ret);
    }
    lu_ent_free(lue);
    lu_ent_free(lueg);
    lu_end(luc);

    return rc;
}

/*
 * DEPRECATED
 */
static CMPIStatus LMI_AccountDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    LMI_Account acc;
    char *errmsg = NULL;

    LMI_Account_InitFromObjectPath(&acc, _cb, cop);
    CMPIrc rc = delete_user(acc.Name.chars, true, true, false, errmsg);
    if (rc != CMPI_RC_OK) {
        CMPIStatus st = {CMPI_RC_ERR_FAILED, NULL};
        CMSetStatusWithChars(_cb, &st, rc, errmsg);
        free(errmsg);
        return st;
    }

    CMReturn(rc);
}

static CMPIStatus LMI_AccountExecQuery(
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
    LMI_Account,
    LMI_Account,
    _cb,
    LMI_AccountInitialize(ctx))

static CMPIStatus LMI_AccountMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Account_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}


CMMethodMIStub(
    LMI_Account,
    LMI_Account,
    _cb,
    LMI_AccountInitialize(ctx))

KUint32 LMI_Account_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Account_ChangePassword(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountRef* self,
    const KString* Password,
    CMPIStatus* status)
{
    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    struct lu_ent *lue = NULL;
    char *errmsg = NULL;
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, CHANGE_PASSWORD_OK);

    if(!(Password->exists && !Password->null)) {
        asprintf(&errmsg, "Password parameter has to be set");
        KUint32_Set(&result, CHANGE_PASSWORD_FAIL);
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, errmsg);
        goto clean;
    }

    luc = lu_start(NULL, lu_user, NULL, NULL, lu_prompt_console_quiet, NULL,
      &error);
    if (!luc) {
        asprintf(&errmsg, "Error initializing: %s\n", lu_strerror(error));
        KUint32_Set(&result, CHANGE_PASSWORD_FAIL);
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, errmsg);
        goto clean;
    }

    lue = lu_ent_new();

    if (!lu_user_lookup_name(luc, self->Name.chars, lue, &error)) {
        asprintf(&errmsg, "Non existing user: %s\n", self->Name.chars);
        KUint32_Set(&result, CHANGE_PASSWORD_FAIL);
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, errmsg);
        goto clean;
    }

    if (!lu_user_setpass(luc, lue, Password->chars, FALSE, &error)) {
        asprintf(&errmsg, "Cannot change password: %s\n", lu_strerror(error));
        KUint32_Set(&result, CHANGE_PASSWORD_FAIL);
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, errmsg);
        goto clean;
    }

clean:
    free(errmsg);
    if(luc) lu_end(luc);
    if(lue) lu_ent_free(lue);
    return result;
}


KUint32 LMI_Account_DeleteUser(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountRef* self,
    const KBoolean* DontDeleteHomeDirectory,
    const KBoolean* DontDeleteGroup,
    const KBoolean* Force,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    bool deletehome = !(DontDeleteHomeDirectory->exists &&
                        !DontDeleteHomeDirectory->null &&
                        DontDeleteHomeDirectory->value);

    bool deletegroup = !(DontDeleteGroup->exists &&
                         !DontDeleteGroup->null &&
                         DontDeleteGroup->value);

    char *errmsg = NULL;

    CMPIrc rc = delete_user(
        self->Name.chars,
        deletehome,
        deletegroup,
        (Force->exists && !Force->null && Force->value),
        errmsg);

    KUint32_Set(&result, rc);

    if (rc > 0 && rc < USER_NOT_EXIST) {
        CMSetStatusWithChars(_cb, status, rc, errmsg);
    }
    free(errmsg);

    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_Account",
    "LMI_Account",
    "instance method")
