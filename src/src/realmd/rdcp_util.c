#include "rdcp_util.h"
#include "rdcp_dbus.h"

#define VERBOSE

const char *provider_name = "realmd";
const ConfigEntry *provider_config_defaults = NULL;

void
print_properties (GVariant *properties, gchar *format, ...)
{
    va_list args;
    GVariantClass class;
    GVariantIter iter;
    GVariant *value;
    gchar *key;
    gchar *value_as_string;
    gsize n_children, i;

    if (format) {
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    }

    g_variant_iter_init (&iter, properties);
    while (g_variant_iter_loop (&iter, "{sv}", &key, &value)) {
        class = g_variant_classify(value);

        if (class == G_VARIANT_CLASS_ARRAY) {
            n_children = g_variant_n_children(value);
            if (n_children == 0) {
                printf("    %s: []\n", key);

            } else {
                GVariant *child;

                printf("    %s: [\n", key);
                for (i = 0; i < n_children; i++) {
                    child = g_variant_get_child_value(value, i);
                    value_as_string = g_variant_print(child, TRUE);
                    printf("        %s", value_as_string);
                    g_free(value_as_string);
                    G_VARIANT_FREE(child);
                    if (i < n_children-1) {
                        printf("\n");
                    } else {
                        printf("]\n");
                    }
                }
            }
        } else {
            value_as_string = g_variant_print(value, TRUE);
            printf("    %s: %s\n", key, value_as_string);
            g_free(value_as_string);
        }
    }
    printf("\n");
}

void
print_paths(gchar **paths, gchar *format, ...) {
    va_list args;
    gchar *path, **pp;
    int i, n_items;

    pp = paths;
    for (path = *pp++, n_items = 0; path; path = *pp++, n_items++);

    if (format) {
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }

    printf(" [%d paths:]\n", n_items);

    pp = paths;
    for (path = *pp++, i = 0; path; path = *pp++, i++) {
        printf("  path[%d]: %s\n", i, path);
    }
}

/*----------------------------------------------------------------------------*/

/**
 * build_g_variant_options_from_KStringA
 * @keys An array of dictionary keys.
 * @values An array of dictionary values.
 * @g_variant_return Pointer to location where GVariant will be returned,
 * will be NULL if error.
 * @g_error initialized to error info when FALSE is returned.
 *
 * Builds a GVariant dictionay of Realmd options.
 *
 * The keys and values in the dict are strings. The keys and values
 * are passed as independent arrays, each value is paired with it's
 * key by using the same index into keys and values array. It is an
 * error if the length of the two arrays are not equal. If the length
 * of the keys & values array is zero then the dictionary will be
 * empty.
 *
 * If the option is not a string you must initialize the value for it
 * as a string. The string value will be parsed and converted to the
 * appropriate type for the option. Boolean values must be either
 * "TRUE" or "FALSE" (case insensitive).
 *
 * Returns: return TRUE if successful, @g_variant_return will be non-NULL.
 * FALSE if error with @g_error initialized, @g_variant_return will be NULL.
 */
gboolean
build_g_variant_options_from_KStringA(const KStringA *keys, const KStringA *values,
                                      GVariant **g_variant_return, GError **g_error)
{
    GVariantBuilder builder;
    GVariant *g_variant;
    CMPICount i, count;

    g_return_val_if_fail (keys != NULL, FALSE);
    g_return_val_if_fail (values != NULL, FALSE);
    g_return_val_if_fail (g_variant_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *g_variant_return = NULL;

    if (keys->count != values->count) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_INVALID_ARG,
                    "length of keys array (%d) != length of values array (%d)",
                    keys->count, values->count);
        return FALSE;
    }

    count = keys->count;

    g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);

    for (i = 0; i < count; i++) {
        const char *key;
        const char *value;

        key = KStringA_Get((KStringA *)keys, i);
        value = KStringA_Get((KStringA *)values, i);

        if (g_str_equal(key, "assume-packages")) {
            gboolean g_boolean;

            if (g_ascii_strcasecmp(value, "true") == 0) {
                g_boolean = TRUE;
            } else if (g_ascii_strcasecmp(value, "false") == 0) {
                g_boolean = FALSE;
            } else {
                g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_INVALID_ARG,
                            "invalid value for assume-packages option (%s), must be TRUE or FALSE", value);
                g_variant_builder_clear(&builder);
                return FALSE;
            }
            g_variant_builder_add_parsed (&builder, "{%s, <%b>}", key, g_boolean);
        } else {
            g_variant_builder_add_parsed (&builder, "{%s, <%s>}", key, value);
        }
    }

    if ((g_variant = g_variant_builder_end(&builder)) == NULL) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "unable to build GVariant options array");
        return FALSE;
    }

    *g_variant_return = g_variant;
    return TRUE;
}

/**
 * build_g_variant_string_array_from_KStringA:
 * @values The KStringA source array
 * @g_variant_return Pointer to location where GVariant will be returned,
 * will be NULL if error.
 * @g_error initialized to error info when FALSE is returned.
 *
 * Builds a GVariant array of strings "as" from a KStringA.
 *
 * Returns: return TRUE if successful, @g_variant_return will be non-NULL.
 * FALSE if error with @g_error initialized, @g_variant_return will be NULL.
 */
gboolean
build_g_variant_string_array_from_KStringA(const KStringA *values,
                                           GVariant **g_variant_return, GError **g_error)
{
    GVariantBuilder builder;
    GVariant *g_variant;
    CMPICount i, count;

    g_return_val_if_fail (values != NULL, FALSE);
    g_return_val_if_fail (g_variant_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *g_variant_return = NULL;

    count = values->count;

    g_variant_builder_init(&builder,  G_VARIANT_TYPE_STRING_ARRAY);

    for (i = 0; i < count; i++) {
        const char *value;

        value = KStringA_Get((KStringA *)values, i);
        g_variant_builder_add (&builder, "s", value);
    }

    if ((g_variant = g_variant_builder_end(&builder)) == NULL) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "unable to build GVariant options array");
        return FALSE;
    }

    *g_variant_return = g_variant;
    return TRUE;
}

/*----------------------------------------------------------------------------*/

/**
 * dbus_path_from_instance_id
 * @instance_id A CIM Class InstanceID with a dbus_path encapsulated inside
 * @dbus_path_return Pointer to location where DBus object path will be returned,
 * will be NULL if error occurs. Must be freed with g_free()
 * @g_error initialized to error info when FALSE is returned.
 *
 * CIM class instances bound to a DBus object embed the DBus object
 * path into their InstanceID. This is one element contributing to the
 * InstanceID's uniqueness. It also allows us to retrieve the DBus
 * object path in order to operate on the matching DBus object.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized
 */
gboolean
dbus_path_from_instance_id(const char *instance_id, gchar **dbus_path_return, GError **g_error)
{
    gchar *dbus_path = NULL;

    g_return_val_if_fail (instance_id != NULL, FALSE);
    g_return_val_if_fail (dbus_path_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *dbus_path_return = NULL;

    if ((dbus_path = strchr(instance_id, ':')) != NULL) {
        dbus_path++;            /* skip ':' */
        *dbus_path_return = g_strdup(dbus_path);
        return TRUE;
    } else {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_INVALID_INSTANCE_ID,
                    "could not locate DBus path in CIM InstanceID = \"%s\"",
                    instance_id);
        return FALSE;
    }
}

/**
 * instance_id_from_dbus_path
 * @dbus_path The DBus object path to encode in the CIM InstanceID
 *
 * Returns: InstanceID, must be freed with g_free()
 */
gchar *
instance_id_from_dbus_path(const char *dbus_path)
{
    GString *g_instance_id = NULL;

    g_instance_id = g_string_new(NULL);
    g_string_printf(g_instance_id, "%s:%s", LMI_ORGID, dbus_path);
    return g_string_free(g_instance_id, FALSE);
}

/**
 * get_data_from_KUint8A
 *
 * @ka KUint8 Array containing binary data
 * @size_return Length of the returned buffer is returned here if non-NULL.
 *
 * Binary data passed in a CMPI Uint8 array cannot be accessed as a
 * pointer to the data. Instead each element of the array must be read
 * and inserted into a contiguous buffer.
 *
 * Returns: data buffer, must be freed with g_free()
 */
gchar *
get_data_from_KUint8A(const KUint8A *ka, gsize *size_return)
{
    gsize n_octets;
    gchar *buffer, *p;
    CMPICount count, i;
    KUint8 octet;

    count = ka->count;
    n_octets = count;

    if ((buffer = g_malloc(n_octets)) == NULL) {
        if (size_return)
            *size_return = 0;
        return NULL;
    }

    for (i = 0, p = buffer; i < count; i++) {
        octet = KUint8A_Get(ka, i);
        *p++ = octet.value;
    }

    if (size_return)
        *size_return = n_octets;

    return buffer;
}
