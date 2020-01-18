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
#include "LMI_AccountOnSystem.h"
#include "LMI_Account.h"
#include "CIM_ComputerSystem.h"

#include <glib.h>

#include <libuser/entity.h>
#include <libuser/user.h>

#include "aux_lu.h"
#include "macros.h"
#include "account_globals.h"
#include "globals.h"

static const CMPIBroker* _cb;

static void LMI_AccountOnSystemInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_AccountOnSystemCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountOnSystemEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_AccountOnSystemEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_AccountRef laref;
    LMI_AccountOnSystem laos;

    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    GPtrArray *accounts = NULL;
    struct lu_ent *lue = NULL;

    size_t i;
    const char *nameSpace = KNameSpace(cop);
    const char *hostname = get_system_name();

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

        LMI_AccountRef_Init(&laref, _cb, nameSpace);
        LMI_AccountRef_Set_SystemCreationClassName(&laref, get_system_creation_class_name());
        LMI_AccountRef_Set_SystemName(&laref, hostname);
        LMI_AccountRef_Set_CreationClassName(&laref,
          LMI_Account_ClassName);
        LMI_AccountRef_Set_Name(&laref, aux_lu_get_str(lue, LU_USERNAME));

        LMI_AccountOnSystem_Init(&laos, _cb, nameSpace);
        KPrintObjectPath(stderr, lmi_get_computer_system(), 1);
        LMI_AccountOnSystem_SetObjectPath_GroupComponent(&laos,
          lmi_get_computer_system());
        LMI_AccountOnSystem_Set_PartComponent(&laos, &laref);

        KReturnInstance(cr, laos);
        lu_ent_free(lue);
      } /* for */

    if (accounts)
      {
        g_ptr_array_free(accounts, TRUE);
      }

    lu_end(luc);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountOnSystemGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_AccountOnSystemCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountOnSystemModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountOnSystemDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountOnSystemExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountOnSystemAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountOnSystemAssociators(
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
        LMI_AccountOnSystem_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_AccountOnSystemAssociatorNames(
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
        LMI_AccountOnSystem_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_AccountOnSystemReferences(
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
        LMI_AccountOnSystem_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_AccountOnSystemReferenceNames(
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
        LMI_AccountOnSystem_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_AccountOnSystem,
    LMI_AccountOnSystem,
    _cb,
    LMI_AccountOnSystemInitialize(ctx))

CMAssociationMIStub( 
    LMI_AccountOnSystem,
    LMI_AccountOnSystem,
    _cb,
    LMI_AccountOnSystemInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AccountOnSystem",
    "LMI_AccountOnSystem",
    "instance association")
