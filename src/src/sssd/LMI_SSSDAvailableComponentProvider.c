/*
 * Copyright (C) 2014 Red Hat, Inc.  All rights reserved.
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
 * Authors: Pavel Březina <pbrezina@redhat.com>
 */

#include <konkret/konkret.h>
#include <string.h>
#include <sss_sifp_dbus.h>
#include "LMI_SSSDAvailableComponent.h"
#include "LMI_SSSDMonitor.h"
#include "LMI_SSSDResponder.h"
#include "LMI_SSSDBackend.h"
#include "utils.h"

static const CMPIBroker* _cb;

static void LMI_SSSDAvailableComponentInitialize(const CMPIContext *ctx)
{
    lmi_init(PROVIDER_NAME, _cb, ctx, NULL);
}

static CMPIStatus LMI_SSSDAvailableComponentCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDAvailableComponentEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SSSDAvailableComponentEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    const char *namespace = KNameSpace(cop);
    LMI_SSSDAvailableComponent association;
    LMI_SSSDServiceRef ref_sssd;
    LMI_SSSDMonitorRef ref_monitor;
    LMI_SSSDResponderRef ref_responder;
    LMI_SSSDBackendRef ref_backend;
    CMPIObjectPath *op;
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_attr **attrs = NULL;
    sss_sifp_error error;
    char **paths = NULL;
    const char *name = NULL;
    const char *type = NULL;
    CMPIStatus cmpi_ret;
    CMPIrc ret;
    int i;

    LMI_SSSDService_Get_Ref(cc, _cb, namespace, &ref_sssd);

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    error = sss_sifp_invoke_list(sifp_ctx, SSSD_DBUS_LIST_COMPONENTS, &paths,
                                 DBUS_TYPE_INVALID);
    check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

    for (i = 0; paths[i] != NULL; i++) {
        error = sss_sifp_fetch_all_attrs(sifp_ctx, paths[i],
                                         SSS_SIFP_IFACE_COMPONENTS, &attrs);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        error = sss_sifp_find_attr_as_string(attrs, "name", &name);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        error = sss_sifp_find_attr_as_string(attrs, "type", &type);
        check_sss_sifp_error(error, ret, CMPI_RC_ERR_FAILED, done);

        if (strcmp(type, "monitor") == 0) {
            LMI_SSSDMonitorRef_Init(&ref_monitor, _cb, namespace);
            LMI_SSSDMonitorRef_Set_Name(&ref_monitor, name);
            op = LMI_SSSDMonitorRef_ToObjectPath(&ref_monitor, &cmpi_ret);
        } else if (strcmp(type, "responder") == 0) {
            LMI_SSSDResponderRef_Init(&ref_responder, _cb, namespace);
            LMI_SSSDResponderRef_Set_Name(&ref_responder, name);
            op = LMI_SSSDResponderRef_ToObjectPath(&ref_responder, &cmpi_ret);
        } else if (strcmp(type, "backend") == 0) {
            LMI_SSSDBackendRef_Init(&ref_backend, _cb, namespace);
            LMI_SSSDBackendRef_Set_Name(&ref_backend, name);
            op = LMI_SSSDBackendRef_ToObjectPath(&ref_backend, &cmpi_ret);
        } else {
            /* unknown type */
            continue;
        }

        if (!KOkay(cmpi_ret)) {
            ret = CMPI_RC_ERR_FAILED;
            goto done;
        }

        LMI_SSSDAvailableComponent_Init(&association, _cb, namespace);
        LMI_SSSDAvailableComponent_Set_SSSD(&association, &ref_sssd);
        LMI_SSSDAvailableComponent_SetObjectPath_Component(&association, op);

        KReturnInstance(cr, association);

        sss_sifp_free_attrs(sifp_ctx, &attrs);
    }

    ret = CMPI_RC_OK;

done:
    sss_sifp_free_string_array(sifp_ctx, &paths);
    sss_sifp_free_attrs(sifp_ctx, &attrs);
    sss_sifp_free(&sifp_ctx);
    CMReturn(ret);
}

static CMPIStatus LMI_SSSDAvailableComponentGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SSSDAvailableComponentCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDAvailableComponentModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDAvailableComponentDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDAvailableComponentExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SSSDAvailableComponentAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SSSDAvailableComponentAssociators(
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
        LMI_SSSDAvailableComponent_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_SSSDAvailableComponentAssociatorNames(
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
        LMI_SSSDAvailableComponent_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_SSSDAvailableComponentReferences(
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
        LMI_SSSDAvailableComponent_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_SSSDAvailableComponentReferenceNames(
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
        LMI_SSSDAvailableComponent_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_SSSDAvailableComponent,
    LMI_SSSDAvailableComponent,
    _cb,
    LMI_SSSDAvailableComponentInitialize(ctx))

CMAssociationMIStub( 
    LMI_SSSDAvailableComponent,
    LMI_SSSDAvailableComponent,
    _cb,
    LMI_SSSDAvailableComponentInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SSSDAvailableComponent",
    "LMI_SSSDAvailableComponent",
    "instance association")
