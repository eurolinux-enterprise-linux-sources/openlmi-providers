#include <stdio.h>
#include "rdcp_util.h"
#include "rdcp_error.h"

GQuark
rdcp_error_quark (void)
{
    static volatile gsize once = 0;
    static GQuark quark = 0;

    if (g_once_init_enter(&once)) {
        quark = g_quark_from_static_string("rdcp-error");
        g_once_init_leave(&once, 1);
    }

    return quark;
}

const char *
rdcp_error_code_to_string(rdcp_error_codes ec)
{
    switch(ec) {
    case RDCP_ERROR_INTERNAL:            return "RDCP_ERROR_INTERNAL";
    case RDCP_ERROR_INVALID_ARG:         return "RDCP_ERROR_INVALID_ARG";
    case RDCP_ERROR_INVALID_INSTANCE_ID: return "RDCP_ERROR_INVALID_INSTANCE_ID";
    case RDCP_ERROR_DBUS:                return "RDCP_ERROR_DBUS";
    default:                             return "unknown error code";
    }
}

/*----------------------------------------------------------------------------*/

/**
 * handle_g_error:
 * @g_error pointer to non-NULL GError pointer describing problem
 * @mb CMPI message broker
 * @st CMPI status result
 * @rc CMPI return code
 * @format printf-style format string, may be #NULL if no additional
 * message is desired.
 *
 * Sets @st status to the @rc return code and an optional printf
 * styles formatted message which is prepended to the error message
 * contained in the g_error. It frees the g_error.
 *
 * Returns: the @st status passed in.
 */
CMPIStatus
handle_g_error(GError **g_error, const CMPIBroker* mb, CMPIStatus* st, CMPIrc rc,
               const gchar *format, ...)
{
    CMPIStatus failsafe_status;
    GString *message;
    va_list va;

    CMSetStatus(&failsafe_status, CMPI_RC_ERR_FAILED);
    g_return_val_if_fail (g_error != NULL && *g_error != NULL, failsafe_status);
    g_return_val_if_fail (st != NULL, failsafe_status);

    message = g_string_sized_new(DEFAULT_STATUS_MSG_SIZE);
    g_string_append_printf(message, "%s: ", ORGID);

    if (format) {
        va_start(va, format);
        g_string_append_vprintf(message, format, va);
        va_end(va);
        g_string_append(message, ": ");
    }

    g_string_append_printf(message, "(%s(%d)) ",
                           rdcp_error_code_to_string((*g_error)->code),
                           (*g_error)->code);
    g_string_append(message, (*g_error)->message);
    g_error_free(*g_error);
    *g_error = NULL;

    CMSetStatusWithChars(mb, st, rc, message->str);
    g_string_free(message, TRUE);

    return *st;
}


/**
 * SetCMPIStatus:
 * @mb CMPI message broker
 * @st CMPI status result
 * @rc CMPI return code
 * @format printf-style format string, may be #NULL if no additional
 * message is desired.
 *
 * Sets @st status to the @rc return code and an optional printf
 * style formatted message.
 *
 * Returns: the @st status passed in.
 */
CMPIStatus
SetCMPIStatus(const CMPIBroker* mb, CMPIStatus* st, CMPIrc rc,
              const gchar *format, ...)
{
    CMPIStatus failsafe_status;
    GString *message = NULL;
    va_list va;

    CMSetStatus(&failsafe_status, CMPI_RC_ERR_FAILED);
    g_return_val_if_fail (st != NULL, failsafe_status);

    if (format) {
        message = g_string_sized_new(DEFAULT_STATUS_MSG_SIZE);
        g_string_append_printf(message, "%s: ", ORGID);

        va_start(va, format);
        g_string_append_vprintf(message, format, va);
        va_end(va);

        CMSetStatusWithChars(mb, st, rc, message->str);
        g_string_free(message, TRUE);
    } else {
        CMSetStatus(st, rc);
    }

    return *st;
}
