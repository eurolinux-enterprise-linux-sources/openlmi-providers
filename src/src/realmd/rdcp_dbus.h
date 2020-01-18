#ifndef __RDCP_DBUS_H__
#define __RDCP_DBUS_H__

#include <glib.h>
#include <dbus/dbus.h>
#include "realm-dbus-constants.h"

#define VERBOSE
//#define TRACE_VARIANT

/*----------------------------------------------------------------------------*/

extern DBusConnection* system_bus;

/*----------------------------------------------------------------------------*/

gboolean
get_dbus_string_property(DBusConnection *bus, const char *object_path,
                         const char* interface, const char *property,
                         char **value_return, GError **g_error);
gboolean
get_dbus_properties(DBusConnection *bus, const char *object_path,
                    const char* interface, GVariant **properties_return,
                    GError **g_error);

gboolean
dbus_discover_call(DBusConnection *bus, const char *target, GVariant *options,
                   gint32 *relevance_return, gchar ***paths_return, GError **g_error);

gboolean
dbus_change_login_policy_call(DBusConnection *bus, const gchar *dbus_path, const char *login_policy,
                              GVariant *permitted_add, GVariant *permitted_remove,
                              GVariant *options, GError **g_error);
gboolean
dbus_join_call(DBusConnection *bus, const gchar *dbus_path,
               GVariant *credentials, GVariant *options, GError **g_error);

gboolean
dbus_leave_call(DBusConnection *bus, const gchar *dbus_path,
                GVariant *credentials, GVariant *options, GError **g_error);

char *
get_short_dbus_interface_name(const char *interface);

gboolean
rdcp_dbus_initialize(GError **g_error);

#ifdef RDCP_DEBUG
#define PRINT_DBUS_PROPERTIES(dbus_props, dbus_path, dbus_interface)    \
    print_properties(dbus_props, "%s: Properties for %s, interface=%s", \
                     __FUNCTION__, dbus_path, dbus_interface);
#else
#define PRINT_DBUS_PROPERTIES(dbus_properties, dbus_path, dbus_interface)
#endif

#define GET_DBUS_PROPERIES_OR_EXIT(dbus_props, dbus_path, dbus_interface, status) \
{                                                                       \
    if (dbus_props != NULL) {                                           \
        handle_g_error(&g_error, _cb, status, CMPI_RC_ERR_FAILED,       \
                       "get_dbus_properties failed, dbus_props was non-NULL (%s:%d)", \
                        __FILE__, __LINE__);                            \
        goto exit;                                                      \
    }                                                                   \
    if (!get_dbus_properties(system_bus, dbus_path, dbus_interface, &dbus_props, &g_error)) { \
        handle_g_error(&g_error, _cb, status, CMPI_RC_ERR_FAILED,       \
                       "get_dbus_properties failed, path=%s interface=%s", \
                       dbus_path, dbus_interface);                      \
        goto exit;                                                      \
    }                                                                   \
                                                                        \
    PRINT_DBUS_PROPERTIES(dbus_props, dbus_path, dbus_interface);       \
}

#endif /* __RDCP_DBUS_H__ */
