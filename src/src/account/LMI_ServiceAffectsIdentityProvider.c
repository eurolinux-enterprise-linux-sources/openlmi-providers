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
#include "LMI_ServiceAffectsIdentity.h"
#include "LMI_AccountManagementService.h"
#include "LMI_Identity.h"

#include "macros.h"
#include "globals.h"
#include "aux_lu.h"
#include "account_globals.h"

#include <libuser/entity.h>
#include <libuser/user.h>

static const CMPIBroker* _cb;

static void LMI_ServiceAffectsIdentityInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_ServiceAffectsIdentityCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ServiceAffectsIdentityEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ServiceAffectsIdentityEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_IdentityRef liref;
    LMI_AccountManagementServiceRef lamsref;
    LMI_ServiceAffectsIdentity lsai;

    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    GPtrArray *accounts = NULL;
    struct lu_ent *lue = NULL;
    size_t i;
    char *id = NULL;

    const char *nameSpace = KNameSpace(cop);
    const char *hostname = get_system_name();

    LMI_AccountManagementServiceRef_Init(&lamsref, _cb, nameSpace);
    LMI_AccountManagementServiceRef_Set_Name(&lamsref, LAMSNAME);
    LMI_AccountManagementServiceRef_Set_SystemCreationClassName(&lamsref,
      get_system_creation_class_name());
    LMI_AccountManagementServiceRef_Set_SystemName(&lamsref, hostname);
    LMI_AccountManagementServiceRef_Set_CreationClassName(&lamsref,
      LMI_AccountManagementService_ClassName);

    luc = lu_start(NULL, lu_user, NULL, NULL, lu_prompt_console_quiet, NULL, &error);
    if (!luc)
      {
        KReturn2(_cb, ERR_FAILED,
                 "Unable to initialize libuser: %s\n", lu_strerror(error));
      }


    /* Go through every accounts first */
    accounts = lu_users_enumerate_full(luc, "*", &error);
    for (i = 0;  (accounts != NULL) && (i < accounts->len); i++)
      {
        lue = g_ptr_array_index(accounts, i);

        LMI_IdentityRef_Init(&liref, _cb, nameSpace);
        asprintf(&id, ORGID":UID:%ld", aux_lu_get_long(lue, LU_UIDNUMBER));
        LMI_IdentityRef_Set_InstanceID(&liref, id);
        free(id);

        LMI_ServiceAffectsIdentity_Init(&lsai, _cb, nameSpace);
        LMI_ServiceAffectsIdentity_Set_AffectedElement(&lsai, &liref);
        LMI_ServiceAffectsIdentity_Set_AffectingElement(&lsai, &lamsref);
        LMI_ServiceAffectsIdentity_Init_ElementEffects(&lsai, 1);
        LMI_ServiceAffectsIdentity_Set_ElementEffects(&lsai, 0,
          LMI_ServiceAffectsIdentity_ElementEffects_Manages);

        KReturnInstance(cr, lsai);
        lu_ent_free(lue);
      } /* for accounts */

    if (accounts)
      {
        g_ptr_array_free(accounts, TRUE);
      }

    /* Go through every groups */
    accounts = lu_groups_enumerate_full(luc, "*", &error);
    for (i = 0;  (accounts != NULL) && (i < accounts->len); i++)
      {
        lue = g_ptr_array_index(accounts, i);

        LMI_IdentityRef_Init(&liref, _cb, nameSpace);
        asprintf(&id, ORGID":GID:%ld", aux_lu_get_long(lue, LU_GIDNUMBER));
        LMI_IdentityRef_Set_InstanceID(&liref, id);
        free(id);

        LMI_ServiceAffectsIdentity_Init(&lsai, _cb, nameSpace);
        LMI_ServiceAffectsIdentity_Set_AffectedElement(&lsai, &liref);
        LMI_ServiceAffectsIdentity_Set_AffectingElement(&lsai, &lamsref);
        LMI_ServiceAffectsIdentity_Init_ElementEffects(&lsai, 1);
        LMI_ServiceAffectsIdentity_Set_ElementEffects(&lsai, 0,
          LMI_ServiceAffectsIdentity_ElementEffects_Manages);

        KReturnInstance(cr, lsai);

        lu_ent_free(lue);
      } /* for accounts */

    if (accounts)
      {
        g_ptr_array_free(accounts, TRUE);
      }

    lu_end(luc);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ServiceAffectsIdentityGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ServiceAffectsIdentityCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ServiceAffectsIdentityModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ServiceAffectsIdentityDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ServiceAffectsIdentityExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ServiceAffectsIdentityAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ServiceAffectsIdentityAssociators(
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
        LMI_ServiceAffectsIdentity_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_ServiceAffectsIdentityAssociatorNames(
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
        LMI_ServiceAffectsIdentity_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_ServiceAffectsIdentityReferences(
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
        LMI_ServiceAffectsIdentity_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_ServiceAffectsIdentityReferenceNames(
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
        LMI_ServiceAffectsIdentity_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_ServiceAffectsIdentity,
    LMI_ServiceAffectsIdentity,
    _cb,
    LMI_ServiceAffectsIdentityInitialize(ctx))

CMAssociationMIStub( 
    LMI_ServiceAffectsIdentity,
    LMI_ServiceAffectsIdentity,
    _cb,
    LMI_ServiceAffectsIdentityInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ServiceAffectsIdentity",
    "LMI_ServiceAffectsIdentity",
    "instance association")
