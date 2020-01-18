#ifndef __RDCP_UTIL_H__
#define __RDCP_UTIL_H__

#include <stdarg.h>
#include <stdbool.h>

#include <konkret/konkret.h>
#include <glib.h>

#include "rdcp_error.h"
#include "openlmi.h"
#include "globals.h"

const char *provider_name;
const ConfigEntry *provider_config_defaults;

#define REALMD_SERVICE_NAME "OpenLMI Realmd Service"


#define G_VARIANT_FREE(variant)                 \
{                                               \
    if (variant) {                              \
        g_variant_unref(variant);               \
        variant = NULL;                         \
    }                                           \
}

#define G_VARIANT_ITER_FREE(iter)               \
{                                               \
    if (iter) {                                 \
        g_variant_iter_free(iter);              \
        iter = NULL;                            \
    }                                           \
}

/**
 * octetstring_parse
 * @octetstring Pointer to octetstring data
 * @data_len_return Pointer to uint32 value which receives the number
 * of octets in the octetstring buffer.
 *
 * Given an octetstring, extract it's length and set the pointer to
 * the data.
 *
 * When specified on elements of type array of uint8, the OctetString
 * qualifier indicates that the entire array represents a single octet
 * string. The first four array entries shall represent a length
 * field, and any subsequent entries shall represent the octets in the
 * octet string. The four uint8 values in the length field shall be
 * interpreted as a 32-bit unsigned number where the first array entry
 * is the most significant byte.  The number represented by the length
 * field shall be the number of octets in the octet string plus
 * four. For example, the empty octet string is represented as { 0x00,
 * 0x00, 0x00, 0x04 }.

 *
 */

KINLINE unsigned char *octetstring_parse(unsigned char *octetstring, CMPIUint32 *data_len)
{
    unsigned char *data = NULL;
    *data_len = 0;
    if (octetstring) {
        *data_len = (octetstring[0] << 24) |
                    (octetstring[1] << 16) |
                    (octetstring[2] <<  8) |
                    (octetstring[3]);
        *data_len -= 4;
        data = octetstring + 4;
    }
    return data;
}

#define LMI_InitRealmdServiceKeys(klass, obj, name_space, host_name)    \
{                                                                       \
    klass##_Init(obj, _cb, name_space);                                 \
    klass##_Set_Name(obj, REALMD_SERVICE_NAME);                         \
    klass##_Set_SystemCreationClassName(obj,                            \
                                        get_system_creation_class_name()); \
    klass##_Set_SystemName(obj, host_name);                             \
    klass##_Set_CreationClassName(obj,                                  \
                                  LMI_RealmdService_ClassName);         \
                                                                        \
}

void
print_properties (GVariant *properties, gchar *format, ...)
__attribute__ ((format (printf, 2, 3)));

void
print_paths(gchar **paths, gchar *format, ...)
__attribute__ ((format (printf, 2, 3)));

gboolean
build_g_variant_options_from_KStringA(const KStringA *keys, const KStringA *values,
                                      GVariant **g_variant_return, GError **g_error);

gboolean
build_g_variant_string_array_from_KStringA(const KStringA *values,
                                           GVariant **g_variant_return, GError **g_error);

gboolean
dbus_path_from_instance_id(const char *instance_id, gchar **dbus_path_return, GError **g_error);

gchar *
instance_id_from_dbus_path(const char *dbus_path);

gchar *
get_data_from_KUint8A(const KUint8A *ka, gsize *size_return);

#endif /* __RDCP_UTIL_H__ */
