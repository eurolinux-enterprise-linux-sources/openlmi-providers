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
 * Authors: Tomas Bzatek <tbzatek@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_JournalLogRecordInstanceCreationIndication.h"

#include "ind_manager.h"
#include "instutil.h"
#include "journal.h"
#include <globals.h>

static const CMPIBroker* _cb = NULL;

static IMManager *im = NULL;
static IMError im_err = IM_ERR_OK;

static void LMI_JournalLogRecordInstanceCreationIndicationInitialize(const CMPIContext *ctx)
{
    lmi_init(JOURNAL_CIM_LOG_NAME, _cb, ctx, provider_config_defaults);
    im = im_create_manager(ind_gather, NULL, false, ind_watcher,
                           IM_IND_CREATION, _cb, &im_err);
    im_register_filter_classes(im, &journald_allowed_classes[0], &im_err);
}

static CMPIStatus LMI_JournalLogRecordInstanceCreationIndicationIndicationCleanup(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    if (! im_destroy_manager(im, cc, &im_err))
        CMReturn(CMPI_RC_ERR_FAILED);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalLogRecordInstanceCreationIndicationAuthorizeFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    const char* user)
{
    if (!im_verify_filter(im, se, cc, &im_err))
        CMReturn(CMPI_RC_ERR_INVALID_QUERY);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalLogRecordInstanceCreationIndicationMustPoll(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_JournalLogRecordInstanceCreationIndicationActivateFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    CMPIBoolean firstActivation)
{
    if (!im_verify_filter(im, se, cc, &im_err))
        CMReturn(CMPI_RC_ERR_INVALID_QUERY);
    if (!im_add_filter(im, (CMPISelectExp*)se, cc, &im_err))
        CMReturn(CMPI_RC_ERR_FAILED);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalLogRecordInstanceCreationIndicationDeActivateFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    CMPIBoolean lastActivation)
{
    if (!im_remove_filter(im, se, cc, &im_err))
        CMReturn(CMPI_RC_ERR_FAILED);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalLogRecordInstanceCreationIndicationEnableIndications(
    CMPIIndicationMI* mi,
    const CMPIContext* cc)
{
    ind_init();
    if (!im_start_ind(im, cc, &im_err)) {
        ind_destroy();
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalLogRecordInstanceCreationIndicationDisableIndications(
    CMPIIndicationMI* mi,
    const CMPIContext* cc)
{
    if (!im_stop_ind(im, cc, &im_err))
        CMReturn(CMPI_RC_ERR_FAILED);
    ind_destroy();
    CMReturn(CMPI_RC_OK);
}

CMIndicationMIStub(
    LMI_JournalLogRecordInstanceCreationIndication,
    LMI_JournalLogRecordInstanceCreationIndication,
    _cb,
    LMI_JournalLogRecordInstanceCreationIndicationInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_JournalLogRecordInstanceCreationIndication",
    "LMI_JournalLogRecordInstanceCreationIndication",
    "indication")
