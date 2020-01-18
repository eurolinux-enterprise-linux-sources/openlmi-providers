/*
 * Copyright (C) 2013-2014 Red Hat, Inc.  All rights reserved.
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
 * Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
*/

#include <konkret/konkret.h>
#include "LMI_ServiceInstanceModificationIndication.h"

#include "ind_manager.h"
#include "util/serviceutil.h"

static const CMPIBroker* _cb = NULL;

static const char* service_allowed_classes[] = {
  LMI_Service_ClassName,
  NULL};

static IMManager *im = NULL;
static IMError im_err = IM_ERR_OK;
static ServiceIndication si;

static void LMI_ServiceInstanceModificationIndicationInitialize()
{
    im = im_create_manager(NULL, NULL, true, ind_watcher,
                           IM_IND_MODIFICATION, _cb, &im_err);
    im_register_filter_classes(im, &service_allowed_classes[0], &im_err);
}

static CMPIStatus LMI_ServiceInstanceModificationIndicationIndicationCleanup(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    if (!im_destroy_manager(im, cc, &im_err))
        CMReturn(CMPI_RC_ERR_FAILED);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ServiceInstanceModificationIndicationAuthorizeFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    const char* user)
{
    if (!im_verify_filter(im, se, cc, &im_err) )
        CMReturn(CMPI_RC_ERR_INVALID_QUERY);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ServiceInstanceModificationIndicationMustPoll(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ServiceInstanceModificationIndicationActivateFilter(
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

static CMPIStatus LMI_ServiceInstanceModificationIndicationDeActivateFilter(
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

static CMPIStatus LMI_ServiceInstanceModificationIndicationEnableIndications(
    CMPIIndicationMI* mi,
    const CMPIContext* cc)
{
    char output[1024];

    if (ind_init(&si, output, sizeof(output)) != 0) {
        lmi_debug("ind_init failed: %s", output);
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    if (!im_start_ind(im, cc, &im_err))
        CMReturn(CMPI_RC_ERR_FAILED);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ServiceInstanceModificationIndicationDisableIndications(
    CMPIIndicationMI* mi,
    const CMPIContext* cc)
{
    if (!im_stop_ind(im, cc, &im_err))
        CMReturn(CMPI_RC_ERR_FAILED);
    ind_destroy(&si);
    CMReturn(CMPI_RC_OK);
}

CMIndicationMIStub(
    LMI_ServiceInstanceModificationIndication,
    LMI_ServiceInstanceModificationIndication,
    _cb,
    LMI_ServiceInstanceModificationIndicationInitialize())

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ServiceInstanceModificationIndication",
    "LMI_ServiceInstanceModificationIndication",
    "indication")
