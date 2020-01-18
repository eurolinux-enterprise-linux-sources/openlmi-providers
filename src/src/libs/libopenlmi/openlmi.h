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
 * Authors: Radek Novacek <rnovacek@redhat.com>
 *          Jan Synacek   <jsynacek@redhat.com>
 */

#ifndef OPENLMI_H
#define OPENLMI_H

#include <cmpidt.h>
#include <cmpimacs.h>
#include <errno.h>
#include <glib.h>
#include <konkret/konkret.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>

#ifndef BUFLEN
  #define BUFLEN   1024
#endif
#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

#define WHITESPACES " \f\n\r\t\v"

#define ARRAY_SIZE(name) (sizeof(name) / sizeof(name[0]))

/* Association sides. */
#define LMI_GROUP_COMPONENT    "GroupComponent"
#define LMI_PART_COMPONENT     "PartComponent"
#define LMI_SAME_ELEMENT       "SameElement"
#define LMI_SYSTEM_ELEMENT     "SystemElement"
#define LMI_COLLECTION         "Collection"
#define LMI_MEMBER             "Member"
#define LMI_SYSTEM             "System"
#define LMI_INSTALLED_SOFTWARE "InstalledSoftware"
#define LMI_AVAILABLE_SAP      "AvailableSAP"
#define LMI_MANAGED_ELEMENT    "ManagedElement"
#define LMI_ELEMENT            "Element"
#define LMI_CHECK              "Check"
#define LMI_FILE               "File"

/* CMPI_RC_ERR_<error> values end at 200. 0xFF should be safe. */
#define LMI_RC_ERR_CLASS_CHECK_FAILED 0xFF

/* Organization ID. Used for InstaceIDs. */
#define LMI_ORGID "LMI"

/* Helper time conversion macros. */
#define LMI_DAYS_TO_MS(days) ((days) * 86400000000)
#define LMI_MS_TO_DAYS(ms)   ((ms) / 86400000000)
#define LMI_SECS_TO_MS(s)    ((s) * 1000000)

#ifdef G_DEPRECATED
#define LMI_DEPRECATED G_DEPRECATED
#else
// GLib < 2.32 doesn't have G_DEPRECATED but it at least has (non portable)
// G_GNUC_DEPRECATED
#define LMI_DEPRECATED G_GNUC_DEPRECATED
#endif

#ifdef G_DEPRECATED_FOR
#define LMI_DEPRECATED_FOR G_DEPRECATED_FOR
#else
// GLib < 2.32 doesn't have G_DEPRECATED_FOR but it at least has (non portable)
// G_GNUC_DEPRECATED_FOR
#define LMI_DEPRECATED_FOR G_GNUC_DEPRECATED_FOR
#endif

typedef struct {
    const char *group;
    const char *key;
    const char *value;
} ConfigEntry;

/**
 * This function returns object path of an instance of CIM_ComputerSystem
 * subclass.
 *
 * The instance is obtained by enumerating the configured ComputerSystem
 * classname.
 *
 * @deprecated Please use lmi_get_computer_system_safe() instead.
 *
 * @warning Call lmi_init function before calling this function!
 *
 * @return CIM_ComputerSystem object path
 */
LMI_DEPRECATED_FOR(lmi_get_computer_system_safe)
CMPIObjectPath *lmi_get_computer_system(void);

/**
 * This function returns object path of an instance of CIM_ComputerSystem
 * subclass.
 *
 * The instance is obtained by enumerating the configured ComputerSystem
 * classname.
 *
 * @warning Call lmi_init function before calling this function!
 *
 * @return CIM_ComputerSystem object path
 */
CMPIObjectPath *lmi_get_computer_system_safe(const CMPIContext *ctx);

/**
 * Check object path of CIM_ComputerSystem.
 *
 * @return true, if the object path matches this computer system.
 */
bool lmi_check_computer_system(const CMPIContext *ctx,
                               const CMPIObjectPath *computer_system);

/**
 * This function returns system name for the computer system
 *
 * @note Use this in the SystemName property of all provider created instances.
 *
 * @deprecated Please use lmi_get_system_name_safe() instead.
 *
 * @warning Call lmi_init function before calling this function!
 *
 * @return The scoping System's Name.
 */
LMI_DEPRECATED_FOR(lmi_get_system_name_safe)
const char *lmi_get_system_name(void);

/**
 * This function returns system name for the computer system
 *
 * @note Use this in the SystemName property of all provider created instances.
 *
 * @warning Call lmi_init function before calling this function!
 *
 * @return The scoping System's Name.
 */
const char *lmi_get_system_name_safe(const CMPIContext *ctx);

/**
 * This function returns system creation class name
 *
 * @note  Use this in the SystemCreationClassName property of all provider
 * created instances.
 *
 * @return The scoping System's Creation class name.
 */
const char *lmi_get_system_creation_class_name(void);


/**
 * Initialize usage base openlmi tools like configuration file access,
 * logging etc.
 *
 * @note You must call this function prior to getting any configuration option
 * or usage of logging. lmi_get_system_creation_class_name requires that this
 * function will be called first (SystemCreationClassName is read from config).
 *
 * This function is reentrant and thread safe, but it should be called always
 * with same parameters
 *
 * @param provider Identification of the CIM provider (must be same as name of the
 *             configuration file)
 * @param cb CMPIBroker
 * @param ctx CMPIContext
 * @param provider_config_defaults Array of default config values for given provider
 *             terminated by empty struct or NULL if there is no
 *             provider-specific configuration
 */
void lmi_init(const char *provider, const CMPIBroker *cb,
              const CMPIContext *ctx,
              const ConfigEntry *provider_config_defaults);

/**
 * Reads string key out of configration files or default configration options.
 *
 * @param group Configration group
 * @param key Configration key
 * @return String value of the key or NULL if group/key is not found
 */
char *lmi_read_config(const char *group, const char *key);

/**
 * Reads a boolean value out of configuration files or default configuration
 * options.
 *
 * Values "1", "yes", "true", and "on" are converted to TRUE, others to FALSE
 *
 * @param group Configration group
 * @param key Configration key
 * @return Boolean value of the key, false if the key is not in the
 *         configuration files neither in default options.
 */
bool lmi_read_config_boolean(const char *group, const char *key);

/**
 * To use standard CIMOM logging facility, broker must be assigned. Without
 * calling this function, logging will go to stderr.
 *
 * @deprecated Use lmi_init instead
 *
 * @param log_id Identification of log messages
 * @param cb CMPIBroker
 */
void lmi_init_logging(const char *log_id, const CMPIBroker *cb);

/**
 * Get currently set logging level
 *
 * @return logging level
 */
int lmi_log_level(void);

/**
 * Set logging level
 *
 * @note This method shouldn't be used directly, user setting
 *       from the configuration file should be honored
 *
 * @param level new logging level
 */
void lmi_set_log_level(int level);

/**
 * Add an instance \p w to the result \p cr.
 *
 * @param cr CMPIResult where should be the instance added
 * @param w instance to add
 * @retval true if succeeds
 * @retval false if addition fails
 */
#define lmi_return_instance(cr, w) KOkay(__KReturnInstance((cr), &(w).__base))

enum {
    _LMI_DEBUG_NONE=0, _LMI_DEBUG_ERROR, _LMI_DEBUG_WARN,
    _LMI_DEBUG_INFO, _LMI_DEBUG_DEBUG
};

void _lmi_debug(int level, const char *file, int line, const char *format, ...);

#define lmi_debug(...) _lmi_debug(_LMI_DEBUG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define lmi_info(...)  _lmi_debug(_LMI_DEBUG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define lmi_warn(...)  _lmi_debug(_LMI_DEBUG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define lmi_error(...) _lmi_debug(_LMI_DEBUG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define lmi_dump_objectpath(o, lvl)                                     \
    lmi_##lvl("OP: %s", CMGetCharsPtr((o)->ft->toString(o, NULL), NULL))



#define lmi_return_with_chars(b, code, ...)             \
    do {                                                \
        char errmsg[BUFLEN];                            \
        snprintf(errmsg, BUFLEN - 1, __VA_ARGS__);      \
        CMReturnWithChars(b, code, errmsg);             \
    } while (0)

/* Status checking helpers. Beware of the returns! */
#define lmi_return_if_status_not_ok(st)         \
    do {                                        \
        if ((st).rc != CMPI_RC_OK) {            \
            return (st);                        \
        }                                       \
    } while (0)

#define lmi_return_if_class_check_not_ok(st)            \
    do {                                                \
        if ((st).rc == LMI_RC_ERR_CLASS_CHECK_FAILED) { \
            CMReturn(CMPI_RC_OK);                       \
        }                                               \
        lmi_return_if_status_not_ok(st);                \
    } while (0)

#define lmi_return_with_status(b, st, code, msg)        \
    do {                                                \
        KSetStatus2(b, st, code, msg);                  \
        return *(st);                                   \
    } while(0)


/**
 * Check required properties and their values. currently, only computer system
 * creation class name and computer system name are checked.
 *
 * @param cb CMPIBroker
 * @param ctx CMPIContext
 * @param o Objectpath to be checked
 * @param csccn_prop_name Computer system creation class name property name
 * @param csn_prop_name Computer system name property name
 */
CMPIStatus lmi_check_required_properties(const CMPIBroker *cb, const CMPIContext *ctx,
                                         const CMPIObjectPath *o,
                                         const char *csccn_prop_name,
                                         const char *csn_prop_name);

/**
 * Determine whether a CIM class is of <class_type> or any of <class_type>
 * subclasses.
 *
 * @param cb CMPIBroker
 * @param namespace CIMOM namespace
 * @param class Class to be checked
 * @param class_type Class to be checked against
 */
CMPIStatus lmi_class_path_is_a(const CMPIBroker *cb, const char *namespace,
                               const char *class, const char *class_type);

const char *lmi_get_string_property_from_objectpath(const CMPIObjectPath *o, const char *prop);
const char *lmi_get_string_property_from_instance(const CMPIInstance *i, const char *prop);

/**
 * Convert CMPI data to string.
 * @param result Will be allocated to hold resulting serialized data.
 */
CMPIStatus lmi_data_to_string(const CMPIData *data, gchar **result);

#endif /* OPENLMI_H */

/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4 */
/* End: */
