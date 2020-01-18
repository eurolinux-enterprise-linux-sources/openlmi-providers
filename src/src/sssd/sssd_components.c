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

#include <stdint.h>
#include <sss_sifp_dbus.h>
#include <dbus/dbus.h>
#include <konkret/konkret.h>
#include "sssd_components.h"
#include "utils.h"

static sss_sifp_error
sssd_component_find(sss_sifp_ctx *sifp_ctx,
                    const char *name,
                    sssd_component_type type,
                    char **_path)
{
    sss_sifp_error error;

    switch (type) {
    case SSSD_COMPONENT_MONITOR:
        error = sss_sifp_invoke_find(sifp_ctx, SSSD_DBUS_FIND_MONITOR, _path,
                                     DBUS_TYPE_INVALID);
        break;
    case SSSD_COMPONENT_RESPONDER:
        error = sss_sifp_invoke_find(sifp_ctx, SSSD_DBUS_FIND_RESPONDER, _path,
                                     DBUS_TYPE_STRING, &name,
                                     DBUS_TYPE_INVALID);
        break;
    case SSSD_COMPONENT_BACKEND:
        error = sss_sifp_invoke_find(sifp_ctx, SSSD_DBUS_FIND_BACKEND, _path,
                                     DBUS_TYPE_STRING, &name,
                                     DBUS_TYPE_INVALID);
        break;
    }

    return error;
}

static sssd_method_error
sssd_component_send_message(sss_sifp_ctx *sifp_ctx,
                            DBusMessage *message)
{
    sss_sifp_error error;

    error = sss_sifp_send_message(sifp_ctx, message, NULL);
    if (error == SSS_SIFP_IO_ERROR) {
        if (strcmp(sss_sifp_get_last_io_error_name(sifp_ctx),
                   DBUS_ERROR_NOT_SUPPORTED) == 0) {
            return SSSD_METHOD_ERROR_NOT_SUPPORTED;
        }

        return SSSD_METHOD_ERROR_IO;
    } else if (error != SSS_SIFP_OK) {
        return SSSD_METHOD_ERROR_FAILED;
    }

    return SSSD_METHOD_ERROR_OK;
}

static KUint32
sssd_component_set_debug_level(const char *method,
                               const char *name,
                               sssd_component_type type,
                               const KUint16* debug_level,
                               CMPIStatus* _status)
{
    KUint32 result = KUINT32_INIT;
    DBusMessage *msg = NULL;
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_error error;
    dbus_bool_t dbret;
    sssd_method_error ret;
    char *path = NULL;
    uint32_t level;

    KSetStatus(_status, OK);

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_IO, done);

    error = sssd_component_find(sifp_ctx, name, type, &path);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_IO, done);

    if (!debug_level->exists || debug_level->null) {
        KSetStatus(_status, ERR_INVALID_PARAMETER);
        ret = SSSD_METHOD_ERROR_FAILED;
        goto done;
    }

    level = debug_level->value;

    msg = sss_sifp_create_message(path, SSS_SIFP_IFACE_COMPONENTS, method);
    if (msg == NULL) {
        ret = SSSD_METHOD_ERROR_FAILED;
        goto done;
    }

    dbret = dbus_message_append_args(msg, DBUS_TYPE_UINT32, &level,
                                          DBUS_TYPE_INVALID);
    if (!dbret) {
        ret = SSSD_METHOD_ERROR_FAILED;
        goto done;
    }

    ret = sssd_component_send_message(sifp_ctx, msg);

done:
    if (msg != NULL) {
        dbus_message_unref(msg);
    }

    sss_sifp_free_string(sifp_ctx, &path);
    sss_sifp_free(&sifp_ctx);

    KUint32_Set(&result, ret);
    return result;
}

static KUint32
sssd_component_set_state(const char *method,
                         const char *name,
                         sssd_component_type type,
                         CMPIStatus* _status)
{
    KUint32 result = KUINT32_INIT;
    DBusMessage *msg = NULL;
    sss_sifp_ctx *sifp_ctx = NULL;
    sss_sifp_error error;
    sssd_method_error ret;
    char *path = NULL;

    error = sss_sifp_init(&sifp_ctx);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_IO, done);

    error = sssd_component_find(sifp_ctx, name, type, &path);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_IO, done);

    msg = sss_sifp_create_message(path, SSS_SIFP_IFACE_COMPONENTS, method);
    if (msg == NULL) {
        ret = SSSD_METHOD_ERROR_FAILED;
        goto done;
    }

    ret = sssd_component_send_message(sifp_ctx, msg);

done:
    if (msg != NULL) {
        dbus_message_unref(msg);
    }

    sss_sifp_free_string(sifp_ctx, &path);
    sss_sifp_free(&sifp_ctx);

    KSetStatus(_status, OK);
    KUint32_Set(&result, ret);
    return result;
}

KUint32
sssd_component_set_debug_permanently(const char *name,
                                     sssd_component_type type,
                                     const KUint16* debug_level,
                                     CMPIStatus* _status)
{
    return sssd_component_set_debug_level("ChangeDebugLevel",
                                          name, type, debug_level, _status);
}

KUint32
sssd_component_set_debug_temporarily(const char *name,
                                     sssd_component_type type,
                                     const KUint16* debug_level,
                                     CMPIStatus* _status)
{
    return sssd_component_set_debug_level("ChangeDebugLevelTemporarily",
                                          name, type, debug_level, _status);
}

KUint32 sssd_component_enable(const char *name,
                              sssd_component_type type,
                              CMPIStatus* _status)
{
    return sssd_component_set_state("Enable", name, type, _status);
}

KUint32 sssd_component_disable(const char *name,
                               sssd_component_type type,
                               CMPIStatus* _status)
{
    return sssd_component_set_state("Disable", name, type, _status);
}

struct sssd_component_attrs {
    const char *name;
    bool enabled;
    uint32_t debug;
};

static sssd_method_error
sssd_component_get_attrs(sss_sifp_attr **attrs,
                         const char *path,
                         struct sssd_component_attrs *out)
{
    sss_sifp_error error;
    sssd_method_error ret;

    error = sss_sifp_find_attr_as_string(attrs, "name", &out->name);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_bool(attrs, "enabled", &out->enabled);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    error = sss_sifp_find_attr_as_uint32(attrs, "debug_level", &out->debug);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_FAILED, done);

    ret = SSSD_METHOD_ERROR_OK;

done:
    return ret;
}

sssd_method_error
sssd_monitor_set_instance(sss_sifp_ctx *sifp_ctx,
                          const char *path,
                          const CMPIBroker* cb,
                          const char *namespace,
                          LMI_SSSDMonitor *instance)
{
    sss_sifp_attr **attrs = NULL;
    sss_sifp_error error;
    struct sssd_component_attrs values;
    sssd_method_error ret;

    error = sss_sifp_fetch_all_attrs(sifp_ctx, path,
                                     SSS_SIFP_IFACE_COMPONENTS, &attrs);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_IO, done);

    ret = sssd_component_get_attrs(attrs, path, &values);
    if (ret != SSSD_METHOD_ERROR_OK) {
        goto done;
    }

    LMI_SSSDMonitor_Init(instance, cb, namespace);

    /* CIM_ManagedElement */
    LMI_SSSDMonitor_Set_Caption(instance, "SSSD Monitor Component");
    LMI_SSSDMonitor_Set_Description(instance, "SSSD Monitor Component");
    LMI_SSSDMonitor_Set_ElementName(instance, values.name);

    /* LMI_SSSDMonitor */
    LMI_SSSDMonitor_Set_Name(instance, values.name);
    LMI_SSSDMonitor_Set_Type(instance, SSSD_COMPONENT_MONITOR);
    LMI_SSSDMonitor_Set_IsEnabled(instance, values.enabled);
    LMI_SSSDMonitor_Set_DebugLevel(instance, values.debug);

    ret = SSSD_METHOD_ERROR_OK;

done:
    sss_sifp_free_attrs(sifp_ctx, &attrs);
    return ret;
}

sssd_method_error
sssd_responder_set_instance(sss_sifp_ctx *sifp_ctx,
                            const char *path,
                            const CMPIBroker* cb,
                            const char *namespace,
                            LMI_SSSDResponder *instance)
{
    sss_sifp_attr **attrs = NULL;
    sss_sifp_error error;
    struct sssd_component_attrs values;
    sssd_method_error ret;

    error = sss_sifp_fetch_all_attrs(sifp_ctx, path,
                                     SSS_SIFP_IFACE_COMPONENTS, &attrs);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_IO, done);

    ret = sssd_component_get_attrs(attrs, path, &values);
    if (ret != SSSD_METHOD_ERROR_OK) {
        goto done;
    }

    LMI_SSSDResponder_Init(instance, cb, namespace);

    /* CIM_ManagedElement */
    LMI_SSSDResponder_Set_Caption(instance, "SSSD Responder Component");
    LMI_SSSDResponder_Set_Description(instance, "SSSD Responder Component");
    LMI_SSSDResponder_Set_ElementName(instance, values.name);

    /* LMI_SSSDResponder */
    LMI_SSSDResponder_Set_Name(instance, values.name);
    LMI_SSSDResponder_Set_Type(instance, SSSD_COMPONENT_RESPONDER);
    LMI_SSSDResponder_Set_IsEnabled(instance, values.enabled);
    LMI_SSSDResponder_Set_DebugLevel(instance, values.debug);

    ret = SSSD_METHOD_ERROR_OK;

done:
    sss_sifp_free_attrs(sifp_ctx, &attrs);
    return ret;
}

sssd_method_error
sssd_backend_set_instance(sss_sifp_ctx *sifp_ctx,
                          const char *path,
                          const CMPIBroker* cb,
                          const char *namespace,
                          LMI_SSSDBackend *instance)
{
    sss_sifp_attr **attrs = NULL;
    sss_sifp_error error;
    struct sssd_component_attrs values;
    sssd_method_error ret;

    error = sss_sifp_fetch_all_attrs(sifp_ctx, path,
                                     SSS_SIFP_IFACE_COMPONENTS, &attrs);
    check_sss_sifp_error(error, ret, SSSD_METHOD_ERROR_IO, done);

    ret = sssd_component_get_attrs(attrs, path, &values);
    if (ret != SSSD_METHOD_ERROR_OK) {
        goto done;
    }

    LMI_SSSDBackend_Init(instance, cb, namespace);

    /* CIM_ManagedElement */
    LMI_SSSDBackend_Set_Caption(instance, "SSSD Responder Component");
    LMI_SSSDBackend_Set_Description(instance, "SSSD Responder Component");
    LMI_SSSDBackend_Set_ElementName(instance, values.name);

    /* LMI_SSSDBackend */
    LMI_SSSDBackend_Set_Name(instance, values.name);
    LMI_SSSDBackend_Set_Type(instance, SSSD_COMPONENT_RESPONDER);
    LMI_SSSDBackend_Set_IsEnabled(instance, values.enabled);
    LMI_SSSDBackend_Set_DebugLevel(instance, values.debug);

    ret = SSSD_METHOD_ERROR_OK;

done:
    sss_sifp_free_attrs(sifp_ctx, &attrs);
    return ret;
}
