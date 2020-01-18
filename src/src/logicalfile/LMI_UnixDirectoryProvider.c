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
#include "LMI_UnixDirectory.h"
#include "file.h"

static const CMPIBroker* _cb = NULL;

static void LMI_UnixDirectoryInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_UnixDirectoryCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_UnixDirectoryEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_UnixDirectoryEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_UnixDirectoryGetInstance(
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

    LMI_UnixDirectory_InitFromObjectPath(&logicalfile.lf.unixdirectory, _cb, cop);
    st = stat_logicalfile_and_fill(_cb, &logicalfile, S_IFDIR, "Not a directory: %s");
    lmi_return_if_status_not_ok(st);

    KReturnInstance(cr, logicalfile.lf.unixdirectory);
    return st;
}

static CMPIStatus LMI_UnixDirectoryCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMPIStatus st;
    CMPIObjectPath *iop = CMGetObjectPath(ci, &st);
    lmi_return_if_status_not_ok(st);
    st = lmi_check_required_properties(_cb, cc, iop, "CSCreationClassName", "CSName");
    lmi_return_if_status_not_ok(st);

    const char *path = lmi_get_string_property_from_objectpath(iop, "Name");
    bool allow = lmi_read_config_boolean("LMI_UnixDirectory", "AllowMkdir");

    if (allow && mkdir(path, 0777) < 0) {
        char errmsg[BUFLEN];
        char strerr[BUFLEN];
        snprintf(errmsg, BUFLEN, "Can't mkdir: %s (%s)",
                 path,
                 strerror_r(errno, strerr, BUFLEN));
        CMReturnWithChars(_cb, CMPI_RC_ERR_FAILED, errmsg);
    }
    if (allow == false) {
        CMReturnWithChars(_cb, CMPI_RC_ERR_FAILED,
                          "Can't mkdir: disabled by provider configuration");
    }

    return CMReturnObjectPath(cr, iop);
}

static CMPIStatus LMI_UnixDirectoryModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_UnixDirectoryDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    const char *path = lmi_get_string_property_from_objectpath(cop, "Name");
    bool allow = lmi_read_config_boolean("LMI_UnixDirectory", "AllowRmdir");

    if (allow && rmdir(path) < 0) {
        char errmsg[BUFLEN];
        char strerr[BUFLEN];
        snprintf(errmsg, BUFLEN, "Can't rmdir: %s (%s)",
                 path,
                 strerror_r(errno, strerr, BUFLEN));
        CMReturnWithChars(_cb, CMPI_RC_ERR_FAILED, errmsg);
    }
    if (allow == false) {
        CMReturnWithChars(_cb, CMPI_RC_ERR_FAILED,
                          "Can't rmdir: disabled by provider configuration");
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_UnixDirectoryExecQuery(
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
    LMI_UnixDirectory,
    LMI_UnixDirectory,
    _cb,
    LMI_UnixDirectoryInitialize(ctx))

static CMPIStatus LMI_UnixDirectoryMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_UnixDirectoryInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_UnixDirectory_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_UnixDirectory,
    LMI_UnixDirectory,
    _cb,
    LMI_UnixDirectoryInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_UnixDirectory",
    "LMI_UnixDirectory",
    "instance method")
/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4 */
/* End: */
