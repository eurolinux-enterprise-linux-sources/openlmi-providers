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

#include "openlmi.h"

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <cmpimacs.h>
#include <glib.h>
#include <dlfcn.h>

static char *_system_name = NULL;
static CMPIObjectPath *_computer_system = NULL;
static const CMPIBroker *_cb = NULL;
static char *_provider = NULL;

// Storage for all keys from configuration files and default configuration option
// Access needs to be mutexed
static GKeyFile *_masterKeyFile = NULL;

// Mutex for running lmi_init
static pthread_mutex_t _init_mutex = PTHREAD_MUTEX_INITIALIZER;

// Cache for SystemCreationClassName
static char *_system_creation_class_name = NULL;

// Cache for log level
static int _log_level = _LMI_DEBUG_NONE;

// Cache for log stderr
static bool _log_stderr = false;

static ConfigEntry _toplevel_config_defaults[] = {
    { "CIM", "Namespace", "root/cimv2" },
    { "CIM", "SystemClassName", "PG_ComputerSystem" },
    { "LOG", "Level", "ERROR" },
    { "LOG", "Stderr" , "false" }
};

#define TOPLEVEL_CONFIG_FILE "/etc/openlmi/openlmi.conf"
#define PROVIDER_CONFIG_FILE "/etc/openlmi/%s/%s.conf"

/**
 * Enumerate configured ComputerSystem class in order to obtain
 * ComputerSystem ObjectPath and Hostname
 *
 * @param cb CMPIBroker
 * @param ctx CMPIContext
 * @retval true Success
 * @retval false Failure
 */
static bool get_computer_system(const CMPIBroker *cb, const CMPIContext *ctx)
{
    free(_system_name);
    _system_name = NULL;
    if (_computer_system != NULL) {
        CMRelease(_computer_system);
        _computer_system = NULL;
    }

    const char *class_name = lmi_get_system_creation_class_name();
    const char *namespace = lmi_read_config("CIM", "Namespace");
    CMPIStatus rc = { 0, NULL };
    CMPIObjectPath *op = CMNewObjectPath(cb, namespace, class_name, &rc);
    if (rc.rc != CMPI_RC_OK) {
        lmi_error("Unable to create object path: %s", rc.msg);
        return false;
    }
    CMPIEnumeration *en = CBEnumInstanceNames(cb, ctx, op, &rc);
    if (rc.rc != CMPI_RC_OK) {
        lmi_error("Unable to enumerate instance names of class %s", class_name);
        return false;
    }

    if (!CMHasNext(en, &rc)) {
        lmi_error("No instance of class %s exists", class_name);
        CMRelease(en);
        return false;
    }
    CMPIData data = CMGetNext(en, &rc);
    if (rc.rc != CMPI_RC_OK) {
        lmi_error("Unable to get instance name of class %s", class_name);
        CMRelease(en);
        return false;
    }
    if (data.type != CMPI_ref) {
        lmi_error("EnumInstanceNames didn't return CMPI_ref type, but %d", data.type);
        CMRelease(en);
        return false;
    }
    _computer_system = CMClone(data.value.ref, &rc);
    if (rc.rc != CMPI_RC_OK) {
        lmi_error("Unable to clone ComputerSystem object path");
        CMRelease(en);
        return false;
    }
    _system_name = strdup(CMGetCharPtr(CMGetKey(_computer_system, "Name", &rc).value.string));
    CMRelease(en);
    return true;
}

CMPIObjectPath *lmi_get_computer_system(void)
{
    return _computer_system;
}

const char *lmi_get_system_name(void)
{
    return _system_name;
}

const char *lmi_get_system_creation_class_name(void)
{
    if (_system_creation_class_name == NULL) {
        if (_masterKeyFile == NULL) {
            lmi_error("Configuration was not read, using default option");
            return "PG_ComputerSystem";
        }
        _system_creation_class_name = lmi_read_config("CIM", "SystemClassName");
        if (_system_creation_class_name == NULL) {
            // This shouldn't happen, SystemClassName should be at least
            // in default config keys
            return "PG_ComputerSystem";
        }
    }
    return _system_creation_class_name;
}

char *lmi_read_config(const char *group, const char *key)
{
    if (_masterKeyFile == NULL) {
        lmi_warn("Attempted to read config file before calling lmi_init");
        return NULL;
    }
    return g_key_file_get_string(_masterKeyFile, group, key, NULL);
}

bool lmi_read_config_boolean(const char *group, const char *key)
{
    char *value = lmi_read_config(group, key);
    if (value == NULL) {
        return false;
    }
    char *true_values[] = { "1", "yes", "true", "on" };
    size_t len = sizeof(true_values) / sizeof(true_values[0]);
    for (size_t i = 0; i < len; ++i) {
        if (strcasecmp(true_values[i], value) == 0) {
            free(value);
            return true;
        }
    }
    free(value);
    return false;
}

GKeyFile *parse_config(const char *provider_name, const ConfigEntry *provider_config_defaults)
{
    GError *error = NULL;
    char *providerconf;

    // Get all configuration options to masterKeyFile
    GKeyFile *masterKeyFile = g_key_file_new();

    // Read top-level openlmi configuration
    if (!g_key_file_load_from_file(masterKeyFile, TOPLEVEL_CONFIG_FILE, G_KEY_FILE_NONE, &error)) {
        lmi_debug("Can't read openlmi top-level config file %s: %s", TOPLEVEL_CONFIG_FILE, error->message);
        g_clear_error(&error);
    }

    // Read provider-specific configuration
    if (asprintf(&providerconf, PROVIDER_CONFIG_FILE, provider_name, provider_name) <= 0) {
        lmi_error("Memory allocation failed");
        g_key_file_free(masterKeyFile);
        return NULL;
    }
    GKeyFile *providerKeyFile;
    if ((providerKeyFile = g_key_file_new()) == NULL) {
        lmi_error("Memory allocation failed");
        g_key_file_free(masterKeyFile);
        return NULL;
    }
    if (!g_key_file_load_from_file(providerKeyFile, providerconf, G_KEY_FILE_NONE, &error)) {
        lmi_debug("Can't read provider specific config file %s: %s", providerconf, error->message);
        g_clear_error(&error);
    }

    // Merge provider config to _masterKeyFile
    gsize groups_len, keys_len, i, j;
    gchar *v;
    gchar **groups = g_key_file_get_groups(providerKeyFile, &groups_len), **keys;
    if (groups != NULL) {
        for (i = 0; i < groups_len; ++i) {
            keys = g_key_file_get_keys(providerKeyFile, groups[i], &keys_len, NULL);
            for (j = 0; j < keys_len; ++j) {
                v = g_key_file_get_value(providerKeyFile, groups[i], keys[j], NULL);
                g_key_file_set_value(masterKeyFile, groups[i], keys[j], v);
                free(v);
            }
            g_strfreev(keys);
        }
        g_strfreev(groups);
    }
    g_key_file_free(providerKeyFile);

    // Fill in default values where nothing gets read from config file.
    // Provider-specific configs first
    gsize len = 0;
    if (provider_config_defaults != NULL) {
        len = sizeof(provider_config_defaults) / sizeof(ConfigEntry);
    }
    for (i = 0; i < len; i++) {
        if (!g_key_file_has_key(masterKeyFile,
                                provider_config_defaults[i].group,
                                provider_config_defaults[i].key, NULL)) {

            g_key_file_set_value(masterKeyFile,
                                 provider_config_defaults[i].group,
                                 provider_config_defaults[i].key,
                                 provider_config_defaults[i].value);
        }
    }

    // Top-level configs
    len = sizeof(_toplevel_config_defaults)/sizeof(ConfigEntry);
    for (i = 0; i < len; i++) {
        if (!g_key_file_has_key(masterKeyFile,
                                _toplevel_config_defaults[i].group,
                                _toplevel_config_defaults[i].key, NULL)) {

            g_key_file_set_value(masterKeyFile,
                                 _toplevel_config_defaults[i].group,
                                 _toplevel_config_defaults[i].key,
                                 _toplevel_config_defaults[i].value);
        }
    }
    return masterKeyFile;
}

void lmi_init(const char *provider, const CMPIBroker *cb,
              const CMPIContext *ctx,
              const ConfigEntry *provider_config_defaults)
{
    static void *glib_loaded = 0;

    pthread_mutex_lock(&_init_mutex);
    // Broker can change between threads
    _cb = cb;

    // Provider should remain the same
    if (_provider != NULL) {
        if (strcmp(_provider, provider) != 0) {
            lmi_error("lmi_init called twice with different provider (%s -> %s), "
                  "this shouldn't happen", _provider, provider);
            free(_provider);
        } else {
            pthread_mutex_unlock(&_init_mutex);
            return;
        }
    }
    _provider = strdup(provider);
    if (_masterKeyFile != NULL) {
        g_key_file_free(_masterKeyFile);
    }
    // Read config only on first call of this function
    _masterKeyFile = parse_config(provider, provider_config_defaults);
    // We might read different log config, reread it in next _log_debug call
    _log_level = _LMI_DEBUG_NONE;

    // Read ComputerSystem instance
    if (_computer_system == NULL || _system_name == NULL) {
        get_computer_system(cb, ctx);
    }

    /*
     * Ugly hack to prevent Pegasus from crashing on cleanup.
     * Glib2 adds some thread shutdown callback (see man pthread_key_create),
     * but when cimprovagt destroys threads, this provider is already dlclosed().
     * So keep the libglib in memory so the callback does not crash.
     * https://bugzilla.redhat.com/show_bug.cgi?id=1010238
     */
    if (!glib_loaded) {
        glib_loaded = dlopen("libglib-2.0.so.0", RTLD_LAZY);
        lmi_info("Loaded glib: %p\n", glib_loaded);
    }

    pthread_mutex_unlock(&_init_mutex);
}

int lmi_log_level(void)
{
    return _log_level;
}

void lmi_set_log_level(int level)
{
    _log_level = level;
}

void _lmi_debug(int level, const char *file, int line, const char *format, ...)
{
    const char *lvl[] = { "NONE", "ERROR", "WARNING", "INFO", "DEBUG" };
    if (_log_level == _LMI_DEBUG_NONE) {
        // Read log level from config or default
        _log_level = _LMI_DEBUG_ERROR; // Default
        char *level = lmi_read_config("Log", "Level");
        if (level != NULL) {
            size_t len = sizeof(lvl) / sizeof(lvl[0]);
            for (size_t i = 0; i < len; ++i) {
                if (strcasecmp(level, lvl[i]) == 0) {
                    _log_level = i;
                    break;
                }
            }
            free(level);
        }

        // Read if log to stderr
        _log_stderr = lmi_read_config_boolean("Log", "Stderr");
    }
    if (level > 4) {
        level = 4;
    }
    if (level < 1) {
        level = 1;
    }

    if (level > _log_level) {
        // Do not log this message
        return;
    }

    char *message, *text;
    va_list args;
    va_start(args, format);
    vasprintf(&message, format, args);
    va_end(args);
    asprintf(&text, "[%s] %s:%d\t%s", lvl[level], file, line, message);
    free(message);

    CMPIStatus rc;
    rc.rc = CMPI_RC_OK;
    if (_cb != NULL) {
        // try to use standard CMPI logging

        // CMPI has different severity levels (1=info, 4=fatal)
        int severity = CMPI_SEV_INFO;
        switch (level) {
            case _LMI_DEBUG_DEBUG:
                severity = CMPI_DEV_DEBUG;
                break;
            case _LMI_DEBUG_INFO:
                severity = CMPI_SEV_INFO;
                break;
            case _LMI_DEBUG_WARN:
                severity = CMPI_SEV_WARNING;
                break;
            case _LMI_DEBUG_ERROR:
                severity = CMPI_SEV_ERROR;
                break;
        }
        rc = CMLogMessage(_cb, severity, _provider, text, NULL);
    }

    if (_log_stderr || _cb == NULL || rc.rc != CMPI_RC_OK) {
        // Fallback to stderr
        fprintf(stderr, "%s\n", text);
    }
    free(text);
}
