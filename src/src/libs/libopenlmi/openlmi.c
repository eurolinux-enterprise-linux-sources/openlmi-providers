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

#include "openlmi.h"

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <dlfcn.h>
#include <pthread.h>

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
    { "Log", "Level", "ERROR" },
    { "Log", "Stderr" , "false" },
    { NULL, NULL, NULL }
};

#define TOPLEVEL_CONFIG_FILE "/etc/openlmi/openlmi.conf"
#define PROVIDER_CONFIG_FILE "/etc/openlmi/%s/%s.conf"

/**
 * Enumerate configured ComputerSystem class in order to obtain
 * ComputerSystem ObjectPath and Hostname
 *
 * @note This call needs to be guarded with `_init_mutex`.
 *
 * @param ctx CMPIContext
 * @param csop If not NULL, will be set with cached computer system.
 * @param system_name If not NULL, will be set with cached system name.
 * @retval true Success
 * @retval false Failure
 */
static bool get_computer_system(const CMPIContext *ctx,
                                CMPIObjectPath **csop,
                                const char **system_name)
{
    CMPIStatus rc = { 0, NULL };
    const char *class_name;
    char *namespace;
    CMPIObjectPath *op;
    CMPIEnumeration *en = NULL;

    if (_system_name == NULL || _computer_system == NULL) {
        lmi_debug("Caching computer system object path.");
        if (_computer_system != NULL) {
            CMRelease(_computer_system);
            _computer_system = NULL;
        }
        free(_system_name);
        _system_name = NULL;

        class_name = lmi_get_system_creation_class_name();
        namespace = lmi_read_config("CIM", "Namespace");
        op = CMNewObjectPath(_cb, namespace, class_name, &rc);
        g_free(namespace);
        if (rc.rc != CMPI_RC_OK) {
            lmi_error("Unable to create object path: %s", rc.msg);
            goto err;
        }
        en = CBEnumInstanceNames(_cb, ctx, op, &rc);
        if (rc.rc != CMPI_RC_OK) {
            lmi_error("Unable to enumerate instance names of class %s",
                    class_name);
            goto err;
        }
        if (!CMHasNext(en, &rc)) {
            lmi_error("No instance of class %s exists", class_name);
            goto err;
        }
        CMPIData data = CMGetNext(en, &rc);
        if (rc.rc != CMPI_RC_OK) {
            lmi_error("Unable to get instance name of class %s", class_name);
            goto err;
        }
        if (data.type != CMPI_ref) {
            lmi_error("EnumInstanceNames didn't return CMPI_ref type, but %d",
                    data.type);
            goto err;
        }
        _computer_system = CMClone(data.value.ref, &rc);
        if (rc.rc != CMPI_RC_OK) {
            lmi_error("Unable to clone ComputerSystem object path");
            goto err;
        }
        _system_name = strdup(CMGetCharPtr(
                    CMGetKey(_computer_system, "Name", NULL).value.string));
        if (_system_name == NULL) {
            lmi_error("Memory allocation failed");
            goto err;
        }
        CMRelease(en);
        lmi_debug("Successfuly cached computer system.");
    }

    if (system_name != NULL)
        *system_name = _system_name;
    if (csop != NULL)
        *csop = _computer_system;

    return true;

err:
    if (en)
        CMRelease(en);
    if (_computer_system != NULL) {
        CMRelease(_computer_system);
        _computer_system = NULL;
    }
    return false;
}

static bool lmi_is_this_system(const CMPIContext *ctx, const char *system_name)
{
    const char *csn;
    bool res = false;

    pthread_mutex_lock(&_init_mutex);
    if (get_computer_system(ctx, NULL, &csn))
        // TODO accept also aliases and FQDNs
        res = strcmp(system_name, csn) == 0;
    pthread_mutex_unlock(&_init_mutex);

    return res;
}

CMPIObjectPath *lmi_get_computer_system(void)
{
    return _computer_system;
}

CMPIObjectPath *lmi_get_computer_system_safe(const CMPIContext *ctx)
{
    CMPIObjectPath *csop = NULL;

    pthread_mutex_lock(&_init_mutex);
    get_computer_system(ctx, &csop, NULL);
    pthread_mutex_unlock(&_init_mutex);

    return csop;
}

bool lmi_check_computer_system(const CMPIContext *ctx,
                               const CMPIObjectPath *computer_system)
{
    CMPIString *nsa = NULL;
    CMPIString *nsb = NULL;
    CMPIString *cls_name = NULL;
    CMPIData data;
    CMPIObjectPath *our = lmi_get_computer_system_safe(ctx);
    CMPIStatus status = {CMPI_RC_OK, NULL};

    if (!our) {
        lmi_error("Failed to get computer system's object path.");
        return false;
    }
    if (!(nsb = CMGetNameSpace(computer_system, NULL)))
        return false;
    if (!(nsa = CMGetNameSpace(our, NULL)))
        return false;
    if (strcmp(CMGetCharsPtr(nsa, NULL), CMGetCharsPtr(nsb, NULL))) {
        lmi_warn("Namespace of computer system does not match.");
        return false;
    }
    if ((cls_name = CMGetClassName(our, NULL)) == NULL)
        return false;
    if (!CMClassPathIsA(_cb, computer_system,
                CMGetCharsPtr(cls_name, NULL), NULL)) {
        lmi_warn("Computer system class \"%s\" does not inherit from \"%s\".",
                CMGetCharsPtr(CMGetClassName(computer_system, NULL), NULL),
                CMGetCharsPtr(cls_name, NULL));
        return false;
    }
    data = CMGetKey(computer_system, "CreationClassName", &status);
    if (status.rc || data.type != CMPI_string ||
            (data.state != CMPI_goodValue && data.state != CMPI_keyValue))
    {
        lmi_warn("Missing CreationClassName property in computer system"
               " instance (status=%u, data.type=%u, data.state=%u).",
               status.rc, data.type, data.state);
        return false;
    }
    if (!CMClassPathIsA(_cb, computer_system,
               CMGetCharsPtr(data.value.string, NULL), &status)) {
        lmi_warn("Bad CreationClassName property in computer system"
               " instance.");
        return false;
    }
    data = CMGetKey(computer_system, "Name", &status);
    if (status.rc || data.type != CMPI_string ||
            (data.state != CMPI_goodValue && data.state != CMPI_keyValue))
    {
        lmi_warn("Missing Name property in computer system instance.");
        return false;
    }
    if (!lmi_is_this_system(ctx, CMGetCharsPtr(data.value.string, NULL))) {
        lmi_warn("Name \"%s\" of computer system do no match this system \"%s\".",
                CMGetCharsPtr(data.value.string, NULL));
        return false;
    }

    return true;
}

const char *lmi_get_system_name(void)
{
    return _system_name;
}

const char *lmi_get_system_name_safe(const CMPIContext *ctx)
{
    const char *csn = NULL;

    pthread_mutex_lock(&_init_mutex);
    get_computer_system(ctx, NULL, &csn);
    pthread_mutex_unlock(&_init_mutex);

    return csn;
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
    char *result = g_key_file_get_string(_masterKeyFile, group, key, NULL);
    if (result == NULL) {
        lmi_warn("Attempted to read unknown group/key: %s/%s", group, key);
    }
    return result;
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

void read_config(GKeyFile *keyFile, const ConfigEntry *config_dict)
{
    while (config_dict != NULL && config_dict->group != NULL && config_dict->key != NULL) {
        if (!g_key_file_has_key(keyFile, config_dict->group,
                                config_dict->key, NULL)) {

            g_key_file_set_value(keyFile, config_dict->group, config_dict->key,
                                 config_dict->value);
        }
        config_dict++;
    }
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
        free(providerconf);
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
    read_config(masterKeyFile, provider_config_defaults);
    // Top-level configs
    read_config(masterKeyFile, _toplevel_config_defaults);

    free(providerconf);
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

    // Read ComputerSystem instance. `_computer_system` and `_system_name` are
    // expected to be initialized by deprecated functions
    // `lmi_get_system_name()` and `lmi_get_computer_system()`. There is a
    // valid case where `get_computer_system()` fails to fetch
    // `CIM_ComputerSystem` instance. It happens at early init when indication
    // subscriptions exist. Broker tries to initialize corresponding before it
    // finishes its own initialization. At that time it's not possible to run
    // any queries.
    get_computer_system(ctx, NULL, NULL);

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



CMPIStatus lmi_check_required_properties(
    const CMPIBroker *cb,
    const CMPIContext *ctx,
    const CMPIObjectPath *o,
    const char *csccn_prop_name,
    const char *csn_prop_name)
{
    CMPIStatus st;
    CMPIData data;

    /* check computer system creation class name */
    data = CMGetKey(o, csccn_prop_name, &st);
    lmi_return_if_status_not_ok(st);
    if (CMIsNullValue(data)) {
        lmi_return_with_chars(cb, CMPI_RC_ERR_FAILED, "%s is empty", csccn_prop_name);
    }
    if (strcmp(KChars(data.value.string), lmi_get_system_creation_class_name()) != 0) {
        lmi_return_with_chars(cb, CMPI_RC_ERR_FAILED, "Wrong %s", csccn_prop_name);
    }

    /* check fqdn */
    data = CMGetKey(o, csn_prop_name, &st);
    lmi_return_if_status_not_ok(st);
    if (CMIsNullValue(data)) {
        lmi_return_with_chars(cb, CMPI_RC_ERR_FAILED, "%s is empty", csn_prop_name);
    }
    if (strcmp(KChars(data.value.string), lmi_get_system_name_safe(ctx)) != 0) {
        lmi_return_with_chars(cb, CMPI_RC_ERR_FAILED, "Wrong %s", csn_prop_name);
    }

    CMReturn(CMPI_RC_OK);
}

CMPIStatus lmi_class_path_is_a(
    const CMPIBroker *cb,
    const char *namespace,
    const char *class,
    const char *class_type)
{
    CMPIObjectPath *o;
    CMPIStatus st = {.rc = CMPI_RC_OK};

    o = CMNewObjectPath(cb, namespace, class, &st);
    if (!o) {
        /* assume that status has been properly set */
        return st;
    } else if (st.rc != CMPI_RC_OK) {
        CMRelease(o);
        return st;
    }
    if (class_type && !CMClassPathIsA(cb, o, class_type, &st)) {
        st.rc = LMI_RC_ERR_CLASS_CHECK_FAILED;
        CMRelease(o);
        return st;
    }
    CMRelease(o);
    return st;
}

const char *lmi_get_string_property_from_objectpath(const CMPIObjectPath *o, const char *prop)
{
    if (!o || !o->ft) {
        return "";
    }

    CMPIData d;
    d = CMGetKey(o, prop, NULL);
    return KChars(d.value.string);
}

const char *lmi_get_string_property_from_instance(const CMPIInstance *i, const char *prop)
{
    if (!i || !i->ft) {
        return "";
    }

    CMPIData d;
    d = CMGetProperty(i, prop, NULL);
    return KChars(d.value.string);
}

static CMPIStatus dump_instance(const CMPIInstance *inst, GString *out)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    gchar *op_str = NULL;
    CMPIData data;
    g_assert(out != NULL);

    data.type = CMPI_ref;
    data.state = CMPI_goodValue;
    data.value.ref = CMGetObjectPath(inst, &status);
    if (status.rc)
        goto err;

    status = lmi_data_to_string(&data, &op_str);
    if (status.rc)
        goto err;
    g_string_append_printf(out, "CMPIInstance(%s)", op_str);
    g_free(op_str);

err:
    return status;
}

static gchar *escape_quotes(const gchar *unescaped_text)
{
    gchar *result = NULL;
    guint escape_cnt = 0;
    guint length = 0;
    guint j = 0;
    for (guint i = 0; unescaped_text[i] > 0; ++i) {
        if (unescaped_text[i] == '\\' || unescaped_text[i] == '"') {
            ++escape_cnt;
        }
        ++length;
    }
    result = g_new(gchar, length + escape_cnt + 1);
    if (result != NULL) {
        for (guint i = 0; i < length; ++i) {
            if (unescaped_text[i] == '\\' || unescaped_text[i] == '"') {
                result[j++] = '\\';
            }
            result[j++] = unescaped_text[i];
        }
        result[length + escape_cnt] = '\0';
    }
    return result;
}

CMPIStatus lmi_data_to_string(const CMPIData *data, gchar **result) {
    gchar *tmpcstr = NULL;
    CMPIDateTime *datetime;
    CMPIString *tmp;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    GString *strbuf = g_string_new("");
    g_assert(result != NULL);

    if (data->state & CMPI_nullValue) {
        g_string_printf(strbuf, "null");
    } else if (data->state & CMPI_badValue) {
        g_string_printf(strbuf, "bad");
    } else if (data->type & CMPI_ARRAY) {
        GString *strbuf = g_string_new("[ ");
        CMPIData tmp;
        CMPICount cnt = CMGetArrayCount(data->value.array, &status);
        if (status.rc)
            goto err;
        for (guint i = 0; i < cnt; ++i) {
            tmp = CMGetArrayElementAt(data->value.array, i, &status);
            if (status.rc)
                goto err;
            status = lmi_data_to_string(&tmp, &tmpcstr);
            if (status.rc)
                goto err;
            g_string_append_printf(strbuf, "%s%s",
                    (i == 0) ? "" : ", ", tmpcstr);
            g_free(tmpcstr);
            tmpcstr = NULL;
        }
        g_string_append_printf(strbuf, "%s",  " ]" + ((cnt == 0) ? 1:0));
    } else {
        switch (data->type) {
        case CMPI_boolean:
            g_string_printf(strbuf, data->value.boolean ? "TRUE" : "FALSE");
            break;
        case CMPI_string:
            tmpcstr = escape_quotes(CMGetCharsPtr(data->value.string, NULL));
            if (tmpcstr == NULL) {
                goto err;
            }
            g_string_printf(strbuf, "\"%s\"", tmpcstr);
            g_free(tmpcstr);
            break;
        case CMPI_chars:
            tmpcstr = escape_quotes(data->value.chars);
            if (tmpcstr == NULL) {
                goto err;
            }
            g_string_printf(strbuf, "\"%s\"", tmpcstr);
            g_free(tmpcstr);
            break;
        case CMPI_uint8:
            g_string_printf(strbuf, "%u", (guint) data->value.uint8);
            break;
        case CMPI_sint8:
            g_string_printf(strbuf, "%d", (gint) data->value.sint8);
            break;
        case CMPI_uint16:
            g_string_printf(strbuf, "%d", (guint) data->value.uint16);
            break;
        case CMPI_sint16:
            g_string_printf(strbuf, "%d", (gint) data->value.sint16);
            break;
        case CMPI_uint32:
            g_string_printf(strbuf, "%u", data->value.uint32);
            break;
        case CMPI_sint32:
            g_string_printf(strbuf, "%d", data->value.sint32);
            break;
        case CMPI_uint64:
            g_string_printf(strbuf, "%llu", data->value.uint64);
            break;
        case CMPI_sint64:
            g_string_printf(strbuf, "%llu", data->value.sint64);
            break;
        case CMPI_dateTime:
            datetime = data->value.dateTime;
            if (CMIsInterval(datetime, NULL)) {
                CMPIUint64 bin = CMGetBinaryFormat(datetime, &status);
                if (status.rc)
                    goto err;
                g_string_printf(strbuf, "%llu", bin);
            } else {
                tmp = CMGetStringFormat(datetime, &status);
                if (status.rc)
                    goto err;
                g_string_printf(strbuf, "%s", CMGetCharsPtr(tmp, NULL));
                CMRelease(tmp);
            }
            break;
        case CMPI_real32:
            g_string_printf(strbuf, "%lf", (gdouble) data->value.real32);
            break;
        case CMPI_real64:
            g_string_printf(strbuf, "%lf", (gdouble) data->value.real64);
            break;
        case CMPI_ref:
            tmp = CMObjectPathToString(data->value.ref, &status);
            if (status.rc)
                goto err;
            g_string_printf(strbuf, "%s", CMGetCharsPtr(tmp, NULL));
            CMRelease(tmp);
            break;
        case CMPI_instance:
            status = dump_instance(data->value.inst, strbuf);
            break;
        default:
            g_string_printf(strbuf,
                    "Cannot serialize data type (%u).", data->type);
            lmi_error(strbuf->str);
            CMSetStatusWithChars(_cb, &status, CMPI_RC_ERR_FAILED, strbuf->str);
            g_string_free(strbuf, TRUE);
            strbuf = NULL;
            goto err;
        }
    }

    *result = g_string_free(strbuf, FALSE);
    return status;

err:
    g_string_free(strbuf, TRUE);
    if (!status.rc) {
        CMSetStatus(&status, CMPI_RC_ERR_FAILED);
        lmi_error("Memory allocation failed");
    } else if (status.msg) {
        lmi_error(CMGetCharsPtr(status.msg, NULL));
    }
    return status;
}

/* vi: set et: */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4 */
/* End: */
