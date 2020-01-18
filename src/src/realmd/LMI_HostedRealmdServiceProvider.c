#include <konkret/konkret.h>
#include "LMI_HostedRealmdService.h"
#include "CIM_ComputerSystem.h"
#include "rdcp_util.h"

static const CMPIBroker* _cb;

static void LMI_HostedRealmdServiceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_HostedRealmdServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedRealmdServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_HostedRealmdServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus status;
    LMI_RealmdServiceRef realmd_service_ref;
    LMI_HostedRealmdService hosted_realmd_service;

    const char *name_space = KNameSpace(cop);
    const char *host_name = lmi_get_system_name_safe(cc);

    CMSetStatus(&status, CMPI_RC_OK);

    LMI_InitRealmdServiceKeys(LMI_RealmdServiceRef, &realmd_service_ref, name_space, host_name);

    LMI_HostedRealmdService_Init(&hosted_realmd_service, _cb, name_space);
    LMI_HostedRealmdService_SetObjectPath_Antecedent(
            &hosted_realmd_service,
            lmi_get_computer_system_safe(cc));
    LMI_HostedRealmdService_Set_Dependent(&hosted_realmd_service,
                                          &realmd_service_ref);

    KReturnInstance(cr, hosted_realmd_service);

    return status;
}

static CMPIStatus LMI_HostedRealmdServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_HostedRealmdServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedRealmdServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedRealmdServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedRealmdServiceExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedRealmdServiceAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedRealmdServiceAssociators(
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
        LMI_HostedRealmdService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_HostedRealmdServiceAssociatorNames(
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
        LMI_HostedRealmdService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_HostedRealmdServiceReferences(
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
        LMI_HostedRealmdService_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_HostedRealmdServiceReferenceNames(
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
        LMI_HostedRealmdService_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_HostedRealmdService,
    LMI_HostedRealmdService,
    _cb,
    LMI_HostedRealmdServiceInitialize(ctx))

CMAssociationMIStub(
    LMI_HostedRealmdService,
    LMI_HostedRealmdService,
    _cb,
    LMI_HostedRealmdServiceInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_HostedRealmdService",
    "LMI_HostedRealmdService",
    "instance association")
