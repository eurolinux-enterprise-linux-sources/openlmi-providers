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
#include "LMI_AccountManagementCapabilities.h"

#include <stdbool.h>

#include "macros.h"
#include "globals.h"
#include "account_globals.h"

#define NAME LAMCNAME

static const CMPIBroker* _cb = NULL;

static void LMI_AccountManagementCapabilitiesInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_AccountManagementCapabilitiesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountManagementCapabilitiesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_AccountManagementCapabilitiesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_AccountManagementCapabilities lamc;
    int size = 0; /* How many encrypt algorithms we have */
    int i = 0;

    LMI_AccountManagementCapabilities_Init(&lamc, _cb, KNameSpace(cop));
    LMI_AccountManagementCapabilities_Set_ElementNameEditSupported(
      &lamc, false);
    LMI_AccountManagementCapabilities_Set_InstanceID(&lamc, ORGID":"LAMCNAME);
    LMI_AccountManagementCapabilities_Set_ElementName(&lamc, NAME);

    LMI_AccountManagementCapabilities_Init_OperationsSupported(&lamc, 3);
    LMI_AccountManagementCapabilities_Set_OperationsSupported(&lamc, 0,
      LMI_AccountManagementCapabilities_OperationsSupported_Create);
    LMI_AccountManagementCapabilities_Set_OperationsSupported(&lamc, 1,
      LMI_AccountManagementCapabilities_OperationsSupported_Modify);
    LMI_AccountManagementCapabilities_Set_OperationsSupported(&lamc, 2,
      LMI_AccountManagementCapabilities_OperationsSupported_Delete);

    LMI_AccountManagementCapabilities_Init_SupportedUserPasswordEncryptionAlgorithms(&lamc, 1);
    LMI_AccountManagementCapabilities_Set_SupportedUserPasswordEncryptionAlgorithms(&lamc, 0, LMI_AccountManagementCapabilities_SupportedUserPasswordEncryptionAlgorithms_Other);

    /* Count how many ecrypt algorithms we have */
    while(crypt_algs[size]) size++;

    /* Create a list of other supported password encrypt algorithms */
    if (size > 0)
      {
        LMI_AccountManagementCapabilities_Init_OtherSupportedUserPasswordEncryptionAlgorithms(&lamc, size);
        for (i = 0; i < size; i++)
          {
            LMI_AccountManagementCapabilities_Set_OtherSupportedUserPasswordEncryptionAlgorithms(&lamc, i, crypt_algs[i]);
          }
      }

    KReturnInstance(cr, lamc);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountManagementCapabilitiesGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_AccountManagementCapabilitiesCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountManagementCapabilitiesModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountManagementCapabilitiesDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountManagementCapabilitiesExecQuery(
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
    LMI_AccountManagementCapabilities,
    LMI_AccountManagementCapabilities,
    _cb,
    LMI_AccountManagementCapabilitiesInitialize(ctx))

static CMPIStatus LMI_AccountManagementCapabilitiesMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountManagementCapabilitiesInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_AccountManagementCapabilities_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

KUint16 LMI_AccountManagementCapabilities_CreateGoalSettings(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_AccountManagementCapabilitiesRef* self,
    const KInstanceA* TemplateGoalSettings,
    KInstanceA* SupportedGoalSettings,
    CMPIStatus* status)
{
    KUint16 result = KUINT16_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

CMMethodMIStub(
    LMI_AccountManagementCapabilities,
    LMI_AccountManagementCapabilities,
    _cb,
    LMI_AccountManagementCapabilitiesInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AccountManagementCapabilities",
    "LMI_AccountManagementCapabilities",
    "instance method")
