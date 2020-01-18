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
 * Authors: Radek Novacek <rnovacek@redhat.com>
 */

#ifndef OPENLMI_H
#define OPENLMI_H

#include <cmpidt.h>
#include <stdbool.h>

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
 * @warning Call lmi_init function before calling this function!
 *
 * @return CIM_ComputerSystem object path
 */
CMPIObjectPath *lmi_get_computer_system(void);

/**
 * This function returns system name for the computer system
 *
 * @note Use this in the SystemName property of all provider created instances.
 *
 * @warning Call lmi_init function before calling this function!
 *
 * @return The scoping System's Name.
 */
const char *lmi_get_system_name(void);

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
 *             NULL if there is no provider-specific configuration
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
#define LMI_ReturnInstance(cr, w) KOkay(__KReturnInstance((cr), &(w).__base))

enum {
    _LMI_DEBUG_NONE=0, _LMI_DEBUG_ERROR, _LMI_DEBUG_WARN,
    _LMI_DEBUG_INFO, _LMI_DEBUG_DEBUG
};

void _lmi_debug(int level, const char *file, int line, const char *format, ...);

#define lmi_debug(...) _lmi_debug(_LMI_DEBUG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define lmi_info(...)  _lmi_debug(_LMI_DEBUG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define lmi_warn(...)  _lmi_debug(_LMI_DEBUG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define lmi_error(...) _lmi_debug(_LMI_DEBUG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif
