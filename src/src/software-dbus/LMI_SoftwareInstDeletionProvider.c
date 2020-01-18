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
 * Authors: Michal Minar <miminar@redhat.com>
 */

#include <konkret/konkret.h>
#include "ind_sender.h"
#include "LMI_SoftwareInstDeletion.h"
#include "LMI_SoftwareInstallationJob.h"
#include "sw-utils.h"

static const CMPIBroker* _cb = NULL;

static void LMI_SoftwareInstDeletionInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SoftwareInstDeletion_ClassName,
            _cb, ctx, TRUE, provider_config_defaults);
}

static CMPIStatus LMI_SoftwareInstDeletionIndicationCleanup(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_SoftwareInstDeletion_ClassName);
}

static CMPIStatus LMI_SoftwareInstDeletionAuthorizeFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    const char* user)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};

    if (!ind_sender_authorize_filter(
            se, LMI_SoftwareInstallationJob_ClassName, op, user))
        KSetStatus2(_cb, &status, ERR_FAILED,
                "Failed to authorize filter!");
    return status;
}

static CMPIStatus LMI_SoftwareInstDeletionMustPoll(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareInstDeletionActivateFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    CMPIBoolean firstActivation)
{
    return ind_sender_activate_filter(
            se, LMI_SoftwareInstallationJob_ClassName, op, firstActivation);
}

static CMPIStatus LMI_SoftwareInstDeletionDeActivateFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    CMPIBoolean lastActivation)
{
    return ind_sender_deactivate_filter(
            se, LMI_SoftwareInstallationJob_ClassName, op, lastActivation);
}

static CMPIStatus LMI_SoftwareInstDeletionEnableIndications(
    CMPIIndicationMI* mi,
    const CMPIContext* cc)
{
    enable_indications();
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareInstDeletionDisableIndications(
    CMPIIndicationMI* mi,
    const CMPIContext* cc)
{
    disable_indications();
    CMReturn(CMPI_RC_OK);
}

CMIndicationMIStub(
    LMI_SoftwareInstDeletion,
    LMI_SoftwareInstDeletion,
    _cb,
    LMI_SoftwareInstDeletionInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SoftwareInstDeletion",
    "LMI_SoftwareInstDeletion",
    "indication")
