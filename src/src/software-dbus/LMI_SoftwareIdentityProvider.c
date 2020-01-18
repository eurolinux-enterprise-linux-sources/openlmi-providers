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
 * Authors: Peter Schiffer <pschiffe@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_SoftwareIdentity.h"
#include "sw-utils.h"

static const CMPIBroker* _cb = NULL;

static void LMI_SoftwareIdentityInitialize(const CMPIContext *ctx)
{
    software_init(LMI_SoftwareIdentity_ClassName,
            _cb, ctx, FALSE, provider_config_defaults);
}

static CMPIStatus LMI_SoftwareIdentityCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    return software_cleanup(LMI_SoftwareIdentity_ClassName);
}

static CMPIStatus LMI_SoftwareIdentityEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    char error_msg[BUFLEN] = "";

    enum_sw_identity_instance_names(0, _cb, cc, KNameSpace(cop), cr, error_msg,
            BUFLEN);

    if (*error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareIdentityEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_SoftwareIdentity w;
    PkPackage *pk_pkg = NULL;
    PkDetails *pk_det = NULL;
    SwPackage sw_pkg;
    short found = 0;

    init_sw_package(&sw_pkg);

    if (get_sw_pkg_from_sw_identity_op(cop, &sw_pkg) != 0) {
        goto done;
    }

    get_pk_pkg_from_sw_pkg(&sw_pkg, 0, &pk_pkg);
    if (!pk_pkg) {
        goto done;
    }

    get_pk_det_from_pk_pkg(pk_pkg, &pk_det, NULL);

    create_instance_from_pkgkit_data(pk_pkg, pk_det, &sw_pkg, _cb,
            KNameSpace(cop), &w);

    KReturnInstance(cr, w);

    found = 1;

done:
    clean_sw_package(&sw_pkg);

    if (pk_det) {
        g_object_unref(pk_det);
    }
    if (pk_pkg) {
        g_object_unref(pk_pkg);
    }

    if (!found) {
        CMReturn(CMPI_RC_ERR_NOT_FOUND);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareIdentityCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SoftwareIdentityExecQuery(
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
    LMI_SoftwareIdentity,
    LMI_SoftwareIdentity,
    _cb,
    LMI_SoftwareIdentityInitialize(ctx))

static CMPIStatus LMI_SoftwareIdentityMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SoftwareIdentityInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_SoftwareIdentity_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_SoftwareIdentity,
    LMI_SoftwareIdentity,
    _cb,
    LMI_SoftwareIdentityInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SoftwareIdentity",
    "LMI_SoftwareIdentity",
    "instance method")
