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
 * Authors: Jan Synacek <jsynacek@redhat.com>
 */
#include <konkret/konkret.h>
#include "LMI_FIFOPipeFile.h"
#include "file.h"

static const CMPIBroker* _cb = NULL;

static void LMI_FIFOPipeFileInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_FIFOPipeFileCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FIFOPipeFileEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FIFOPipeFileEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FIFOPipeFileGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus st = {.rc = CMPI_RC_OK};
    logicalfile_t logicalfile;

    st = lmi_check_required_properties(_cb, cc, cop, "CSCreationClassName", "CSName");
    lmi_return_if_status_not_ok(st);

    LMI_FIFOPipeFile_InitFromObjectPath(&logicalfile.lf.fifopipefile, _cb, cop);
    st = stat_logicalfile_and_fill(_cb, &logicalfile, S_IFIFO, "No such FIFO: %s");
    lmi_return_if_status_not_ok(st);

    KReturnInstance(cr, logicalfile.lf.fifopipefile);
    return st;

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FIFOPipeFileCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FIFOPipeFileModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FIFOPipeFileDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_FIFOPipeFileExecQuery(
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
    LMI_FIFOPipeFile,
    LMI_FIFOPipeFile,
    _cb,
    LMI_FIFOPipeFileInitialize(ctx))

static CMPIStatus LMI_FIFOPipeFileMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_FIFOPipeFileInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_FIFOPipeFile_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_FIFOPipeFile,
    LMI_FIFOPipeFile,
    _cb,
    LMI_FIFOPipeFileInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_FIFOPipeFile",
    "LMI_FIFOPipeFile",
    "instance method")
/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* End: */
