/*
 * Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
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
#include "LMI_AccountInstanceDeletionIndication.h"

#include <sys/inotify.h>
#include <unistd.h>

#include "ind_manager.h"
#include "indication_common.h"

#include "macros.h"
#include "account_globals.h"
#include "openlmi.h"

static const CMPIBroker* _cb = NULL;

static IMManager *im = NULL;
static IMError im_err = IM_ERR_OK;
static AccountIndication ai;

static bool deletion_watcher(void **data)
{
    return watcher(&ai, data);
}

static void LMI_AccountInstanceDeletionIndicationInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
    im = im_create_manager(NULL, filter_checker, true, deletion_watcher,
                           IM_IND_DELETION, _cb, &im_err);
}

static CMPIStatus LMI_AccountInstanceDeletionIndicationIndicationCleanup(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    if (!im_destroy_manager(im, cc, &im_err)) CMReturn(CMPI_RC_ERR_FAILED);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountInstanceDeletionIndicationAuthorizeFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    const char* user)
{
    if (!im_verify_filter(im, se, cc, &im_err) ) {
        CMReturn(CMPI_RC_ERR_INVALID_QUERY);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountInstanceDeletionIndicationMustPoll(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AccountInstanceDeletionIndicationActivateFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    CMPIBoolean firstActivation)
{
    if (!im_verify_filter(im, se, cc, &im_err)) {
        CMReturn(CMPI_RC_ERR_INVALID_QUERY);
    }
    if (!im_add_filter(im, (CMPISelectExp*)se, cc, &im_err)) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountInstanceDeletionIndicationDeActivateFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    CMPIBoolean lastActivation)
{
    if (!im_remove_filter(im, se, cc, &im_err)) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountInstanceDeletionIndicationEnableIndications(
    CMPIIndicationMI* mi,
    const CMPIContext* cc)
{
    if (!watcher_init(&ai) ||
        !im_start_ind(im, cc, &im_err)) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AccountInstanceDeletionIndicationDisableIndications(
    CMPIIndicationMI* mi,
    const CMPIContext* cc)
{
    watcher_destroy(&ai);
    if (!im_stop_ind(im, cc, &im_err)) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

CMIndicationMIStub(
    LMI_AccountInstanceDeletionIndication,
    LMI_AccountInstanceDeletionIndication,
    _cb,
    LMI_AccountInstanceDeletionIndicationInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AccountInstanceDeletionIndication",
    "LMI_AccountInstanceDeletionIndication",
    "indication")
