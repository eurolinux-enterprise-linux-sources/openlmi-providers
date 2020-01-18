#include <konkret/konkret.h>

#include "rdcp_error.h"
#include "realm-dbus-constants.h"

#include "rdcp_dbus.h"
#include "rdcp_util.h"

/*----------------------------------------------------------------------------*/

DBusConnection* system_bus = NULL;

/*----------------------------------------------------------------------------*/
#ifdef TRACE_VARIANT
static const char *
dbus_type_to_string(int type);
#endif

static GError *
dbus_error_to_gerror(DBusError *dbus_error);

static const char*
dbus_msg_type_to_string (int message_type);

static void
dbus_message_print_indent(GString *string, int depth);

static void
dbus_message_print_hex(GString *string, unsigned char *bytes, unsigned int len, int depth);

static void
dbus_message_print_byte_array(GString *string, DBusMessageIter *iter, int depth);

static void
dbus_message_print_iter(GString *string, DBusMessageIter *iter, int depth);

static GString *
dbus_message_print_string(DBusMessage *message, GString *string, dbus_bool_t show_header);

static gchar *
dbus_message_print(DBusMessage *message, GString *string, dbus_bool_t show_header);

static gboolean
append_g_variant_to_dbus_msg_iter(DBusMessageIter *iter, GVariant *value, GError **g_error);

static gboolean
append_g_variant_to_dbus_message(DBusMessage  *message, GVariant *g_variant, GError **g_error);

static gboolean
dbus_iter_to_variant(DBusMessageIter *iter, GVariant **g_variant_return, GError **g_error);

static gboolean
dbus_message_to_g_variant(DBusMessage *msg, GVariant **g_variant_return, GError **g_error);

static gboolean
dbus_method_reply_to_g_variant_tuple(DBusMessage *msg, GVariant **g_variant_return, GError **g_error);

gboolean
get_dbus_string_property(DBusConnection *bus, const char *object_path,
                         const char* interface, const char *property,
                         char **value_return, GError **g_error);
gboolean
get_dbus_properties(DBusConnection *bus, const char *object_path,
                    const char* interface, GVariant **properties_return,
                    GError **g_error);

static gboolean
dbus_discover_marshal(const char* target, GVariant *options,
                      DBusMessage **msg_return, GError **g_error);

static gboolean
dbus_discover_unmarshal(DBusMessage *reply, gint32 *relevance_return, gchar ***paths_return, GError **g_error);

/*----------------------------------------------------------------------------*/

#define RETURN_DBUS_ERROR(gerror, dbus_error)           \
{                                                       \
    if (gerror) {                                       \
        *gerror = dbus_error_to_gerror(&dbus_error);    \
    }                                                   \
    dbus_error_free(&dbus_error);                       \
    return FALSE;                                       \
}

GError *
dbus_error_to_gerror(DBusError *dbus_error)
{
    GError *gerror = NULL;

    if (dbus_error == NULL) {
        g_set_error(&gerror, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "dbus error not provided");
    } else {
        g_set_error(&gerror, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "dbus error (%s): %s", dbus_error->name, dbus_error->message);
    }

    return gerror;
}

/*----------------------------------------------------------------------------*/

/*
 * The code to print a DBus message is based upon DBUS code in
 * dbus/tools/dbus-print-message.c which is GPL.
 */

static const char*
dbus_msg_type_to_string (int message_type)
{
  switch (message_type)
    {
    case DBUS_MESSAGE_TYPE_SIGNAL:
      return "signal";
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
      return "method call";
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      return "method return";
    case DBUS_MESSAGE_TYPE_ERROR:
      return "error";
    default:
      return "(unknown message type)";
    }
}

#define INDENT_STRING "  "
#define INDENT_WIDTH sizeof(INDENT_STRING)
#define PAGE_WIDTH 80

static void
dbus_message_print_indent(GString *string, int depth)
{
  while (depth-- > 0)
    g_string_append(string, INDENT_STRING);
}

static void
dbus_message_print_hex(GString *string, unsigned char *bytes, unsigned int len, int depth)
{
  unsigned int i, columns;

  g_string_append_printf(string, "array of bytes [\n");

  dbus_message_print_indent (string, depth + 1);

  /* Each byte takes 3 cells (two hexits, and a space), except the last one. */
  columns = (PAGE_WIDTH - ((depth + 1) * INDENT_WIDTH)) / 3;

  if (columns < 8)
    columns = 8;

  i = 0;

  while (i < len) {
      g_string_append_printf(string, "%02x", bytes[i]);
      i++;

      if (i != len) {
          if (i % columns == 0) {
              g_string_append(string, "\n");
              dbus_message_print_indent(string, depth + 1);
          } else {
              g_string_append(string, " ");
          }
      }
  }

  g_string_append(string, "\n");
  dbus_message_print_indent(string, depth);
  g_string_append(string, "]\n");
}

#define DEFAULT_SIZE 100

static void
dbus_message_print_byte_array(GString *string, DBusMessageIter *iter, int depth)
{
  unsigned char *bytes = malloc (DEFAULT_SIZE + 1);
  unsigned int len = 0;
  unsigned int max = DEFAULT_SIZE;
  dbus_bool_t all_ascii = TRUE;
  int current_type;

  if (!bytes)
      return;

  while ((current_type = dbus_message_iter_get_arg_type (iter)) != DBUS_TYPE_INVALID) {
      unsigned char val;

      dbus_message_iter_get_basic (iter, &val);
      bytes[len] = val;
      len++;

      if (val < 32 || val > 126)
        all_ascii = FALSE;

      if (len == max) {
          unsigned char *tmp_bytes;

          max *= 2;
          if (!(tmp_bytes = realloc(bytes, max + 1)))
              goto fail;
          bytes = tmp_bytes;
      }

      dbus_message_iter_next (iter);
    }

  if (all_ascii) {
      bytes[len] = '\0';
      g_string_append_printf(string, "array of bytes \"%s\"\n", bytes);
  } else {
      dbus_message_print_hex(string, bytes, len, depth);
  }

fail:
  free (bytes);
}

static void
dbus_message_print_iter(GString *string, DBusMessageIter *iter, int depth)
{
    int type;

    while ((type = dbus_message_iter_get_arg_type (iter)) != DBUS_TYPE_INVALID) {

        dbus_message_print_indent(string, depth);

        switch (type) {
	case DBUS_TYPE_BOOLEAN: {
	    dbus_bool_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    g_string_append_printf(string, "boolean %s\n", val ? "true" : "false");
        } break;

	case DBUS_TYPE_BYTE: {
	    unsigned char val;
	    dbus_message_iter_get_basic (iter, &val);
	    g_string_append_printf(string, "byte %d\n", val);
        } break;

	case DBUS_TYPE_INT16: {
	    dbus_int16_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    g_string_append_printf(string, "int16 %" G_GINT16_FORMAT "\n", val);
        } break;

	case DBUS_TYPE_UINT16: {
	    dbus_uint16_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    g_string_append_printf(string, "uint16 %" G_GUINT16_FORMAT "\n", val);
        } break;

	case DBUS_TYPE_INT32: {
	    dbus_int32_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    g_string_append_printf(string, "int32 %" G_GINT32_FORMAT "\n", val);
        } break;

	case DBUS_TYPE_UINT32: {
	    dbus_uint32_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    g_string_append_printf(string, "uint32 %" G_GUINT32_FORMAT "\n", val);
        } break;

	case DBUS_TYPE_INT64: {
	    dbus_int64_t val;
	    dbus_message_iter_get_basic (iter, &val);
            g_string_append_printf(string, "int64 %" G_GINT64_FORMAT "\n", val);
        } break;

	case DBUS_TYPE_UINT64: {
	    dbus_uint64_t val;
	    dbus_message_iter_get_basic (iter, &val);
            g_string_append_printf(string, "uint64 %" G_GUINT64_FORMAT "\n", val);
        } break;

	case DBUS_TYPE_DOUBLE: {
	    double val;
	    dbus_message_iter_get_basic (iter, &val);
	    g_string_append_printf(string, "double %g\n", val);
        } break;

        case DBUS_TYPE_STRING: {
            char *val;
            dbus_message_iter_get_basic (iter, &val);
            g_string_append_printf(string, "string \"%s\"\n", val);
        } break;

	case DBUS_TYPE_OBJECT_PATH: {
	    char *val;
	    dbus_message_iter_get_basic (iter, &val);
            g_string_append_printf(string, "object path \"%s\"\n", val);
        } break;

        case DBUS_TYPE_SIGNATURE: {
            char *val;
            dbus_message_iter_get_basic (iter, &val);
            g_string_append_printf(string, "signature \"%s\"\n", val);
        } break;

	case DBUS_TYPE_UNIX_FD: {
	    dbus_uint32_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    g_string_append_printf(string, "Unix FD %" G_GUINT32_FORMAT "\n", val);
        } break;

	case DBUS_TYPE_ARRAY: {
	    int current_type;
	    DBusMessageIter subiter;

	    dbus_message_iter_recurse (iter, &subiter);

	    current_type = dbus_message_iter_get_arg_type (&subiter);

	    if (current_type == DBUS_TYPE_BYTE) {
                dbus_message_print_byte_array(string, &subiter, depth);
		break;
            }

	    g_string_append(string, "array [\n");
	    while (current_type != DBUS_TYPE_INVALID) {
		dbus_message_print_iter(string, &subiter, depth+1);

		dbus_message_iter_next (&subiter);
		current_type = dbus_message_iter_get_arg_type (&subiter);

		if (current_type != DBUS_TYPE_INVALID)
                    g_string_append(string, ",");
            }
	    dbus_message_print_indent(string, depth);
	    g_string_append(string, "]\n");
        } break;

	case DBUS_TYPE_VARIANT: {
	    DBusMessageIter subiter;

	    dbus_message_iter_recurse (iter, &subiter);

	    g_string_append(string, "variant ");
	    dbus_message_print_iter(string, &subiter, depth+1);
        } break;

	case DBUS_TYPE_STRUCT: {
	    int current_type;
	    DBusMessageIter subiter;

	    dbus_message_iter_recurse (iter, &subiter);

	    g_string_append(string, "struct {\n");
	    while ((current_type = dbus_message_iter_get_arg_type (&subiter)) != DBUS_TYPE_INVALID) {
		dbus_message_print_iter(string, &subiter, depth+1);
		dbus_message_iter_next (&subiter);
		if (dbus_message_iter_get_arg_type (&subiter) != DBUS_TYPE_INVALID)
                    g_string_append(string, ",");
            }
	    dbus_message_print_indent(string, depth);
	    g_string_append(string, "}\n");
        } break;

	case DBUS_TYPE_DICT_ENTRY: {
	    DBusMessageIter subiter;

	    dbus_message_iter_recurse (iter, &subiter);

	    g_string_append(string, "dict entry(\n");
	    dbus_message_print_iter(string, &subiter, depth+1);
	    dbus_message_iter_next (&subiter);
	    dbus_message_print_iter(string, &subiter, depth+1);
	    dbus_message_print_indent(string, depth);
	    g_string_append(string, ")\n");
        } break;

        default:
            g_string_append_printf(string, " (unknown arg type '%c')\n", type);
            break;
        }
        dbus_message_iter_next(iter);
    }
}

/**
 * dbus_message_print_string:
 * @message The DBusMessage to format into a string.
 * @string If non-NULL appends to this GString.
 * @show_header If #TRUE the message header will be included.
 *
 * Formats a DBusMessage into a string.
 *
 * Returns: A GString which must be freed with g_string_free()
 */
static GString *
dbus_message_print_string(DBusMessage *message, GString *string, dbus_bool_t show_header)
{
    DBusMessageIter iter;
    const char *sender;
    const char *destination;
    int message_type;

    g_return_val_if_fail (message != NULL, NULL);

    if (string == NULL) {
        string = g_string_new(NULL);
    }

    message_type = dbus_message_get_type (message);
    sender = dbus_message_get_sender (message);
    destination = dbus_message_get_destination (message);

    if (show_header) {
        g_string_append_printf(string, "%s sender=%s -> dest=%s",
                               dbus_msg_type_to_string (message_type),
                               sender ? sender : "(null sender)",
                               destination ? destination : "(null destination)");

        switch (message_type) {
        case DBUS_MESSAGE_TYPE_METHOD_CALL:
        case DBUS_MESSAGE_TYPE_SIGNAL:
            g_string_append_printf(string, " serial=%u path=%s; interface=%s; member=%s\n",
                                   dbus_message_get_serial (message),
                                   dbus_message_get_path (message),
                                   dbus_message_get_interface (message),
                                   dbus_message_get_member (message));
            break;

        case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            g_string_append_printf(string, " reply_serial=%u\n",
                                   dbus_message_get_reply_serial (message));
            break;

        case DBUS_MESSAGE_TYPE_ERROR:
            g_string_append_printf(string, " error_name=%s reply_serial=%u\n",
                                   dbus_message_get_error_name (message),
                                   dbus_message_get_reply_serial (message));
            break;

        default:
            g_string_append(string, "\n");
            break;
        }
    }

    dbus_message_iter_init(message, &iter);
    dbus_message_print_iter(string, &iter, 1);

    return string;
}

/**
 * dbus_message_print_string:
 * @message The DBusMessage to format into a string.
 * @string If non-NULL appends to this GString.
 * @show_header If #TRUE the message header will be included.
 *
 * Formats a DBusMessage into a string.
 *
 * Returns: A simple malloc'ed string which must be freed with g_free().
 */
static gchar *
dbus_message_print(DBusMessage *message, GString *string, dbus_bool_t show_header)
{
    g_return_val_if_fail (message != NULL, NULL);

    return g_string_free(dbus_message_print_string(message, NULL, show_header), FALSE);
}

/*----------------------------------------------------------------------------*/

/**
 * append_g_variant_to_dbus_msg_iter:
 * @g_error initialized to error info when FALSE is returned.
 *
 * Helper routine for append_g_variant_to_dbus_message().
 * Performs the recusive descent into the GVariant appending
 * values to the DBusMessage as it goes.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized.
 */
static gboolean
append_g_variant_to_dbus_msg_iter(DBusMessageIter *iter, GVariant *value, GError **g_error)
{
    GVariantClass class;

    g_return_val_if_fail (iter != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    class = g_variant_classify(value);

    switch (class) {
    case G_VARIANT_CLASS_BOOLEAN: {
        dbus_bool_t v = g_variant_get_boolean(value);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &v);
    } break;
    case G_VARIANT_CLASS_BYTE: {
        guint8 v = g_variant_get_byte(value);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_BYTE, &v);
    } break;
    case G_VARIANT_CLASS_INT16: {
        gint16 v = g_variant_get_int16 (value);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_INT16, &v);
    } break;
    case G_VARIANT_CLASS_UINT16: {
        guint16 v = g_variant_get_uint16(value);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT16, &v);
    } break;
    case G_VARIANT_CLASS_INT32: {
        gint32 v = g_variant_get_int32(value);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &v);
    } break;
    case G_VARIANT_CLASS_UINT32: {
        guint32 v = g_variant_get_uint32(value);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &v);
    } break;
    case G_VARIANT_CLASS_INT64: {
        gint64 v = g_variant_get_int64(value);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_INT64, &v);
    } break;
    case G_VARIANT_CLASS_UINT64: {
        guint64 v = g_variant_get_uint64(value);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &v);
    } break;
    case G_VARIANT_CLASS_HANDLE: {
        gint32 v = g_variant_get_handle(value);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &v);
    } break;
    case G_VARIANT_CLASS_DOUBLE: {
        gdouble v = g_variant_get_double(value);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_DOUBLE, &v);
    } break;
    case G_VARIANT_CLASS_STRING: {
        const gchar *v = g_variant_get_string(value, NULL);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &v);
    } break;
    case G_VARIANT_CLASS_OBJECT_PATH: {
        const gchar *v = g_variant_get_string(value, NULL);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &v);
    } break;
    case G_VARIANT_CLASS_SIGNATURE: {
        const gchar *v = g_variant_get_string(value, NULL);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_SIGNATURE, &v);
    } break;
    case G_VARIANT_CLASS_VARIANT: {
        DBusMessageIter sub;
        GVariant *child;

        child = g_variant_get_child_value(value, 0);
        dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
                                         g_variant_get_type_string(child),
                                         &sub);
        if (!append_g_variant_to_dbus_msg_iter(&sub, child, g_error)) {
            G_VARIANT_FREE(child);
            goto fail;
        }
        dbus_message_iter_close_container(iter, &sub);
        G_VARIANT_FREE(child);
    } break;
    case G_VARIANT_CLASS_MAYBE: {
        GVariant *child;

        if (!g_variant_n_children(value)) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "cannot serialize an empty GVariant MAYBE");
            goto fail;
        } else {
            child = g_variant_get_child_value(value, 0);
            if (!append_g_variant_to_dbus_msg_iter(iter, child, g_error)) {
                G_VARIANT_FREE(child);
                goto fail;
            }
            G_VARIANT_FREE(child);
        }
    } break;
    case G_VARIANT_CLASS_ARRAY: {
        DBusMessageIter dbus_iter;
        const gchar *type_string;
        gsize n, i;
        GVariant *child;

        type_string = g_variant_get_type_string(value);
        type_string++; /* skip the 'a' */

        dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                                         type_string, &dbus_iter);

        n = g_variant_n_children(value);

        for (i = 0; i < n; i++) {
            child = g_variant_get_child_value(value, i);
            if (!append_g_variant_to_dbus_msg_iter(&dbus_iter, child, g_error)) {
                G_VARIANT_FREE(child);
                goto fail;
            }
            G_VARIANT_FREE(child);
        }

        dbus_message_iter_close_container(iter, &dbus_iter);
    } break;
    case G_VARIANT_CLASS_TUPLE: {
        DBusMessageIter dbus_iter;
        gsize n, i;
        GVariant *child;

        dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT,
                                         NULL, &dbus_iter);

        n = g_variant_n_children(value);

        for (i = 0; i < n; i++) {
            child = g_variant_get_child_value(value, i);
            if (!append_g_variant_to_dbus_msg_iter(&dbus_iter, child, g_error)) {
                G_VARIANT_FREE(child);
                goto fail;
            }
            G_VARIANT_FREE(child);
        }

        dbus_message_iter_close_container(iter, &dbus_iter);

    } break;
    case G_VARIANT_CLASS_DICT_ENTRY: {
        DBusMessageIter dbus_iter;
        GVariant *key, *val;

        dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY,
                                         NULL, &dbus_iter);
        key = g_variant_get_child_value(value, 0);
        if (!append_g_variant_to_dbus_msg_iter(&dbus_iter, key, g_error)) {
            G_VARIANT_FREE(key);
            goto fail;
        }
        G_VARIANT_FREE(key);

        val = g_variant_get_child_value(value, 1);
        if (!append_g_variant_to_dbus_msg_iter(&dbus_iter, val, g_error)) {
            G_VARIANT_FREE(val);
            goto fail;
        }
        G_VARIANT_FREE(val);

        dbus_message_iter_close_container(iter, &dbus_iter);
    } break;
    default: {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "Error serializing GVariant with class '%c' to a D-Bus message",
                    class);
        goto fail;
    } break;
    }

    return TRUE;

 fail:
    return FALSE;
}

/**
 * append_g_variant_to_dbus_message:
 * @message DBus message currently being built
 * @g_variant The GVariant item to be appended to @message
 * @g_error initialized to error info when FALSE is returned.
 *
 * Given a DBusMessage append the contents of the provied @g_variant to the message.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized.
 */
static gboolean
append_g_variant_to_dbus_message(DBusMessage  *message, GVariant *g_variant, GError **g_error)
{
    DBusMessageIter iter;

    g_return_val_if_fail (message != NULL, FALSE);
    g_return_val_if_fail (g_variant != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    dbus_message_iter_init_append(message, &iter);
    if (!append_g_variant_to_dbus_msg_iter(&iter, g_variant, g_error)) {
        return FALSE;
    }
    return TRUE;
}

/*----------------------------------------------------------------------------*/


/**
 * dbus_iter_to_variant:
 * @msg DBusMessage which will be converted to a GVariant
 * @g_variant_return Pointer to location where GVariant will be returned,
 * will be NULL if error occurs
 * @g_error initialized to error info when FALSE is returned.
 *
 * Helper routine for dbus_message_to_g_variant(),
 * Performs the recusive descent into the DBusMessage appending
 * values to the GVariant as it goes.
 *
 * Returns: return TRUE if successful, @g_variant_return will be non-NULL.
 * FALSE if error with @g_error initialized, @g_variant_return will be NULL.
 */
static gboolean
dbus_iter_to_variant(DBusMessageIter *iter, GVariant **g_variant_return, GError **g_error)
{
    gboolean result = TRUE;
    int arg_type;
    GVariant *g_variant = NULL;
    char *signature = NULL;
    GVariantBuilder builder;
    DBusMessageIter sub;

    g_return_val_if_fail (iter != NULL, FALSE);
    g_return_val_if_fail (g_variant_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *g_variant_return = NULL;

    arg_type = dbus_message_iter_get_arg_type(iter);

#ifdef TRACE_VARIANT
    signature = dbus_message_iter_get_signature(iter);
    printf("dbus_iter_to_variant enter: type=%s signature=%s\n",
           dbus_type_to_string(arg_type), signature);
    g_free(signature);
    signature = NULL;
#endif

    switch (arg_type) {
    case DBUS_TYPE_BOOLEAN: {
        dbus_bool_t value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_boolean(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant boolean value=%d", value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_BYTE: {
        guint8 value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_byte(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant byte value=%uc", value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_INT16: {
        gint16 value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_int16(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant int16 value=%" G_GINT16_FORMAT, value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_UINT16: {
        guint16 value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_uint16(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant uint16 value=%" G_GUINT16_FORMAT, value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_INT32: {
        gint32 value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_int32(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant int32 value=%" G_GINT32_FORMAT, value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_UINT32: {
        guint32 value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_uint32(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant uint32 value=%" G_GUINT32_FORMAT, value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_INT64: {
        gint64 value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_int64(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant int64 value=%" G_GINT64_FORMAT, value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_UINT64: {
        guint64 value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_uint64(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant uint64 value=%" G_GUINT64_FORMAT, value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_DOUBLE: {
        gdouble value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_double(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant double value=%f", value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_STRING: {
        gchar *value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_string(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant string value=\"%s\"", value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_OBJECT_PATH: {
        gchar *value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_object_path(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant object path value=\"%s\"", value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_SIGNATURE: {
        gchar *value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_signature(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant signature value=\"%s\"", value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_UNIX_FD: {
        guint32 value;

        dbus_message_iter_get_basic(iter, &value);
        if ((g_variant = g_variant_new_uint32(value)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant file descriptor value=%u", value);
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_ARRAY: {
        GVariant *item;

        signature = dbus_message_iter_get_signature(iter);

        g_variant_builder_init(&builder, G_VARIANT_TYPE(signature));
        dbus_message_iter_recurse(iter, &sub);

        while (dbus_message_iter_get_arg_type(&sub) != DBUS_TYPE_INVALID) {
            if (!dbus_iter_to_variant(&sub, &item, g_error)) {
                g_variant_builder_clear(&builder);
                result = FALSE;
                goto exit;
            }
#ifdef TRACE_VARIANT
            {
                gchar *variant_as_string = g_variant_print(item, TRUE);
                printf("array item=%s\n", variant_as_string);
                g_free(variant_as_string);
            }
#endif
            g_variant_builder_add_value(&builder, item);
            dbus_message_iter_next(&sub);
        }

        if ((g_variant = g_variant_builder_end(&builder)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant array");
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_VARIANT: {
        GVariant *item;

        signature = dbus_message_iter_get_signature(iter);
        g_variant_builder_init(&builder, G_VARIANT_TYPE(signature));
        dbus_message_iter_recurse(iter, &sub);

        if (!dbus_iter_to_variant(&sub, &item, g_error)) {
                g_variant_builder_clear(&builder);
                result = FALSE;
                goto exit;
        }
#ifdef TRACE_VARIANT
            {
                gchar *variant_as_string = g_variant_print(item, TRUE);
                printf("variant item=%s\n", variant_as_string);
                g_free(variant_as_string);
            }
#endif

        g_variant_builder_add_value(&builder, item);

        if ((g_variant = g_variant_builder_end(&builder)) == NULL) {
            gchar *variant_as_string = g_variant_print(item, FALSE);

            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant variant for value=%s", variant_as_string);
            g_free(variant_as_string);
            result = FALSE;
        }
    } break;
    case DBUS_TYPE_STRUCT: {
        GVariant *item;

        signature = dbus_message_iter_get_signature(iter);
        g_variant_builder_init(&builder, G_VARIANT_TYPE(signature));
        dbus_message_iter_recurse(iter, &sub);

        while (dbus_message_iter_get_arg_type(&sub) != DBUS_TYPE_INVALID) {
            if (!dbus_iter_to_variant(&sub, &item, g_error)) {
                g_variant_builder_clear(&builder);
                result = FALSE;
                goto exit;
            }
#ifdef TRACE_VARIANT
            {
                gchar *variant_as_string = g_variant_print(item, TRUE);
                printf("struct item=%s\n", variant_as_string);
                g_free(variant_as_string);
            }
#endif
            g_variant_builder_add_value(&builder, item);
            dbus_message_iter_next(&sub);
        }

        if ((g_variant = g_variant_builder_end(&builder)) == NULL) {
            g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                        "unable to create GVariant struct");
            result = FALSE;
        }

    } break;
    case DBUS_TYPE_DICT_ENTRY: {
        GVariant *key, *value;

        dbus_message_iter_recurse(iter, &sub);
        signature = dbus_message_iter_get_signature(iter);

        if (!dbus_iter_to_variant(&sub, &key, g_error)) {
            g_prefix_error(g_error, "unable to create GVariant dict_entry key: ");
            result = FALSE;
            goto exit;
        }

        dbus_message_iter_next(&sub);

        if (!dbus_iter_to_variant(&sub, &value, g_error)) {
            g_prefix_error(g_error, "unable to create GVariant dict_entry value: ");
            result = FALSE;
            goto exit;
        }
#ifdef TRACE_VARIANT
            {
                gchar *key_variant_as_string = g_variant_print(key, TRUE);
                gchar *value_variant_as_string = g_variant_print(key, TRUE);
                printf("dict_entry key=%s value=%s\n", g_variant_print(key, TRUE), g_variant_print(value, TRUE));
                g_free(key_variant_as_string);
                g_free(value_variant_as_string);
            }
#endif

        g_variant = g_variant_new_dict_entry(key, value);

    } break;
    default: {
        signature = dbus_message_iter_get_signature(iter);
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "unknown DBus type=%d, signature=%s", arg_type, signature);
        result = FALSE;
        break;
    }
    }

 exit:
    if (signature) dbus_free(signature);
    *g_variant_return = g_variant;

#ifdef TRACE_VARIANT
    {
        gchar *variant_as_string = NULL;
        if (g_variant) {
            variant_as_string = g_variant_print(g_variant, TRUE);
        } else {
            variant_as_string = "NULL";
        }
        printf("dbus_iter_to_variant returns %s, variant=%s\n",
               result ? "TRUE" : "FALSE", variant_as_string);
        if (g_variant) {
            g_free(variant_as_string);
        }
    }
#endif

    return result;
}

/**
 * dbus_message_to_g_variant:
 * @msg DBusMessage which will be converted to a GVariant
 * @g_variant_return Pointer to location where GVariant will be returned,
 * will be NULL if error occurs
 * @g_error initialized to error info when FALSE is returned.
 *
 * Converts a DBusMessage to a GVariant.
 *
 * Returns: return TRUE if successful, @g_variant_return will be non-NULL.
 * FALSE if error with @g_error initialized, @g_variant_return will be NULL.
 */
static gboolean
dbus_message_to_g_variant(DBusMessage *msg, GVariant **g_variant_return, GError **g_error)
{
    DBusMessageIter iter;

    g_return_val_if_fail (msg != NULL, FALSE);
    g_return_val_if_fail (g_variant_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *g_variant_return = NULL;

#ifdef RDCP_DBUS_DEBUG
    {
        gchar *msg_as_string = dbus_message_print(msg, NULL, FALSE);
        printf("dbus_message_to_g_variant: msg=\n%s", msg_as_string);
        g_free(msg_as_string);
    }
#endif

    if (!dbus_message_iter_init(msg, &iter)) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "could not create iterator to parse DBus message");
        return FALSE;
    }

    if (!dbus_iter_to_variant(&iter, g_variant_return, g_error)) {
        g_prefix_error(g_error, "unable to convert dbus_message to GVariant: ");
        return FALSE;
    }

#ifdef RDCP_DBUS_DEBUG
    {
        gchar *variant_as_string = NULL;
        if (*g_variant_return) {
            variant_as_string = g_variant_print(*g_variant_return, TRUE);
        } else {
            variant_as_string = "NULL";
        }
        printf("dbus_message_to_g_variant returns variant=%s\n", variant_as_string);
        if (*g_variant_return) {
            g_free(variant_as_string);
        }
    }
#endif

    return TRUE;
}

/**
 * dbus_method_reply_to_g_variant_tuple:
 * @msg DBus message reply which will be converted to a tuple of GVariant's
 * @g_variant_return Pointer to location where GVariant will be returned,
 * will be NULL if error occurs
 * @g_error initialized to error info when FALSE is returned.
 *
 * A DBus method reply contains a sequence of zero or more OUT parameters.
 * Parse the method reply and build a GVariant tuple whose members are
 * the OUT parameters. Each tuple member will also be a GVariant.
 *
 * Returns: return TRUE if successful, @g_variant_return will be non-NULL.
 * FALSE if error with @g_error initialized, @g_variant_return will be NULL.
 */
static gboolean
dbus_method_reply_to_g_variant_tuple(DBusMessage *msg, GVariant **g_variant_return, GError **g_error)
{
    DBusMessageIter iter;
    GVariantBuilder builder;

    g_return_val_if_fail (msg != NULL, FALSE);
    g_return_val_if_fail (g_variant_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *g_variant_return = NULL;

#ifdef RDCP_DBUS_DEBUG
    {
        gchar *msg_as_string = dbus_message_print(msg, NULL, FALSE);
        printf("dbus_method_reply_to_g_variant_tuple: msg=\n%s", msg_as_string);
        g_free(msg_as_string);
    }
#endif

    g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);

    if (!dbus_message_iter_init(msg, &iter)) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "could not create iterator to parse DBus message");
        return FALSE;
    }

    while (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
        GVariant *g_variant = NULL;

        if (!dbus_iter_to_variant(&iter, &g_variant, g_error)) {
            g_prefix_error(g_error, "unable to convert dbus_message to GVariant: ");
            return FALSE;
        }

        g_variant_builder_add_value(&builder, g_variant);

        dbus_message_iter_next (&iter);
    }

    if ((*g_variant_return = g_variant_builder_end(&builder)) == NULL) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "unable to build GVariant options array");
        return FALSE;
    }


#ifdef RDCP_DBUS_DEBUG
    {
        gchar *variant_as_string = NULL;
        if (*g_variant_return) {
            variant_as_string = g_variant_print(*g_variant_return, TRUE);
        } else {
            variant_as_string = "NULL";
        }
        printf("dbus_method_reply_to_g_variant_tuple returns variant=%s\n", variant_as_string);
        if (*g_variant_return) {
            g_free(variant_as_string);
        }
    }
#endif

    return TRUE;
}

/*----------------------------------------------------------------------------*/

/**
 * get_dbus_string_property:
 * @bus The DBus connection on which the query will be performed
 * @object_path The DBus object path identifying the object
 * @interface The DBus interface which provides the property
 * @property The name of the proptery on the interface
 * @value_return Pointer to where string value will be returned.
 * Must be freed with g_free(), will be NULL if error occurs.
 * @g_error initialized to error info when FALSE is returned.
 *
 * Retrieve a string valued property from an DBus object.
 *
 * Returns: return TRUE if successful, @value_return will be non-NULL.
 * FALSE if error with @g_error initialized, @value_return will be NULL.
 */
gboolean
get_dbus_string_property(DBusConnection *bus, const char *object_path,
                         const char* interface, const char *property,
                         char **value_return, GError **g_error)
{
    const char *method = "Get";
    DBusMessage* msg = NULL;
    DBusMessage* reply = NULL;
    DBusError dbus_error;
    const char *interface_ptr = interface;
    const char *property_ptr = property;
    DBusMessageIter iter, variant;
    char *value = NULL;
    char *signature;

    g_return_val_if_fail (bus != NULL, FALSE);
    g_return_val_if_fail (object_path != NULL, FALSE);
    g_return_val_if_fail (interface != NULL, FALSE);
    g_return_val_if_fail (property != NULL, FALSE);
    g_return_val_if_fail (value_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *value_return = NULL;
    dbus_error_init(&dbus_error);

    if ((msg = dbus_message_new_method_call(REALM_DBUS_BUS_NAME, object_path,
                                            DBUS_INTERFACE_PROPERTIES, method)) == NULL) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "failed to create"
                    "DBus %s.%s() message, object_path=%s, interface=%s, property=%s",
                    DBUS_INTERFACE_PROPERTIES, method, object_path, interface, property);
        return FALSE;
    }

    if (!dbus_message_append_args(msg,
                                  DBUS_TYPE_STRING, &interface_ptr,
                                  DBUS_TYPE_STRING, &property_ptr,
                                  DBUS_TYPE_INVALID)) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "failed to add args to "
                    "DBus %s.%s() message, object_path=%s, interface=%s, property=%s",
                    DBUS_INTERFACE_PROPERTIES, method, object_path, interface, property);
        dbus_message_unref(msg);
        return FALSE;
    }

    if ((reply = dbus_connection_send_with_reply_and_block(bus, msg, -1, &dbus_error)) == NULL) {
        dbus_message_unref(msg);
        RETURN_DBUS_ERROR(g_error, dbus_error);
    }
    dbus_message_unref(msg);

    if (!dbus_message_has_signature(reply, "v")) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "expected variant in DBus %s.%s() reply, object_path=%s, interface=%s, property=%s",
                    DBUS_INTERFACE_PROPERTIES, method, object_path, interface, property);
        dbus_message_unref(reply);
        return FALSE;
    }

    if (!dbus_message_iter_init(reply, &iter)) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "could not create iterator to parse "
                    "DBus %s.%s() reply, object_path=%s, interface=%s, property=%s",
                    DBUS_INTERFACE_PROPERTIES, method, object_path, interface, property);
        dbus_message_unref(reply);
        return FALSE;
    }

    dbus_message_iter_recurse(&iter, &variant);
    signature = dbus_message_iter_get_signature(&variant);
    if (!g_str_equal(signature, DBUS_TYPE_STRING_AS_STRING)) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "expected string type variant but got \"%s\" signature instead for "
                    "DBus %s.%s() reply, object_path=%s, interface=%s, property=%s",
                    signature, DBUS_INTERFACE_PROPERTIES, method, object_path, interface, property);
        dbus_free(signature);
        dbus_message_unref(reply);
        return FALSE;
    }
    dbus_free(signature);
    dbus_message_iter_get_basic(&variant, &value);
    *value_return = g_strdup(value);

    dbus_message_unref(reply);
    return TRUE;
}

/**
 * get_dbus_properties:
 * @bus The DBus connection on which the query will be performed
 * @object_path The DBus object path identifying the object
 * @interface The DBus interface which provides the property
 * @properties_return Pointer to where a GVariant containing all
 * the interface's properties for the object  will be returned.
 * Must be freed with g_variant_unref(), will be NULL if error occurs.
 * @g_error initialized to error info when FALSE is returned.
 *
 * Retrieve all the interface properties from an DBus object.
 * Returned as a GVariant dictionary. Use g_variant_lookup() to
 * obtain a specific property in the dictionary.
 *
 * Returns: return TRUE if successful, @value_return will be non-NULL.
 * FALSE if error with @g_error initialized, @value_return will be NULL.
 */
gboolean
get_dbus_properties(DBusConnection *bus, const char *object_path,
                    const char* interface, GVariant **properties_return,
                    GError **g_error)
{
    const char *method = "GetAll";
    DBusMessage* msg = NULL;
    DBusMessage* reply = NULL;
    DBusError dbus_error;
    const char *interface_ptr = interface;

    g_return_val_if_fail (bus != NULL, FALSE);
    g_return_val_if_fail (object_path != NULL, FALSE);
    g_return_val_if_fail (interface != NULL, FALSE);
    g_return_val_if_fail (properties_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *properties_return = NULL;
    dbus_error_init(&dbus_error);

    if ((msg = dbus_message_new_method_call(REALM_DBUS_BUS_NAME, object_path,
                                            DBUS_INTERFACE_PROPERTIES, method)) == NULL) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "failed to create"
                    "DBus %s.%s() message, object_path=%s, interface=%s",
                    DBUS_INTERFACE_PROPERTIES, method, object_path, interface);
        return FALSE;
    }

    if (!dbus_message_append_args(msg,
                                  DBUS_TYPE_STRING, &interface_ptr,
                                  DBUS_TYPE_INVALID)) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "failed to add args to "
                    "DBus %s.%s() message, object_path=%s, interface=%s",
                    DBUS_INTERFACE_PROPERTIES, method, object_path, interface);
        dbus_message_unref(msg);
        return FALSE;
    }

    if ((reply = dbus_connection_send_with_reply_and_block(bus, msg, -1, &dbus_error)) == NULL) {
        dbus_message_unref(msg);
        RETURN_DBUS_ERROR(g_error, dbus_error);
    }
    dbus_message_unref(msg);

    if (!dbus_message_to_g_variant(reply, properties_return, g_error)) {
        dbus_message_unref(reply);
        return FALSE;
    }


    dbus_message_unref(reply);
    return TRUE;
}

/*----------------------------------------------------------------------------*/

/**
 * dbus_discover_marshal:
 * @target what to discover
 * @options dictionary of option {key,values}
 * @msg_return if successful DBus message returned here,
 * if error then this will be NULL.
 * @g_error initialized to error info when FALSE is returned.
 *
 * Marshal a realm Discover method call.
 *
 * Returns: return TRUE if successful, @msg_return will point to DBusMessage,
 * FALSE if error with @g_error initialized. @msg_return will be NULL.
 */
static gboolean
dbus_discover_marshal(const char* target, GVariant *options,
                      DBusMessage **msg_return, GError **g_error)
{
    const char *method = "Discover";
    DBusMessage *msg = NULL;
    DBusMessageIter iter;

    g_return_val_if_fail (target != NULL, FALSE);
    g_return_val_if_fail (options != NULL, FALSE);
    g_return_val_if_fail (msg_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *msg_return = NULL;

    if ((msg = dbus_message_new_method_call(REALM_DBUS_BUS_NAME, REALM_DBUS_SERVICE_PATH,
                                            REALM_DBUS_PROVIDER_INTERFACE, method)) == NULL) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "failed to create dbus method call %s.%s() message, object_path=%s",
                    REALM_DBUS_PROVIDER_INTERFACE, method, REALM_DBUS_SERVICE_PATH);
        return FALSE;
    }

    dbus_message_iter_init_append (msg, &iter); /* void return */

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &target)) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                           "failed to add target parameter (%s)", target);
        dbus_message_unref(msg);
        return FALSE;
    }

    if (!append_g_variant_to_dbus_message(msg, options, g_error)) {
        g_prefix_error(g_error, "unable to append GVariant options dictionary into %s.%s() message",
                       REALM_DBUS_PROVIDER_INTERFACE, method);
        dbus_message_unref(msg);
        return FALSE;
    }

    *msg_return = msg;
    return TRUE;
}

/**
 * dbus_discover_unmarshal:
 * @reply DBus method reply from Discover call
 * @relevance_return Pointer to returned relevance value
 * @paths_return Pointer to an array of object path strings,
 * must be freed with g_strfreev().
 * @g_error initialized to error info when FALSE is returned.
 *
 * Parses the DBus reply message from the Discover call and unpacks
 * the output paramters in the *_return parameters of this function.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized.
 */

static gboolean
dbus_discover_unmarshal(DBusMessage *reply, gint32 *relevance_return, gchar ***paths_return, GError **g_error)
{
    GVariant *g_variant_reply = NULL;

    g_return_val_if_fail (reply != NULL, FALSE);
    g_return_val_if_fail (relevance_return != NULL, FALSE);
    g_return_val_if_fail (paths_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    if (!dbus_method_reply_to_g_variant_tuple(reply, &g_variant_reply, g_error)) {
        gchar *reply_string = dbus_message_print(reply, NULL, FALSE);

        g_prefix_error(g_error, "unable convert reply (%s) to GVariant tuple: ", reply_string);
        g_free(reply_string);
        dbus_message_unref(reply);
        return FALSE;
    }

    g_variant_get(g_variant_reply, "(i^ao)", relevance_return, paths_return);
    G_VARIANT_FREE(g_variant_reply);

    return TRUE;
}

/**
 * dbus_discover_call:
 * @target what to discover
 * @options dictionary of option {key,values}
 * @relevance_return Pointer to returned relevance value
 * @paths_return Pointer to an array of object path strings,
 * must be freed with g_strfreev().
 * @g_error initialized to error info when FALSE is returned.
 *
 * Marshal a realm Discover method call, call it synchronously,
 * unmarsh it's reply and return the OUT parameters.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized
 */
gboolean
dbus_discover_call(DBusConnection *bus, const char *target, GVariant *options,
                   gint32 *relevance_return, gchar ***paths_return, GError **g_error)
{
    DBusError dbus_error;
    DBusMessage *msg = NULL;
    DBusMessage* reply = NULL;

    g_return_val_if_fail (bus != NULL, FALSE);
    g_return_val_if_fail (target != NULL, FALSE);
    g_return_val_if_fail (options != NULL, FALSE);
    g_return_val_if_fail (relevance_return != NULL, FALSE);
    g_return_val_if_fail (paths_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    dbus_error_init(&dbus_error);
    if (!dbus_discover_marshal(target, options, &msg, g_error)) {
        RETURN_DBUS_ERROR(g_error, dbus_error);
    }

    if ((reply = dbus_connection_send_with_reply_and_block(bus, msg, -1, &dbus_error)) == NULL) {
        dbus_message_unref(msg);
        RETURN_DBUS_ERROR(g_error, dbus_error);
    }
    dbus_message_unref(msg);

    if (!dbus_discover_unmarshal(reply, relevance_return, paths_return, g_error)) {
        dbus_message_unref(reply);
        return FALSE;
    }
    dbus_message_unref(reply);
    return TRUE;
}

/*----------------------------------------------------------------------------*/

/**
 * dbus_change_login_policy_marshal:
 * @dbus_path The DBus object path of the object supporting the Realm
 * interface on which the call will be made.
 * @login_policy The new login policy, or an empty string.
 * @permitted_add: An array of logins to permit.
 * @permitted_remove: An array of logins to not permit.
 * @options dictionary of option {key,values}
 * @msg_return if successful DBus message returned here,
 * if error then this will be NULL.
 * @g_error initialized to error info when FALSE is returned.
 *
 * Marshal a realm ChangeLoginPolicy method call.
 *
 * Returns: return TRUE if successful, @msg_return will point to DBusMessage,
 * FALSE if error with @g_error initialized. @msg_return will be NULL.
 */
static gboolean
dbus_change_login_policy_marshal(const gchar *dbus_path, const char *login_policy,
                                 GVariant *permitted_add, GVariant *permitted_remove,
                                 GVariant *options, DBusMessage **msg_return, GError **g_error)
{
    const char *method = "ChangeLoginPolicy";
    DBusMessage *msg = NULL;
    DBusMessageIter iter;

    g_return_val_if_fail (dbus_path != NULL, FALSE);
    g_return_val_if_fail (login_policy != NULL, FALSE);
    g_return_val_if_fail (permitted_add != NULL, FALSE);
    g_return_val_if_fail (permitted_remove != NULL, FALSE);
    g_return_val_if_fail (options != NULL, FALSE);
    g_return_val_if_fail (msg_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *msg_return = NULL;

    if ((msg = dbus_message_new_method_call(REALM_DBUS_BUS_NAME, dbus_path,
                                            REALM_DBUS_REALM_INTERFACE, method)) == NULL) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "failed to create dbus method call %s.%s() message, object_path=%s",
                    REALM_DBUS_PROVIDER_INTERFACE, method, REALM_DBUS_SERVICE_PATH);
        return FALSE;
    }

    dbus_message_iter_init_append (msg, &iter); /* void return */

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &login_policy)) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                           "failed to add login_policy parameter (%s)", login_policy);
        dbus_message_unref(msg);
        return FALSE;
    }

    if (!append_g_variant_to_dbus_message(msg, permitted_add, g_error)) {
        g_prefix_error(g_error, "unable to append GVariant permitted_add dictionary into %s.%s() message",
                       REALM_DBUS_PROVIDER_INTERFACE, method);
        dbus_message_unref(msg);
        return FALSE;
    }

    if (!append_g_variant_to_dbus_message(msg, permitted_remove, g_error)) {
        g_prefix_error(g_error, "unable to append GVariant permitted_remove dictionary into %s.%s() message",
                       REALM_DBUS_PROVIDER_INTERFACE, method);
        dbus_message_unref(msg);
        return FALSE;
    }

    if (!append_g_variant_to_dbus_message(msg, options, g_error)) {
        g_prefix_error(g_error, "unable to append GVariant options dictionary into %s.%s() message",
                       REALM_DBUS_PROVIDER_INTERFACE, method);
        dbus_message_unref(msg);
        return FALSE;
    }

    *msg_return = msg;
    return TRUE;
}

/**
 * dbus_change_login_policy_unmarshal:
 * @reply DBus method reply from ChangeLoginPolicy call
 * @g_error initialized to error info when FALSE is returned.
 *
 * Parses the DBus reply message from the ChangeLoginPolicy call.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized.
 */

static gboolean
dbus_change_login_policy_unmarshal(DBusMessage *reply, GError **g_error)
{
    g_return_val_if_fail (reply != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    return TRUE;
}

/**
 * dbus_change_login_policy_call:
 * @dbus_path The DBus object path of the object supporting the Realm
 * interface on which the call will be made.
 * @login_policy The new login policy, or an empty string.
 * @permitted_add: An array of logins to permit.
 * @permitted_remove: An array of logins to not permit.
 * @options dictionary of option {key,values}
 * @g_error initialized to error info when FALSE is returned.
 *
 * Marshal a realm ChangeLoginPolicy method call, call it synchronously,
 * unmarsh it's reply and return the OUT parameters.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized
 */
gboolean
dbus_change_login_policy_call(DBusConnection *bus, const gchar *dbus_path, const char *login_policy,
                              GVariant *permitted_add, GVariant *permitted_remove,
                              GVariant *options, GError **g_error)
{
    DBusError dbus_error;
    DBusMessage *msg = NULL;
    DBusMessage* reply = NULL;

    g_return_val_if_fail (bus != NULL, FALSE);
    g_return_val_if_fail (dbus_path != NULL, FALSE);
    g_return_val_if_fail (login_policy != NULL, FALSE);
    g_return_val_if_fail (permitted_add != NULL, FALSE);
    g_return_val_if_fail (permitted_remove != NULL, FALSE);
    g_return_val_if_fail (options != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    dbus_error_init(&dbus_error);
    if (!dbus_change_login_policy_marshal(dbus_path, login_policy,
                                          permitted_add, permitted_remove,
                                          options, &msg, g_error)) {
        RETURN_DBUS_ERROR(g_error, dbus_error);
    }

    if ((reply = dbus_connection_send_with_reply_and_block(bus, msg, -1, &dbus_error)) == NULL) {
        dbus_message_unref(msg);
        RETURN_DBUS_ERROR(g_error, dbus_error);
    }
    dbus_message_unref(msg);

    if (!dbus_change_login_policy_unmarshal(reply, g_error)) {
        dbus_message_unref(reply);
        return FALSE;
    }
    dbus_message_unref(reply);
    return TRUE;
}

/*----------------------------------------------------------------------------*/

/**
 * dbus_join_leave_marshal:
 * @dbus_path The DBus object path of the object supporting the kerberos
 * membership interface on which the Join/Leave call will be made.
 * @credentials A GVariant encoding the credentials according the the credential
 * type. See the Realmd DBus interface for specifics.
 * @options dictionary of option {key,values}
 * @msg_return if successful DBus message returned here,
 * if error then this will be NULL.
 * @g_error initialized to error info when FALSE is returned.
 *
 * Since the Join() & Leave() methods share identical signatures (differing
 * only in their method name) we use a common routine.
 *
 * Returns: return TRUE if successful, @msg_return will point to DBusMessage,
 * FALSE if error with @g_error initialized. @msg_return will be NULL.
 */
static gboolean
dbus_join_leave_marshal(const char *method, const gchar* dbus_path,
                  GVariant *credentials, GVariant *options,
                  DBusMessage **msg_return, GError **g_error)
{
    DBusMessage *msg = NULL;
    DBusMessageIter iter;

    g_return_val_if_fail (method != NULL, FALSE);
    g_return_val_if_fail (dbus_path != NULL, FALSE);
    g_return_val_if_fail (credentials != NULL, FALSE);
    g_return_val_if_fail (options != NULL, FALSE);
    g_return_val_if_fail (msg_return != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    *msg_return = NULL;

    if ((msg = dbus_message_new_method_call(REALM_DBUS_BUS_NAME, dbus_path,
                                            REALM_DBUS_KERBEROS_MEMBERSHIP_INTERFACE, method)) == NULL) {
        g_set_error(g_error, RDCP_ERROR, RDCP_ERROR_DBUS,
                    "failed to create dbus method call %s.%s() message, object_path=%s",
                    REALM_DBUS_PROVIDER_INTERFACE, method, REALM_DBUS_SERVICE_PATH);
        return FALSE;
    }

    dbus_message_iter_init_append (msg, &iter); /* void return */

    if (!append_g_variant_to_dbus_message(msg, credentials, g_error)) {
        g_prefix_error(g_error, "unable to append GVariant credentials into %s.%s() message",
                       REALM_DBUS_PROVIDER_INTERFACE, method);
        dbus_message_unref(msg);
        return FALSE;
    }

    if (!append_g_variant_to_dbus_message(msg, options, g_error)) {
        g_prefix_error(g_error, "unable to append GVariant options dictionary into %s.%s() message",
                       REALM_DBUS_PROVIDER_INTERFACE, method);
        dbus_message_unref(msg);
        return FALSE;
    }

    *msg_return = msg;
    return TRUE;
}

/**
 * dbus_join_leave_unmarshal:
 * @reply DBus method reply from Join/Leave call
 * @g_error initialized to error info when FALSE is returned.
 *
 * Since the Join() & Leave() methods share identical signatures (differing
 * only in their method name) we use a common routine.
 *
 * Parses the DBus reply message from the Join/Leave call.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized.
 */

static gboolean
dbus_join_leave_unmarshal(DBusMessage *reply, GError **g_error)
{

    g_return_val_if_fail (reply != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    return TRUE;
}

/**
 * dbus_join_leave_call:
 * @dbus_path The DBus object path of the object supporting the kerberos
 * membership interface on which the Join/Leave call will be made.
 * @credentials A GVariant encoding the credentials according the the credential
 * type. See the Realmd DBus interface for specifics.
 * @options dictionary of option {key,values}
 * @g_error initialized to error info when FALSE is returned.
 *
 * Since the Join() & Leave() methods share identical signatures (differing
 * only in their method name) we use a common routine.
 *
 * Marshal a realm Join/Leave method call, call it synchronously,
 * unmarsh it's reply and return the OUT parameters.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized
 */
gboolean
dbus_join_leave_call(const char *method, DBusConnection *bus, const gchar *dbus_path,
                     GVariant *credentials, GVariant *options, GError **g_error)
{
    DBusError dbus_error;
    DBusMessage *msg = NULL;
    DBusMessage* reply = NULL;

    g_return_val_if_fail (method != NULL, FALSE);
    g_return_val_if_fail (bus != NULL, FALSE);
    g_return_val_if_fail (dbus_path != NULL, FALSE);
    g_return_val_if_fail (credentials != NULL, FALSE);
    g_return_val_if_fail (options != NULL, FALSE);
    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    dbus_error_init(&dbus_error);
    if (!dbus_join_leave_marshal(method, dbus_path, credentials, options, &msg, g_error)) {
        RETURN_DBUS_ERROR(g_error, dbus_error);
    }

    if ((reply = dbus_connection_send_with_reply_and_block(bus, msg, -1, &dbus_error)) == NULL) {
        dbus_message_unref(msg);
        RETURN_DBUS_ERROR(g_error, dbus_error);
    }
    dbus_message_unref(msg);

    if (!dbus_join_leave_unmarshal(reply, g_error)) {
        dbus_message_unref(reply);
        return FALSE;
    }
    dbus_message_unref(reply);
    return TRUE;
}


/*----------------------------------------------------------------------------*/

/**
 * dbus_join_call:
 * @dbus_path The DBus object path of the object supporting the kerberos
 * membership interface on which the Join call will be made.
 * @credentials A GVariant encoding the credentials according the the credential
 * type. See the Realmd DBus interface for specifics.
 * @options dictionary of option {key,values}
 * @g_error initialized to error info when FALSE is returned.
 *
 * Marshal a realm Join method call, call it synchronously,
 * unmarsh it's reply and return the OUT parameters.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized
 */
gboolean
dbus_join_call(DBusConnection *bus, const gchar *dbus_path,
               GVariant *credentials, GVariant *options, GError **g_error)
{

    return dbus_join_leave_call("Join", bus, dbus_path, credentials,
                                options, g_error);

}


/**
 * dbus_leave_call:
 * @dbus_path The DBus object path of the object supporting the kerberos
 * membership interface on which the Leave call will be made.
 * @credentials A GVariant encoding the credentials according the the credential
 * type. See the Realmd DBus interface for specifics.
 * @options dictionary of option {key,values}
 * @g_error initialized to error info when FALSE is returned.
 *
 * Marshal a realm Leave method call, call it synchronously,
 * unmarsh it's reply and return the OUT parameters.
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized
 */
gboolean
dbus_leave_call(DBusConnection *bus, const gchar *dbus_path,
               GVariant *credentials, GVariant *options, GError **g_error)
{

    return dbus_join_leave_call("Leave", bus, dbus_path, credentials,
                                options, g_error);

}


/*----------------------------------------------------------------------------*/
/**
 * get_short_dbus_interface_name
 * @interface fully qualified DBus interface name
 *
 * Given a DBus interface name return a friendly short name
 * appropriate for users to see. Currently the known short names are:
 *
 * * "Kerberos"
 * * "KerberosMembership"
 * * "Realm"
 * * "Provider"
 * * "Service"
 *
 * If the interface is not recognized the portion of the interface
 * following the last period (".") will be returned. If there is no
 * period in the interface name then the entire interface string is returned.
 * If the interface is NULL then "(null)" is returned.
 *
 * Returns: pointer to string, must free with g_free()
 */

char *
get_short_dbus_interface_name(const char *interface)
{
    char *token = NULL;

    if (interface == NULL) {
        return g_strdup("(null)");
    }

    if (strcmp(interface, REALM_DBUS_KERBEROS_INTERFACE) == 0) {
        return g_strdup("Kerberos");
    }
    if (strcmp(interface, REALM_DBUS_KERBEROS_MEMBERSHIP_INTERFACE) == 0) {
        return g_strdup("KerberosMembership");
    }
    if (strcmp(interface, REALM_DBUS_REALM_INTERFACE) == 0) {
        return g_strdup("Realm");
    }
    if (strcmp(interface, REALM_DBUS_PROVIDER_INTERFACE) == 0) {
        return g_strdup("Provider");
    }
    if (strcmp(interface, REALM_DBUS_SERVICE_INTERFACE) == 0) {
        return g_strdup("Service");
    }
    /* Return string which begins after last period */
    if ((token = rindex(interface, '.'))) {
        token++;                  /* skip "." char */
        return g_strdup(token);
    } else {
        return g_strdup(interface);
    }

}

/*----------------------------------------------------------------------------*/

/**
 * rdcp_dbus_initialize:
 * @g_error initialized to error info when FALSE is returned.
 *
 * Initializes the dbus module.
 *
 * - opens a connection the System Bus, exported as #system_bus
 *
 * Returns: return TRUE if successful, FALSE if error with @g_error initialized.
 */

gboolean
rdcp_dbus_initialize(GError **g_error)
{
    DBusError dbus_error = DBUS_ERROR_INIT;

    dbus_error_init(&dbus_error);

    g_return_val_if_fail (g_error == NULL || *g_error == NULL, FALSE);

    if (!system_bus) {
        if ((system_bus = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error)) == NULL) {
            *g_error = dbus_error_to_gerror(&dbus_error);
            g_prefix_error(g_error, "could not connect to System DBus");
            return FALSE;
        }
    }

    return TRUE;
}
