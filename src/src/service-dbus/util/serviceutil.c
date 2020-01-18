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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <gio/gio.h>

#include "serviceutil.h"

#define MAX_SLIST_CNT 1000

#define MANAGER_NAME "org.freedesktop.systemd1"
#define MANAGER_OP "/org/freedesktop/systemd1"
#define MANAGER_INTERFACE "org.freedesktop.systemd1.Manager"
#define UNIT_INTERFACE "org.freedesktop.systemd1.Unit"
#define SERVICE_INTERFACE "org.freedesktop.systemd1.Service"
#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

const char *provider_name = "service";
const ConfigEntry *provider_config_defaults = NULL;

void service_free_slist(SList *slist)
{
    int i;

    if (!slist)
        return;

    for(i = 0; i < slist->cnt; i++)
        free(slist->name[i]);
    free(slist->name);
    free(slist);

    return;
}

SList *service_find_all(
    char *output,
    int output_len)
{
    GDBusProxy *manager_proxy = NULL;
    GVariantIter *arr = NULL;
    GVariant *result = NULL;
    GError *error = NULL;
    SList *slist = NULL;
    gchar *primary_unit_name = NULL;
    char *tmps = NULL;

#if !defined(GLIB_VERSION_2_36)
    g_type_init();
#endif

    manager_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, MANAGER_NAME, MANAGER_OP, MANAGER_INTERFACE, NULL, &error);
    if (!manager_proxy) {
        strncpy(output, error->message, output_len);
        g_error_free(error);
        return NULL;
    }

    error = NULL;
    result = g_dbus_proxy_call_sync(manager_proxy, "ListUnitFiles", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) {
        strncpy(output, error->message, output_len);
        g_error_free(error);
        g_object_unref(manager_proxy);
        return NULL;
    }

    slist = malloc(sizeof(SList));
    if (!slist) return NULL;
    slist->name = malloc(MAX_SLIST_CNT * sizeof(char *));
    if (!slist->name) {
        free(slist);
        g_object_unref(manager_proxy);
        return NULL;
    }
    slist->cnt = 0;

    g_variant_get(result, "(a(ss))", &arr);
    while (g_variant_iter_loop(arr, "(ss)", &primary_unit_name, NULL)) {
        /* Ignore instantiable units (containing '@') until we find out how to properly present them */
        if (strstr(primary_unit_name, ".service") && strchr(primary_unit_name, '@') == NULL) {
            tmps = strdup(primary_unit_name);
            if (!tmps)
                continue;
            slist->name[slist->cnt] = strndup(basename(tmps), strlen(basename(tmps)));
            if (!slist->name[slist->cnt]) {
                free(tmps);
                continue;
            }
            free(tmps);
            slist->cnt++;
        }
    }
    g_variant_iter_free(arr);

    g_object_unref(manager_proxy);
    return slist;
}


int service_get_properties(
    Service *svc,
    const char *service,
    char *output,
    int output_len)
{
    GDBusProxy *manager_proxy = NULL;
    GDBusProxy *proxy = NULL;
    GVariantIter *arr = NULL;
    GVariant *result = NULL;
    GVariant *result2 = NULL;
    GError *error = NULL;
    gchar *fragment_path = NULL, *unit_file_state = NULL;
    gchar *unit, *value_str;
    char found = 0;

    if (!service) {
        strncpy(output, "Invalid service name", output_len);
        return -1;
    }

    svc->svName = strdup(service);
    if (!svc->svName) {
        strncpy(output, "Insufficient memory", output_len);
        return -1;
    }

#if !defined(GLIB_VERSION_2_36)
    g_type_init();
#endif

    manager_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, MANAGER_NAME, MANAGER_OP, MANAGER_INTERFACE, NULL, &error);
    if (!manager_proxy) goto error;

    error = NULL;
    result = g_dbus_proxy_call_sync(manager_proxy, "ListUnitFiles", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) goto error;

    svc->svEnabledDefault = NOT_APPLICABLE;
    g_variant_get(result, "(a(ss))", &arr);
    while (g_variant_iter_loop(arr, "(ss)", &fragment_path, &unit_file_state)) {
        if (strstr(fragment_path, service) && strcmp(strstr(fragment_path, service), service) == 0) {
            found = 1;
            if (strncmp(unit_file_state, "enabled", 7) == 0)
                svc->svEnabledDefault = ENABLED;
            if (strncmp(unit_file_state, "disabled", 8) == 0)
                svc->svEnabledDefault = DISABLED;
        }
    }
    g_variant_iter_free(arr);
    arr = NULL;

    if (!found) {
        free(svc->svName);
        g_object_unref(manager_proxy);
        return -2;
    }

    error = NULL;
    result = g_dbus_proxy_call_sync(manager_proxy, "LoadUnit", g_variant_new("(s)", service),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) goto error;

    g_object_unref(manager_proxy);
    manager_proxy = NULL;
    g_variant_get(result, "(o)", &unit);

    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, MANAGER_NAME, unit, PROPERTY_INTERFACE, NULL, &error);
    if (!proxy) goto error;

    error = NULL;
    result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", UNIT_INTERFACE, "Description"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) goto error;

    g_variant_get(result, "(v)", &result2);
    g_variant_get(result2, "s", &value_str);
    svc->svCaption = strdup(value_str);
    if (!svc->svCaption) goto error;

    error = NULL;
    result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", UNIT_INTERFACE, "ActiveState"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) goto error;

    g_variant_get(result, "(v)", &result2);
    g_variant_get(result2, "s", &value_str);

    if (!strcmp(value_str, "active")) {
        svc->svOperationalStatus[0] = OS_OK;
        svc->svOperationalStatusCnt = 1;
        svc->svStarted = 1;
        svc->svStatus = strdup("OK");
    }
    else if (!strcmp(value_str, "inactive")) {
        svc->svOperationalStatus[0] = OS_COMPLETED;
        svc->svOperationalStatus[1] = OS_OK;
        svc->svOperationalStatusCnt = 2;
        svc->svStarted = 0;
        svc->svStatus = strdup("Stopped");
    }
    else if (!strcmp(value_str, "failed")) {
        svc->svOperationalStatus[0] = OS_COMPLETED;
        svc->svOperationalStatus[1] = OS_ERROR;
        svc->svOperationalStatusCnt = 2;
        svc->svStarted = 0;
        svc->svStatus = strdup("Stopped");
    }
    else if (!strcmp(value_str, "activating")) {
        svc->svOperationalStatus[0] = OS_STARTING;
        svc->svOperationalStatusCnt = 1;
        svc->svStarted = 0;
        svc->svStatus = strdup("Stopped");
    }
    else if (!strcmp(value_str, "deactivating")) {
        svc->svOperationalStatus[0] = OS_STOPPING;
        svc->svOperationalStatusCnt = 1;
        svc->svStarted = 1;
        svc->svStatus = strdup("OK");
    }
    if (!svc->svStatus) goto error;

    g_object_unref(proxy);
    return 0;

error:
    if (svc->svName) free(svc->svName);
    if (error) {
        strncpy(output, error->message, output_len);
        g_error_free(error);
    }
    if (manager_proxy) g_object_unref(manager_proxy);
    if (proxy) g_object_unref(proxy);
    if (svc->svCaption) free(svc->svCaption);

    return -1;
}

unsigned int service_operation(
    const char *service,
    const char *method,
    char *output,
    int output_len)
{
    GDBusProxy *manager_proxy = NULL;
    GError *error = NULL;
    GVariantBuilder *builder;

    if (!service) {
        strncpy(output, "Invalid service name", output_len);
        return -1;
    }

    if (!method) {
        strncpy(output, "Invalid method name", output_len);
        return -1;
    }

#if !defined(GLIB_VERSION_2_36)
    g_type_init();
#endif

    manager_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, MANAGER_NAME, MANAGER_OP, MANAGER_INTERFACE, NULL, &error);
    if (!manager_proxy) {
        strncpy(output, error->message, output_len);
        g_error_free(error);
        return -1;
    }

    error = NULL;
    if (!strcasecmp(method, "EnableUnitFiles") || !strcasecmp(method, "DisableUnitFiles")) {
        builder = g_variant_builder_new(G_VARIANT_TYPE ("as"));
        g_variant_builder_add(builder, "s", service);
        if (!strcasecmp(method, "EnableUnitFiles")) {
            g_dbus_proxy_call_sync(manager_proxy, method, g_variant_new("(asbb)", builder, FALSE, TRUE),
                G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        } else {
            g_dbus_proxy_call_sync(manager_proxy, method, g_variant_new("(asb)", builder, FALSE),
                G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        }
        if (error) {
            strncpy(output, error->message, output_len);
            g_error_free(error);
            g_object_unref(manager_proxy);
            return -1;
        }
    } else {
        g_dbus_proxy_call_sync(manager_proxy, method, g_variant_new("(ss)", service, "replace"),
            G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        if (error) {
            strncpy(output, error->message, output_len);
            g_error_free(error);
            g_object_unref(manager_proxy);
            return -1;
        }
    }

    g_object_unref(manager_proxy);
    return 0;
}
