#include <konkret/konkret.h>
#include "LMI_RealmdService.h"
#include "rdcp_util.h"
#include "globals.h"
#include "rdcp_error.h"
#include "rdcp_dbus.h"

static const CMPIBroker* _cb = NULL;

/**
 * get_joined_domain:
 *
 * @provider_props Realmd service provider properties
 *
 * Determine if the host is joined to a domain and if so return the domain name.
 *
 * Returns: domain name if found, NULL otherwise. Must be freed with g_free.
 */
static gchar *
get_joined_domain(GVariant *provider_props)
{
    CMPIStatus status;
    GError *g_error = NULL;
    GVariant *realm_props = NULL;
    GVariant *kerberos_props = NULL;
    GVariantIter *iter = NULL;
    gchar *realm_obj_path = NULL;
    gchar *configured_interface = NULL;
    gchar *domain_name = NULL;

    CMSetStatus(&status, CMPI_RC_OK);

    if (!g_variant_lookup(provider_props, "Realms", "ao", &iter))
        goto exit;

    while (g_variant_iter_next(iter, "&o", &realm_obj_path)) {
        GET_DBUS_PROPERIES_OR_EXIT(realm_props, realm_obj_path,
                                   REALM_DBUS_REALM_INTERFACE, &status);
        if (g_variant_lookup(realm_props, "Configured", "&s", &configured_interface)) {
            if (strlen(configured_interface)) {
                if (strcmp(configured_interface, REALM_DBUS_KERBEROS_MEMBERSHIP_INTERFACE) == 0) {
                    GET_DBUS_PROPERIES_OR_EXIT(kerberos_props, realm_obj_path,
                                               REALM_DBUS_KERBEROS_INTERFACE, &status);
                    if (g_variant_lookup(kerberos_props, "DomainName", "&s", &domain_name)) {
                        goto exit;
                    }
                    G_VARIANT_FREE(kerberos_props);
                }
            }
        }
        G_VARIANT_FREE(realm_props);
    }

 exit:
    G_VARIANT_ITER_FREE(iter);
    G_VARIANT_FREE(realm_props);
    G_VARIANT_FREE(kerberos_props);

    return domain_name ? g_strdup(domain_name) : NULL;
}



static void LMI_RealmdServiceInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_RealmdServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_RealmdServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_RealmdServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus status;
    GError *g_error = NULL;
    LMI_RealmdService lmi_realmd_service;
    const char *name_space = KNameSpace(cop);
    const char *host_name = get_system_name();
    GVariant *provider_props = NULL;
    gchar *joined_domain = NULL;

    CMSetStatus(&status, CMPI_RC_OK);

    if (!rdcp_dbus_initialize(&g_error)) {
        return handle_g_error(&g_error, _cb, &status, CMPI_RC_ERR_FAILED, "rdcp_dbus_initialize failed");
    }

    LMI_InitRealmdServiceKeys(LMI_RealmdService, &lmi_realmd_service, name_space, host_name);

    GET_DBUS_PROPERIES_OR_EXIT(provider_props, REALM_DBUS_SERVICE_PATH,
                               REALM_DBUS_PROVIDER_INTERFACE, &status);

    if ((joined_domain = get_joined_domain(provider_props))) {
        LMI_RealmdService_Set_Domain(&lmi_realmd_service, joined_domain);
    }

    KReturnInstance(cr, lmi_realmd_service);

 exit:
    G_VARIANT_FREE(provider_props);
    g_free(joined_domain);

    return status;
}

static CMPIStatus LMI_RealmdServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_RealmdServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_RealmdServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_RealmdServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_RealmdServiceExecQuery(
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
    LMI_RealmdService,
    LMI_RealmdService,
    _cb,
    LMI_RealmdServiceInitialize(ctx))

static CMPIStatus LMI_RealmdServiceMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_RealmdServiceInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_RealmdService_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_RealmdService,
    LMI_RealmdService,
    _cb,
    LMI_RealmdServiceInitialize(ctx))

KUint32 LMI_RealmdService_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_RealmdServiceRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_RealmdService_StartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_RealmdServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_RealmdService_StopService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_RealmdServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_RealmdService_ChangeAffectedElementsAssignedSequence(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_RealmdServiceRef* self,
    const KRefA* ManagedElements,
    const KUint16A* AssignedSequence,
    KRef* Job,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

// FIXME
static gboolean
get_credential_supported_owner(GVariant *supported, const gchar *cred_type, const gchar **cred_owner_return)
{
    GVariantIter iter;
    const gchar *type;
    const gchar *owner;

    g_variant_iter_init (&iter, supported);
    while (g_variant_iter_loop (&iter, "(&s&s)", &type, &owner)) {
        if (g_str_equal (cred_type, type)) {
            *cred_owner_return = owner;
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
is_credential_supported (GVariant *supported, const gchar *cred_type, const gchar *cred_owner)
{
    GVariantIter iter;
    const gchar *type;
    const gchar *owner;

    g_variant_iter_init(&iter, supported);
    while (g_variant_iter_loop (&iter, "(&s&s)", &type, &owner)) {
        if (g_str_equal(cred_type, type) &&
            g_str_equal(cred_owner, owner)) {
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
supports_dbus_interface(GVariant *dbus_props, const char *dbus_interface)
{
    gboolean result = FALSE;
    GVariantIter *iter = NULL;
    gchar *value;

    if (g_variant_lookup(dbus_props, "SupportedInterfaces", "as", &iter)) {
        while (g_variant_iter_next(iter, "&s", &value)) {
            if (strcmp(value, dbus_interface) == 0) {
                result = TRUE;
                break;
            }
        }
        G_VARIANT_ITER_FREE(iter);
    }
    return result;
}

KUint32 LMI_RealmdService_Join_Leave_Domain(
    bool join,
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_RealmdServiceRef* self,
    const KString* Domain,
    const KString* User,
    const KString* Password,
    const KStringA* OptionNames,
    const KStringA* OptionValues,
    CMPIStatus* status)
{
    const gchar *method_name = NULL;
    const gchar *supported_credentials_property = NULL;
    GError *g_error = NULL;
    KUint32 result = KUINT32_INIT;
    gint32 relevance = 0;
    gchar **paths = NULL;
    gchar *dbus_path, **pp;
    CMPICount n_paths;
    const gchar *cred_type = NULL;
    const gchar *cred_owner = NULL;
    GVariant *supported_creds = NULL;
    GVariant *realm_props = NULL;
    GVariant *kerberos_membership_props = NULL;
    GVariant *credentials = NULL;
    GVariant *options = NULL;

    KUint32_Set(&result, LMI_REALMD_RESULT_SUCCESS);
    CMSetStatus(status, CMPI_RC_OK);

    /* Assure we can communicate with DBus */
    if (!rdcp_dbus_initialize(&g_error)) {
        handle_g_error(&g_error, _cb, status, CMPI_RC_ERR_FAILED, "rdcp_dbus_initialize failed");
        KUint32_Set(&result, LMI_REALMD_RESULT_FAILED);
        goto exit;
    }

    if (join) {
        method_name = "Join";
        supported_credentials_property = "SupportedJoinCredentials";
    } else {
        method_name = "Leave";
        supported_credentials_property = "SupportedLeaveCredentials";
    }

    /* Call Discover to obtain list of DBus object paths for domain */
    if (!build_g_variant_options_from_KStringA(OptionNames, OptionValues, &options, &g_error)) {
        handle_g_error(&g_error, _cb, status, CMPI_RC_ERR_FAILED,
                       "failed to convert options to gvariant");
        KUint32_Set(&result, LMI_REALMD_RESULT_FAILED);
        goto exit;
    }

    if (!dbus_discover_call(system_bus, Domain->chars, options,
                            &relevance, &paths, &g_error)) {
        handle_g_error(&g_error, _cb, status, CMPI_RC_ERR_FAILED, "dbus_discover_call() failed");
        KUint32_Set(&result, LMI_REALMD_RESULT_FAILED);
        goto exit;
    }

#ifdef RDCP_DEBUG
    print_paths(paths, "%s: target=%s, paths:", __FUNCTION__, Domain->chars);
#endif

    for (pp = paths, dbus_path = *pp++, n_paths = 0; dbus_path; dbus_path = *pp++, n_paths++);

    if (n_paths < 1) {
        SetCMPIStatus(cb, status, CMPI_RC_ERR_FAILED, "Domain (%s) does not exist", Domain->chars);
        KUint32_Set(&result, LMI_REALMD_RESULT_NO_SUCH_DOMAIN);
        goto exit;
    }

    dbus_path = paths[0];

    /* Lookup the realm properties so we can determine the supported DBus interfaces */
    GET_DBUS_PROPERIES_OR_EXIT(realm_props, dbus_path,
                               REALM_DBUS_REALM_INTERFACE, status);
    if (!supports_dbus_interface(realm_props, REALM_DBUS_KERBEROS_MEMBERSHIP_INTERFACE)) {
        SetCMPIStatus(cb, status, CMPI_RC_ERR_FAILED, "Domain (%s) does not support joining or leaving",
                      Domain->chars);
        KUint32_Set(&result, LMI_REALMD_RESULT_DOMAIN_DOES_NOT_SUPPORT_JOINING);
        goto exit;
    }

    GET_DBUS_PROPERIES_OR_EXIT(kerberos_membership_props, dbus_path,
                               REALM_DBUS_KERBEROS_MEMBERSHIP_INTERFACE, status);

    if (!g_variant_lookup(kerberos_membership_props, supported_credentials_property, "@a(ss)",
                          &supported_creds)) {
        SetCMPIStatus(cb, status, CMPI_RC_ERR_FAILED,
                      "Domain (%s) did not supply supported %s credentials",
                      Domain->chars, method_name);
        KUint32_Set(&result, LMI_REALMD_RESULT_FAILED);
        goto exit;
    }

    if (!User->exists || User->null) {
        /* No User */
        if (!Password->exists || Password->null) {
            /* No User, No Password: automatic */
            cred_type = "automatic";
            if (!get_credential_supported_owner(supported_creds, cred_type, &cred_owner)) {
                SetCMPIStatus(cb, status, CMPI_RC_ERR_FAILED,
                              "Domain (%s) does not support automatic %s credentials",
                              Domain->chars, method_name);
                KUint32_Set(&result, LMI_REALMD_RESULT_DOMAIN_DOES_NOT_SUPPORT_PROVIDED_CREDENTIALS);
                goto exit;
            }

            credentials = g_variant_new ("(ssv)", cred_type, cred_owner,
                                         g_variant_new_string (""));

        } else {
            /* No User, Password: one time password using secret */
            cred_type = "secret";
            if (!get_credential_supported_owner(supported_creds, cred_type, &cred_owner)) {
                SetCMPIStatus(cb, status, CMPI_RC_ERR_FAILED,
                              "Domain (%s) does not support secret %s credentials",
                              Domain->chars, method_name);
                KUint32_Set(&result, LMI_REALMD_RESULT_DOMAIN_DOES_NOT_SUPPORT_PROVIDED_CREDENTIALS);
                goto exit;
            }
            credentials = g_variant_new("(ssv)", cred_type, cred_owner,
                                        g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE,
                                                                   Password->chars,
                                                                   strlen(Password->chars), 1));
        }
    } else {
        /* User */
        if (!Password->exists || Password->null) {
            /* User, No Password: invalid combination */
            SetCMPIStatus(cb, status, CMPI_RC_ERR_INVALID_PARAMETER,
                          "Must provide a password when User is provided");
            KUint32_Set(&result, LMI_REALMD_RESULT_FAILED);
            goto exit;
        } else {
            /* User, Password: password auth */
            cred_type = "password";
            cred_owner = "administrator";
            if (!is_credential_supported(supported_creds, cred_type, cred_owner)) {
                SetCMPIStatus(cb, status, CMPI_RC_ERR_FAILED,
                              "Domain (%s) does not support password with administrator ownership credentials",
                              Domain->chars);
                KUint32_Set(&result, LMI_REALMD_RESULT_DOMAIN_DOES_NOT_SUPPORT_PROVIDED_CREDENTIALS);
                goto exit;
            }
            credentials = g_variant_new("(ssv)", cred_type, cred_owner,
                                        g_variant_new("(ss)", User->chars, Password->chars));

        }
    }

    if (join) {
        if (!dbus_join_call(system_bus, dbus_path, credentials, options, &g_error)) {
            handle_g_error(&g_error, cb, status, CMPI_RC_ERR_FAILED, "dbus_join_call() failed");
            KUint32_Set(&result, LMI_REALMD_RESULT_FAILED);
            goto exit;
        }
    } else {
        if (!dbus_leave_call(system_bus, dbus_path, credentials, options, &g_error)) {
            handle_g_error(&g_error, cb, status, CMPI_RC_ERR_FAILED, "dbus_leave_call() failed");
            KUint32_Set(&result, LMI_REALMD_RESULT_FAILED);
            goto exit;
        }
    }


 exit:

    G_VARIANT_FREE(supported_creds);
    G_VARIANT_FREE(realm_props);
    G_VARIANT_FREE(kerberos_membership_props);
    G_VARIANT_FREE(credentials);
    G_VARIANT_FREE(options);
    g_strfreev(paths);

    return result;
}
KEXTERN KUint32 LMI_RealmdService_JoinDomain(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_RealmdServiceRef* self,
    const KString* Domain,
    const KString* User,
    const KString* Password,
    const KStringA* OptionNames,
    const KStringA* OptionValues,
    CMPIStatus* status)
{
    return LMI_RealmdService_Join_Leave_Domain(true, cb, mi, context, self,
                                               Domain, User, Password,
                                               OptionNames, OptionValues,
                                               status);
}

KEXTERN KUint32 LMI_RealmdService_LeaveDomain(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_RealmdServiceRef* self,
    const KString* Domain,
    const KString* User,
    const KString* Password,
    const KStringA* OptionNames,
    const KStringA* OptionValues,
    CMPIStatus* status)
{
    return LMI_RealmdService_Join_Leave_Domain(false, cb, mi, context, self,
                                               Domain, User, Password,
                                               OptionNames, OptionValues,
                                               status);
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_RealmdService",
    "LMI_RealmdService",
    "instance method")
