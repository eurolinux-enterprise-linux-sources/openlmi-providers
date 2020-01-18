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
 * Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
 *          Radek Novacek <rnovacek@redhat.com>
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "serviceutil.h"

#define INITIAL_SLIST_NALLOC 100

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

void service_free_all_services(AllServices *svcs)
{
    int i;

    if (!svcs)
        return;

    for(i = 0; i < svcs->cnt; i++) {
        free(svcs->svc[i]->svName);
        free(svcs->svc[i]->svCaption);
        free(svcs->svc[i]->svStatus);
        free(svcs->svc[i]);
    }
    free(svcs);

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
    if (!slist) {
        strncpy(output, "Insufficient memory", output_len);
        return NULL;
    }
    slist->nalloc = INITIAL_SLIST_NALLOC;
    slist->name = malloc(slist->nalloc * sizeof(char *));
    if (!slist->name) {
        free(slist);
        g_object_unref(manager_proxy);
        strncpy(output, "Insufficient memory", output_len);
        return NULL;
    }
    slist->cnt = 0;

    g_variant_get(result, "(a(ss))", &arr);
    while (g_variant_iter_loop(arr, "(ss)", &primary_unit_name, NULL)) {
        /* Ignore instantiable units (containing '@') until we find out how to properly present them */
        if (strstr(primary_unit_name, ".service") && strchr(primary_unit_name, '@') == NULL) {
            if (slist->cnt >= slist->nalloc) {
                char **tmpp = NULL;
                slist->nalloc *= 2;
                tmpp = realloc(slist->name, slist->nalloc * sizeof(char *));
                if (!tmpp) {
                    g_variant_iter_free(arr);
                    free(slist);
                    g_object_unref(manager_proxy);
                    strncpy(output, "Insufficient memory", output_len);
                    return NULL;
                }
                slist->name = tmpp;
            }
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

AllServices *service_get_properties_all(
    char *output,
    int output_len)
{
    GDBusProxy *manager_proxy = NULL;
    GDBusProxy *proxy = NULL;
    GVariantIter *arr = NULL;
    GVariant *result = NULL;
    GVariant *result2 = NULL;
    GError *error = NULL;
    gchar *primary_unit_name = NULL, *unit_file_state = NULL;
    gchar *unit, *value_str;
    AllServices *svcs = NULL;
    char *tmps = NULL;

    svcs = malloc(sizeof(AllServices));
    if (!svcs) {
        strncpy(output, "Insufficient memory", output_len);
        return NULL;
    }
    svcs->nalloc = INITIAL_SLIST_NALLOC;
    svcs->svc = malloc(svcs->nalloc * sizeof(Service *));
    if (!svcs->svc) {
        free(svcs);
        strncpy(output, "Insufficient memory", output_len);
        return NULL;
    }
    svcs->cnt = 0;

#if !defined(GLIB_VERSION_2_36)
    g_type_init();
#endif

    manager_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, MANAGER_NAME, MANAGER_OP, MANAGER_INTERFACE, NULL, &error);
    if (!manager_proxy) goto err;

    error = NULL;
    result = g_dbus_proxy_call_sync(manager_proxy, "ListUnitFiles", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) goto err;

    g_variant_get(result, "(a(ss))", &arr);
    while (g_variant_iter_loop(arr, "(ss)", &primary_unit_name, &unit_file_state)) {
        /* Ignore instantiable units (containing '@') until we find out how to properly present them */
        if (strstr(primary_unit_name, ".service") && strchr(primary_unit_name, '@') == NULL) {

            /* Realloc we are out of space */
            if (svcs->cnt >= svcs->nalloc) {
                Service **tmpp = NULL;
                svcs->nalloc *= 2;
                tmpp = realloc(svcs->svc, svcs->nalloc * sizeof(Service *));
                if (!tmpp) {
                    g_variant_iter_free(arr);
                    service_free_all_services(svcs);
                    g_object_unref(manager_proxy);
                    strncpy(output, "Insufficient memory", output_len);
                    return NULL;
                }
                svcs->svc = tmpp;
            }

            svcs->svc[svcs->cnt] = malloc(sizeof(Service));

            /* Fill svName */
            tmps = strdup(primary_unit_name);
            if (!tmps) {
                g_variant_iter_free(arr);
                service_free_all_services(svcs);
                g_object_unref(manager_proxy);
                strncpy(output, "Insufficient memory", output_len);
                return NULL;
            }
            svcs->svc[svcs->cnt]->svName = strndup(basename(tmps), strlen(basename(tmps)));
            if (!svcs->svc[svcs->cnt]->svName) {
                free(tmps);
                g_variant_iter_free(arr);
                service_free_all_services(svcs);
                g_object_unref(manager_proxy);
                strncpy(output, "Insufficient memory", output_len);
                return NULL;
            }
            free(tmps);

            /* Fill svEnabledDefault */
            svcs->svc[svcs->cnt]->svEnabledDefault = NOT_APPLICABLE;
            if (strncmp(unit_file_state, "enabled", 7) == 0)
                svcs->svc[svcs->cnt]->svEnabledDefault = ENABLED;
            if (strncmp(unit_file_state, "disabled", 8) == 0)
                svcs->svc[svcs->cnt]->svEnabledDefault = DISABLED;

            error = NULL;
            result = g_dbus_proxy_call_sync(manager_proxy, "LoadUnit", g_variant_new("(s)", svcs->svc[svcs->cnt]->svName),
                G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
            if (error) goto err;

            g_variant_get(result, "(o)", &unit);

            proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
                NULL, MANAGER_NAME, unit, PROPERTY_INTERFACE, NULL, &error);
            if (!proxy) goto err;

            error = NULL;
            result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", UNIT_INTERFACE, "Description"),
                G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
            if (error) goto err;

            /* Fill svCaption */
            g_variant_get(result, "(v)", &result2);
            g_variant_get(result2, "s", &value_str);
            svcs->svc[svcs->cnt]->svCaption = strdup(value_str);
            if (!svcs->svc[svcs->cnt]->svCaption) goto err;

            error = NULL;
            result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", UNIT_INTERFACE, "ActiveState"),
                G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
            if (error) goto err;

            /* Fill svOperationalStatus, svStarted, svStatus */
            g_variant_get(result, "(v)", &result2);
            g_variant_get(result2, "s", &value_str);

            if (strcmp(value_str, "active") == 0) {
                svcs->svc[svcs->cnt]->svOperationalStatus[0] = OS_OK;
                svcs->svc[svcs->cnt]->svOperationalStatusCnt = 1;
                svcs->svc[svcs->cnt]->svStarted = 1;
                svcs->svc[svcs->cnt]->svStatus = strdup("OK");
            }
            else if (strcmp(value_str, "inactive") == 0) {
                svcs->svc[svcs->cnt]->svOperationalStatus[0] = OS_COMPLETED;
                svcs->svc[svcs->cnt]->svOperationalStatus[1] = OS_OK;
                svcs->svc[svcs->cnt]->svOperationalStatusCnt = 2;
                svcs->svc[svcs->cnt]->svStarted = 0;
                svcs->svc[svcs->cnt]->svStatus = strdup("Stopped");
            }
            else if (strcmp(value_str, "failed") == 0) {
                svcs->svc[svcs->cnt]->svOperationalStatus[0] = OS_COMPLETED;
                svcs->svc[svcs->cnt]->svOperationalStatus[1] = OS_ERROR;
                svcs->svc[svcs->cnt]->svOperationalStatusCnt = 2;
                svcs->svc[svcs->cnt]->svStarted = 0;
                svcs->svc[svcs->cnt]->svStatus = strdup("Stopped");
            }
            else if (strcmp(value_str, "activating") == 0) {
                svcs->svc[svcs->cnt]->svOperationalStatus[0] = OS_STARTING;
                svcs->svc[svcs->cnt]->svOperationalStatusCnt = 1;
                svcs->svc[svcs->cnt]->svStarted = 0;
                svcs->svc[svcs->cnt]->svStatus = strdup("Stopped");
            }
            else if (strcmp(value_str, "deactivating") == 0) {
                svcs->svc[svcs->cnt]->svOperationalStatus[0] = OS_STOPPING;
                svcs->svc[svcs->cnt]->svOperationalStatusCnt = 1;
                svcs->svc[svcs->cnt]->svStarted = 1;
                svcs->svc[svcs->cnt]->svStatus = strdup("OK");
            }
            if (!svcs->svc[svcs->cnt]->svStatus) goto err;

            g_object_unref(proxy);
            svcs->cnt++;
        }
    }
    g_variant_iter_free(arr);
    arr = NULL;

    g_object_unref(manager_proxy);
    manager_proxy = NULL;

    return svcs;

err:
    service_free_all_services(svcs);
    if (error) {
        strncpy(output, error->message, output_len);
        g_error_free(error);
    }
    if (arr) g_variant_iter_free(arr);
    if (manager_proxy) g_object_unref(manager_proxy);
    if (proxy) g_object_unref(proxy);

    return NULL;
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

char *job_result = NULL;
GMainLoop *loop = NULL;

static void on_manager_signal(
    GDBusProxy *proxy,
    gchar *sender_name,
    gchar *signal_name,
    GVariant *parameters,
    gpointer user_data)
{
    gchar *result, *job;

    if (g_strcmp0(signal_name, "JobRemoved") == 0) {

        g_variant_get(parameters, "(u&os&s)", NULL, &job, NULL, &result);

        if (g_strcmp0((gchar *) user_data, job) == 0) {
            job_result = strdup(result);
            g_main_loop_quit(loop);
        }
    }
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
    GMainContext *context;
    GVariant *result = NULL;
    gchar *job = NULL;

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

    context = g_main_context_new();

    g_main_context_push_thread_default(context);

    manager_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, MANAGER_NAME, MANAGER_OP, MANAGER_INTERFACE, NULL, &error);
    if (!manager_proxy) {
        strncpy(output, error->message, output_len);
        g_error_free(error);
        g_main_context_unref(context);
        return -1;
    }

    error = NULL;
    if (!strcasecmp(method, "EnableUnitFiles") || !strcasecmp(method, "DisableUnitFiles")) {
        /* these two methods don't return job - always success, don't wait for signal etc. */
        builder = g_variant_builder_new(G_VARIANT_TYPE ("as"));
        g_variant_builder_add(builder, "s", service);
        if (!strcasecmp(method, "EnableUnitFiles")) {
            g_dbus_proxy_call_sync(manager_proxy, method, g_variant_new("(asbb)", builder, FALSE, TRUE),
                G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        } else {
            g_dbus_proxy_call_sync(manager_proxy, method, g_variant_new("(asb)", builder, FALSE),
                G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        }
        g_object_unref(manager_proxy);
        g_main_context_pop_thread_default(context);
        g_main_context_unref(context);
        if (error) {
            strncpy(output, error->message, output_len);
            g_error_free(error);
            return -1;
        }
        return 0;
    }

    g_dbus_proxy_call_sync(manager_proxy, "Subscribe", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) {
        strncpy(output, error->message, output_len);
        g_error_free(error);
        g_object_unref(manager_proxy);
        g_main_context_unref(context);
        return -1;
    }

    g_main_context_pop_thread_default(context);

    loop = g_main_loop_new(context, FALSE);

    error = NULL;
    result = g_dbus_proxy_call_sync(manager_proxy, method, g_variant_new("(ss)", service, "replace"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) {
        strncpy(output, error->message, output_len);
        g_error_free(error);
        g_dbus_proxy_call_sync(manager_proxy, "Unsubscribe", NULL,
            G_DBUS_CALL_FLAGS_NONE, -1,  NULL, NULL);
        g_object_unref(manager_proxy);
        g_main_loop_unref(loop);
        g_main_context_unref(context);
        return -1;
    }

    g_variant_get(result, "(&o)", &job);

    g_signal_connect(manager_proxy, "g-signal", G_CALLBACK(on_manager_signal), job);

    g_main_loop_run(loop);

    lmi_debug("job_result: %s", job_result);
    strncpy(output, job_result, output_len);

    g_dbus_proxy_call_sync(manager_proxy, "Unsubscribe", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1,  NULL, &error);
    if (error) {
        lmi_debug("Unsubscribe error: %s", error->message);
        g_error_free(error);
    }

    g_object_unref(manager_proxy);
    g_main_loop_unref(loop);
    g_main_context_unref(context);

    if (strcmp(job_result, JR_DONE) != 0) {
        free(job_result);
        return -1;
    }

    free(job_result);
    return 0;
}

/* Indications */

pthread_mutex_t m;
pthread_cond_t c;

void *loop_thread(
    void *arg)
{
    ServiceIndication *si = (ServiceIndication *) arg;

    lmi_debug("loop_thread enter");

    g_main_loop_run(si->loop);

    lmi_debug("loop_thread exit");

    return NULL;
}

static void on_signal(
    GDBusProxy *proxy,
    gchar *sender_name,
    gchar *signal_name,
    GVariant *parameters,
    gpointer user_data)
{
    lmi_debug("on_signal enter, object_path: %s", g_dbus_proxy_get_object_path(proxy));

    pthread_mutex_lock(&m);
    pthread_cond_signal(&c);
    pthread_mutex_unlock(&m);

    lmi_debug("on_signal exit");
}

int ind_init(ServiceIndication *si, char *output, int output_len)
{
    GVariant *result = NULL;
    GError *error = NULL;
    gchar *tmps = NULL;
    int i = 0;

    lmi_debug("ind_init enter");

    si->context = g_main_context_new();
    g_main_context_push_thread_default(si->context);

    si->loop = NULL;

    si->slist = service_find_all(output, output_len);
    if (si->slist == NULL) {
        g_main_context_unref(si->context);
        return -1;
    }

    si->manager_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, MANAGER_NAME, MANAGER_OP, MANAGER_INTERFACE, NULL, &error);
    if (!si->manager_proxy) {
        strncpy(output, error->message, output_len);
        g_error_free(error);
        g_main_context_unref(si->context);
        service_free_slist(si->slist);
        return -1;
    }

    si->signal_proxy = malloc(si->slist->cnt * sizeof(GDBusProxy *));
    if (!si->signal_proxy) {
        strncpy(output, "Insufficient memory", output_len);
        g_main_context_unref(si->context);
        service_free_slist(si->slist);
        return -1;
    }

    for (i = 0; i < si->slist->cnt; i++) {
        result = g_dbus_proxy_call_sync(si->manager_proxy, "LoadUnit", g_variant_new("(s)", si->slist->name[i]),
            G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        if (error) {
            strncpy(output, error->message, output_len);
            g_error_free(error);
            ind_destroy(si);
            return -1;
        }
        g_variant_get(result, "(&o)", &tmps);

        error = NULL;
        si->signal_proxy[i] = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
            NULL, MANAGER_NAME, tmps, PROPERTY_INTERFACE, NULL, &error);
        g_variant_unref(result);
        if (si->signal_proxy[i] == NULL)
        {
            strncpy(output, error->message, output_len);
            g_error_free(error);
            ind_destroy(si);
            return -1;
        }
        g_signal_connect(si->signal_proxy[i], "g-signal", G_CALLBACK(on_signal), NULL);
    }

    error = NULL;
    g_dbus_proxy_call_sync(si->manager_proxy, "Subscribe", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1,  NULL, &error);
    if (error) {
        strncpy(output, error->message, output_len);
        g_error_free(error);
        ind_destroy(si);
        return -1;
    }

    g_main_context_pop_thread_default(si->context);

    si->loop = g_main_loop_new(si->context, FALSE);

    if (pthread_create(&si->p, NULL, loop_thread, si) != 0) {
        ind_destroy(si);
        return -1;
    }

    if (pthread_mutex_init(&m, NULL) != 0) {
        strncpy(output, "pthread_mutex_init error", output_len);
        return -1;
    }
    if (pthread_cond_init(&c, NULL) != 0) {
        strncpy(output, "pthread_cond_init error", output_len);
        return -1;
    }

    lmi_debug("ind_init exit");

    return 0;
}

bool ind_watcher(
    void **data)
{
    lmi_debug("ind_watcher enter");

    pthread_mutex_lock(&m);
    pthread_cond_wait(&c, &m);
    pthread_mutex_unlock(&m);

    lmi_debug("ind_watcher exit");

    return true;
}

void ind_destroy(ServiceIndication *si)
{
    int i;
    GError *error = NULL;

    lmi_debug("ind_destroy enter");

    if (si->loop) {
        g_main_loop_quit(si->loop);
        if (pthread_join(si->p, NULL) != 0) {
            lmi_debug("pthread_join error");
        }
        g_main_loop_unref(si->loop);
    }

    if (pthread_cond_destroy(&c) != 0) {
        lmi_debug("pthread_cond_destroy error");
    }
    if (pthread_mutex_destroy(&m) != 0) {
        lmi_debug("pthread_mutex_destroy error");
    }

    g_main_context_unref(si->context);

    for (i = 0; i < si->slist->cnt; i++) {
        if (si->signal_proxy[i]) {
            g_object_unref(si->signal_proxy[i]);
        }
    }
    free(si->signal_proxy);

    g_dbus_proxy_call_sync(si->manager_proxy, "Unsubscribe", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1,  NULL, &error);
    if (error) {
        lmi_debug("Unsubscribe error: %s", error->message);
        g_error_free(error);
    }

    g_object_unref(si->manager_proxy);

    service_free_slist(si->slist);

    lmi_debug("ind_destroy exit");

    return;
}
