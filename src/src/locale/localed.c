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
 * Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
 */


#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>

#include "localed.h"

#define LOCALE_NAME "org.freedesktop.locale1"
#define LOCALE_OP "/org/freedesktop/locale1"
#define LOCALE_INTERFACE "org.freedesktop.locale1"
#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

const char *provider_name = "locale";
const ConfigEntry *provider_config_defaults = NULL;

void locale_init()
{
#if !defined(GLIB_VERSION_2_36)
    g_type_init();
#endif
}


CimLocale *locale_get_properties()
{
    GDBusProxy *proxy = NULL;
    GVariantIter *arr = NULL;
    GVariant *result = NULL;
    GVariant *result2 = NULL;
    gchar *env_str = NULL;
    gchar *value_str;
    CimLocale *cloc = NULL;


    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, LOCALE_NAME, LOCALE_OP, PROPERTY_INTERFACE, NULL, NULL);
    if (!proxy) goto error;

    cloc = malloc(sizeof(CimLocale));
    if (!cloc) goto error;
    cloc->LocaleCnt = 0;
    cloc->VConsoleKeymap = NULL;
    cloc->VConsoleKeymapToggle = NULL;
    cloc->X11Layouts = NULL;
    cloc->X11Model = NULL;
    cloc->X11Variant = NULL;
    cloc->X11Options = NULL;

    if ((result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", LOCALE_INTERFACE, "Locale"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL)) == NULL)
        goto error;
    g_variant_get(result, "(v)", &result2);
    g_variant_unref(result);
    g_variant_get(result2, "as", &arr);
    g_variant_unref(result2);
    while (g_variant_iter_loop(arr, "&s", &env_str)) {
        cloc->Locale[cloc->LocaleCnt] = strdup(env_str);
        if (!cloc->Locale[cloc->LocaleCnt]) goto error;
        cloc->LocaleCnt++;
    }
    g_variant_iter_free(arr);
    arr = NULL;

    if ((result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", LOCALE_INTERFACE, "VConsoleKeymap"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL)) == NULL)
        goto error;
    g_variant_get(result, "(v)", &result2);
    g_variant_unref(result);
    g_variant_get(result2, "s", &value_str);
    g_variant_unref(result2);
    cloc->VConsoleKeymap = value_str;
    if (!cloc->VConsoleKeymap) goto error;

    if ((result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", LOCALE_INTERFACE, "VConsoleKeymapToggle"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL)) == NULL)
        goto error;
    g_variant_get(result, "(v)", &result2);
    g_variant_unref(result);
    g_variant_get(result2, "s", &value_str);
    g_variant_unref(result2);
    cloc->VConsoleKeymapToggle = value_str;
    if (!cloc->VConsoleKeymapToggle) goto error;

    if ((result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", LOCALE_INTERFACE, "X11Layout"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL)) == NULL)
        goto error;
    g_variant_get(result, "(v)", &result2);
    g_variant_unref(result);
    g_variant_get(result2, "s", &value_str);
    g_variant_unref(result2);
    cloc->X11Layouts = value_str;
    if (!cloc->X11Layouts) goto error;

    if ((result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", LOCALE_INTERFACE, "X11Model"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL)) == NULL)
        goto error;
    g_variant_get(result, "(v)", &result2);
    g_variant_unref(result);
    g_variant_get(result2, "s", &value_str);
    g_variant_unref(result2);
    cloc->X11Model = value_str;
    if (!cloc->X11Model) goto error;

    if ((result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", LOCALE_INTERFACE, "X11Variant"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL)) == NULL)
        goto error;
    g_variant_get(result, "(v)", &result2);
    g_variant_unref(result);
    g_variant_get(result2, "s", &value_str);
    g_variant_unref(result2);
    cloc->X11Variant = value_str;
    if (!cloc->X11Variant) goto error;

    if ((result = g_dbus_proxy_call_sync(proxy, "Get", g_variant_new("(ss)", LOCALE_INTERFACE, "X11Options"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL)) == NULL)
        goto error;
    g_variant_get(result, "(v)", &result2);
    g_variant_unref(result);
    g_variant_get(result2, "s", &value_str);
    g_variant_unref(result2);
    cloc->X11Options = value_str;
    if (!cloc->X11Options) goto error;

    g_object_unref(proxy);
    return cloc;

error:
    if (proxy) g_object_unref(proxy);
    if (cloc) locale_free(cloc);

    return NULL;
}


void locale_free(CimLocale *cloc)
{
    int i;

    if (!cloc) return;

    for (i = 0; i < cloc->LocaleCnt; i++) g_free(cloc->Locale[i]);
    g_free(cloc->VConsoleKeymap);
    g_free(cloc->VConsoleKeymapToggle);
    g_free(cloc->X11Layouts);
    g_free(cloc->X11Model);
    g_free(cloc->X11Variant);
    g_free(cloc->X11Options);
    free(cloc);

    return;
}


// The user_interaction boolean parameter (last parameter in SetLocale, SetVConsoleKeyboard
// and SetX11Keyboard) can be used to control whether PolicyKit should interactively ask the user
// for authentication credentials if it needs to - we don't want it, so it's hardcoded to FALSE

int set_locale(
    char *locale[],
    int locale_cnt)
{
    GDBusProxy *proxy = NULL;
    GError *gerr = NULL;
    GVariantBuilder *builder;

    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, LOCALE_NAME, LOCALE_OP, LOCALE_INTERFACE, NULL, &gerr);
    if (!proxy) goto error;

    builder = g_variant_builder_new(G_VARIANT_TYPE ("as"));
    for (int i = 0; i < locale_cnt; i++)
        g_variant_builder_add(builder, "s", locale[i]);

    g_dbus_proxy_call_sync(proxy, "SetLocale",
        g_variant_new("(asb)", builder, FALSE),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &gerr);
    g_variant_builder_unref(builder);
    if (gerr) goto error;

    return 0;

error:
    g_clear_error(&gerr);
    if (proxy) g_object_unref(proxy);
    return 1;
}


int set_v_console_keyboard(
    const char *keymap,
    const char* keymap_toggle,
    int convert)
{
    GDBusProxy *proxy = NULL;
    GError *gerr = NULL;

    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, LOCALE_NAME, LOCALE_OP, LOCALE_INTERFACE, NULL, &gerr);
    if (!proxy) goto error;

    gerr = NULL;
    // SetVConsoleKeyboard() instantly applies the new keymapping to the console
    g_dbus_proxy_call_sync(proxy, "SetVConsoleKeyboard",
        g_variant_new("(ssbb)", keymap, keymap_toggle, convert, FALSE),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &gerr);
    if (gerr) goto error;

    return 0;

error:
    g_clear_error(&gerr);
    if (proxy) g_object_unref(proxy);
    return 1;
}


int set_x11_keyboard(
    const char *layouts,
    const char *model,
    const char *variant,
    const char *options,
    int convert)
{
    GDBusProxy *proxy = NULL;
    GError *gerr = NULL;

    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL, LOCALE_NAME, LOCALE_OP, LOCALE_INTERFACE, NULL, &gerr);
    if (!proxy) goto error;

    gerr = NULL;
    // SetX11Keyboard() simply sets a default that may be used by later sessions.
    g_dbus_proxy_call_sync(proxy, "SetX11Keyboard",
        g_variant_new("(ssssbb)", layouts, model, variant, options, convert, FALSE),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &gerr);
    if (gerr) goto error;

    return 0;

error:
    g_clear_error(&gerr);
    if (proxy) g_object_unref(proxy);
    return 1;
}
