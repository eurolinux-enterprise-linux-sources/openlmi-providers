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
 * Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
 *          Radek Novacek <rnovacek@redhat.com>
 */

#include <konkret/konkret.h>
#include <stdint.h>
#include "LMI_Service.h"
#include "util/serviceutil.h"
#include "globals.h"

static const CMPIBroker* _cb = NULL;

static void LMI_ServiceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_ServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    const char *ns = KNameSpace(cop);
    SList *slist = NULL;
    char output[1024];

    slist = service_find_all(output, sizeof(output));
    if (slist == NULL) {
        KReturn2(_cb, ERR_FAILED, "%s", output);
    }

    for (int i = 0; i < slist->cnt; i++) {
        LMI_ServiceRef w;
        LMI_ServiceRef_Init(&w, _cb, ns);
        LMI_ServiceRef_Set_CreationClassName(&w, LMI_Service_ClassName);
        LMI_ServiceRef_Set_SystemCreationClassName(&w, get_system_creation_class_name());
        LMI_ServiceRef_Set_SystemName(&w, get_system_name());
        LMI_ServiceRef_Set_Name(&w, slist->name[i]);

        CMReturnObjectPath(cr, LMI_ServiceRef_ToObjectPath(&w, NULL));
    }
    service_free_slist(slist);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus st;
    CMPIEnumeration* e;
    if (!(e = _cb->bft->enumerateInstanceNames(_cb, cc, cop, &st))) {
        KReturn2(_cb, ERR_FAILED, "Unable to enumerate instances of LMI_Service");
    }
    CMPIData cd;
    while (CMHasNext(e, &st)) {

        cd = CMGetNext(e, &st);
        if (st.rc || cd.type != CMPI_ref) {
            KReturn2(_cb, ERR_FAILED, "Enumerate instances didn't returned list of references");
        }
        CMPIInstance *in = _cb->bft->getInstance(_cb, cc, cd.value.ref, properties, &st);
        if (st.rc) {
            KReturn2(_cb, ERR_FAILED, "Unable to get instance of LMI_Service");
        }
        cr->ft->returnInstance(cr, in);
    }
    KReturn(OK);
}

static CMPIStatus LMI_ServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    char output[1024];
    int res;
    LMI_Service w;
    LMI_Service_InitFromObjectPath(&w, _cb, cop);

    Service servicebuf = {0, };
    if ((res = service_get_properties(&servicebuf, w.Name.chars, output, sizeof(output))) == 0) {
        LMI_Service_Set_Status(&w, servicebuf.svStatus);
        free(servicebuf.svStatus);
        LMI_Service_Set_Started(&w, servicebuf.svStarted);
        LMI_Service_Set_Caption(&w, servicebuf.svCaption);
        free(servicebuf.svCaption);
        LMI_Service_Init_OperationalStatus(&w, servicebuf.svOperationalStatusCnt);
        for (int i = 0; i < servicebuf.svOperationalStatusCnt; i++) {
            LMI_Service_Set_OperationalStatus(&w, i, servicebuf.svOperationalStatus[i]);
        }

        switch (servicebuf.svEnabledDefault) {
            case ENABLED:
                LMI_Service_Set_EnabledDefault(&w, LMI_Service_EnabledDefault_Enabled);
                break;
            case DISABLED:
                LMI_Service_Set_EnabledDefault(&w, LMI_Service_EnabledDefault_Disabled);
                break;
            default:
                LMI_Service_Set_EnabledDefault(&w, LMI_Service_EnabledDefault_Not_Applicable);
                break;
        }

        KReturnInstance(cr, w);
        KReturn(OK);
    } else if (res == -2) { /* service of that name doesn't exist */
        KReturn(ERR_NOT_FOUND);
    } else { /* some error occured when getting properties */
        KReturn2(_cb, ERR_FAILED, "%s", output);
    }
}

static CMPIStatus LMI_ServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ServiceExecQuery(
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
    LMI_Service,
    LMI_Service,
    _cb,
    LMI_ServiceInitialize(ctx))

static CMPIStatus LMI_ServiceMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ServiceInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Service_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_Service,
    LMI_Service,
    _cb,
    LMI_ServiceInitialize(ctx))

KUint32 LMI_Service_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

unsigned int Service_RunOperation(const char *service, const char *operation, CMPIStatus *status)
{
    char output[1024];
    int res = service_operation(service, operation, output, sizeof(output));
    if (res == 0) {
        KSetStatus2(_cb, status, OK, output);
    } else {
        KSetStatus2(_cb, status, ERR_FAILED, output);
    }
    return res;
}

KUint32 LMI_Service_StartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, Service_RunOperation(self->Name.chars, "StartUnit", status));
    return result;
}

KUint32 LMI_Service_StopService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, Service_RunOperation(self->Name.chars, "StopUnit", status));
    return result;
}

KUint32 LMI_Service_ChangeAffectedElementsAssignedSequence(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    const KRefA* ManagedElements,
    const KUint16A* AssignedSequence,
    KRef* Job,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Service_ReloadService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, Service_RunOperation(self->Name.chars, "ReloadUnit", status));
    return result;
}

KUint32 LMI_Service_RestartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, Service_RunOperation(self->Name.chars, "RestartUnit", status));
    return result;
}

KUint32 LMI_Service_TryRestartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, Service_RunOperation(self->Name.chars, "TryRestartUnit", status));
    return result;
}

KUint32 LMI_Service_CondRestartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, Service_RunOperation(self->Name.chars, "TryRestartUnit", status));
    return result;
}

KUint32 LMI_Service_ReloadOrRestartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, Service_RunOperation(self->Name.chars, "ReloadOrRestartUnit", status));
    return result;
}

KUint32 LMI_Service_ReloadOrTryRestartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, Service_RunOperation(self->Name.chars, "ReloadOrTryRestartUnit", status));
    return result;
}

KUint32 LMI_Service_TurnServiceOn(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, Service_RunOperation(self->Name.chars, "EnableUnitFiles", status));
    return result;
}

KUint32 LMI_Service_TurnServiceOff(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    KUint32_Set(&result, Service_RunOperation(self->Name.chars, "DisableUnitFiles", status));
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_Service",
    "LMI_Service",
    "instance method")
