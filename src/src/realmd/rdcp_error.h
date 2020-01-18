#ifndef __RDCP_ERROR_H__
#define __RDCP_ERROR_H__

#include <glib.h>

#define       RDCP_ERROR               (rdcp_error_quark ())

GQuark        rdcp_error_quark         (void) G_GNUC_CONST;

typedef enum {
	RDCP_ERROR_INTERNAL = 1,
	RDCP_ERROR_INVALID_ARG,
	RDCP_ERROR_INVALID_INSTANCE_ID,
	RDCP_ERROR_DBUS,
} rdcp_error_codes;

const char *
rdcp_error_code_to_string(rdcp_error_codes ec);

#define LMI_REALMD_RESULT_SUCCESS                                      0
#define LMI_REALMD_RESULT_FAILED                                       1
#define LMI_REALMD_RESULT_NO_SUCH_DOMAIN                               2
#define LMI_REALMD_RESULT_DOMAIN_DOES_NOT_SUPPORT_PROVIDED_CREDENTIALS 3
#define LMI_REALMD_RESULT_DOMAIN_DOES_NOT_SUPPORT_JOINING              4

CMPIStatus
handle_g_error(GError **g_error, const CMPIBroker* cb, CMPIStatus* status, CMPIrc rc,
               const gchar *format, ...)
    __attribute__ ((format (printf, 5, 6)));

CMPIStatus
SetCMPIStatus(const CMPIBroker* mb, CMPIStatus* st, CMPIrc rc,
              const gchar *format, ...)
    __attribute__ ((format (printf, 4, 5)));

#endif /* __RDCP_ERROR_H__ */
