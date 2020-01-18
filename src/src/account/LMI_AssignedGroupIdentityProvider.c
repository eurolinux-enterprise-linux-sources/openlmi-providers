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

#include <konkret/konkret.h>
#include "LMI_AssignedGroupIdentity.h"
#include "LMI_Identity.h"
#include "LMI_Group.h"

#include "aux_lu.h"
#include "macros.h"
#include "account_globals.h"

#include <libuser/entity.h>
#include <libuser/user.h>

static const CMPIBroker* _cb;

static void LMI_AssignedGroupIdentityInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_AssignedGroupIdentityCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssignedGroupIdentityEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_AssignedGroupIdentityEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_IdentityRef liref;
    LMI_GroupRef lgref;
    LMI_AssignedGroupIdentity lagi;

    struct lu_context *luc = NULL;
    struct lu_error *error = NULL;
    GPtrArray *groups = NULL;
    struct lu_ent *lue = NULL;

    size_t i;
    const char *nameSpace = KNameSpace(cop);
    char *gid = NULL;

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

        LMI_GroupRef_Init(&lgref, _cb, nameSpace);
        LMI_GroupRef_Set_CreationClassName(&lgref, LMI_Group_ClassName);
        LMI_GroupRef_Set_Name(&lgref, aux_lu_get_str(lue, LU_GROUPNAME));

        LMI_IdentityRef_Init(&liref, _cb, nameSpace);
        asprintf(&gid, LMI_ORGID":GID:%ld", aux_lu_get_long(lue, LU_GIDNUMBER));
        LMI_IdentityRef_Set_InstanceID(&liref, gid);
        free(gid);

        LMI_AssignedGroupIdentity_Init(&lagi, _cb, nameSpace);
        LMI_AssignedGroupIdentity_Set_IdentityInfo(&lagi, &liref);
        LMI_AssignedGroupIdentity_Set_ManagedElement(&lagi, &lgref);

        KReturnInstance(cr, lagi);
        lu_ent_free(lue);
      } /* for */
    if (groups)
        g_ptr_array_free(groups, TRUE);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssignedGroupIdentityGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_AssignedGroupIdentityCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssignedGroupIdentityModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssignedGroupIdentityDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssignedGroupIdentityExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssignedGroupIdentityAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssignedGroupIdentityAssociators(
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
        LMI_AssignedGroupIdentity_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_AssignedGroupIdentityAssociatorNames(
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
        LMI_AssignedGroupIdentity_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_AssignedGroupIdentityReferences(
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
        LMI_AssignedGroupIdentity_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_AssignedGroupIdentityReferenceNames(
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
        LMI_AssignedGroupIdentity_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_AssignedGroupIdentity,
    LMI_AssignedGroupIdentity,
    _cb,
    LMI_AssignedGroupIdentityInitialize(ctx))

CMAssociationMIStub(
    LMI_AssignedGroupIdentity,
    LMI_AssignedGroupIdentity,
    _cb,
    LMI_AssignedGroupIdentityInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AssignedGroupIdentity",
    "LMI_AssignedGroupIdentity",
    "instance association")
