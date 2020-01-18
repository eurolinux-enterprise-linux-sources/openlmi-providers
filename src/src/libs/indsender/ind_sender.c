/*
 * Copyright (C) 2014 Red Hat, Inc.  All rights reserved.
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
 * Authors: Michal Minar <miminar@redhat.com>
 */
#include <stdio.h>
#include <glib.h>
#include <pthread.h>
#include "openlmi.h"
#include "openlmi-utils.h"
#include "ind_sender.h"

/* Prefix used in indications's class name creation. */
static gchar *_name_prefix;
/**
 * Number of calls to ind_sender_init() minus calls to ind_sender_cleanup().
 * This is to ensure that static variables are initialised upon initialization
 * of the first provider that needs us and cleaned up upon clean up of last
 * provider using us. */
static gint _init_count = 0;
/* Manipulated by CIMOM. */
static gint _indications_enabled = 0;
/* Maps keys to filter query strings.
 * Key is a string composed of `<cim_class_name>:<filter_id>`. */
static GHashTable *_filter_table = NULL;
/* Set of filter keys that are activated. It's a subset of keys in
 * _filter_table. */
static GTree *_subscribed_filters = NULL;
/* Mutex for accessing static variables. */
static pthread_mutex_t _filter_mutex = PTHREAD_MUTEX_INITIALIZER;

const gchar *ind_sender_static_filter_names[] = {
    "Changed",
    "Created",
    "Deleted",
    "Failed",
    "PercentUpdated",
    "Succeeded",
};

/**
 * Treat keys of _subscribed_filters in case-insensitive way.
 */
static gint keys_compare(gconstpointer a,
                         gconstpointer b,
                         gpointer unused)
{
    return g_ascii_strcasecmp((const gchar *) a, (const gchar *) b);
}

/**
 * Create a key identifying static filter.
 *
 * @return number of bytes written without the terminating '\0'
 */
static gint make_filter_key(gchar *buf,
                            guint buflen,
                            const gchar *class_name,
                            const gchar *filter_id)
{
    return g_snprintf(buf, buflen, "%s:%s", class_name, filter_id);
}

/**
 * Create filter name used to fill IndicationFilterName property of created
 * CIM_Indication.
 *
 * @return number of bytes written without the terminating '\0'
 */
static gint make_filter_name(gchar *buf,
                             guint buflen,
                             const gchar *class_name,
                             const gchar *filter_id)
{
    gint res = g_snprintf(buf, buflen, LMI_ORGID ":");
    res += make_filter_key(buf + res, buflen - res, class_name, filter_id);
    return res;
}

/**
 * Find registered static filter with matching query.
 *
 * @note caller must ensure that _filter_mutex is locked during execution
 *      of this function.
 *
 * @param query value checked for exact match against registered filters.
 * @param filter_key if not NULL, will be filled with filter's corresponding
 *      key in _filter_table.
 * @param class_name if not NULL, will be filled with class name this filter
 *      belongs to.
 * @param filter_id if not NULL, will be filled with corresponding filter_id.
 * @returns status with CMPI_RC_OK code if found. Code CMPI_RC_ERR_NOT_FOUND
 *      will be set if not found.
 */
static CMPIStatus get_matching_filter(const gchar *query,
                                      gchar **filter_key,
                                      gchar **class_name,
                                      gchar **filter_id)
{
    CMPIStatus status = {CMPI_RC_ERR_NOT_FOUND, NULL};
    GHashTableIter iter;
    const gchar *value;
    const gchar *key;
    const gchar *cls_name_end;

    g_assert(_filter_table);

    g_hash_table_iter_init(&iter, _filter_table);
    while (g_hash_table_iter_next(&iter, (gpointer) &key, (gpointer) &value)) {
        if (g_ascii_strcasecmp(query, value))
            continue;
        cls_name_end = index(key, ':');
        if (  (filter_key && (*filter_key = g_strdup(key)) == NULL)
           || (  class_name
              && (*class_name = g_strndup(key, cls_name_end - key)) == NULL)
           || (  filter_id
              && (*filter_id = g_strdup(cls_name_end + 1)) == NULL))
        {
            if (class_name && *class_name) {
                g_free((gchar *) *class_name);
                *class_name = NULL;
            }
            lmi_error("Memory allocation failed");
            CMSetStatus(&status, CMPI_RC_ERR_FAILED);
        }else {
            CMSetStatus(&status, CMPI_RC_OK);
        }
        break;
    }

    return status;
}

/**
 * Checks whether static filter is subscribed and indications are enabled.
 *
 * @return TRUE if indication can be sent.
 */
static gboolean is_subscribed(const gchar *class_name, const gchar *filter_id)
{
    gboolean found = FALSE;
    gchar filter_key[BUFLEN];

    if (_indications_enabled)
    {
        make_filter_key(filter_key, BUFLEN, class_name, filter_id);
        if (g_tree_lookup_extended(
                    _subscribed_filters, filter_key, NULL, NULL)) {
            found = TRUE;
        }
    }

    return found;
}

/**
 * Similar to is_subscribed(). This function obtains class_name directly from
 * instance.
 *
 * @param inst subject of indication (source instance).
 * @return TRUE if indication can be sent.
 */
static gboolean is_subscribed_inst(CMPIInstance *inst, const gchar *filter_id)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIObjectPath *iop;
    CMPIString *clsname;
    iop = CMGetObjectPath(inst, &status);
    if (status.rc || !iop)
        return FALSE;
    clsname = CMGetClassName(iop, &status);
    if (status.rc || !clsname)
        return FALSE;
    return is_subscribed(CMGetCharPtr(clsname), filter_id);
}

/**
 * Deliver indication to a broker. Indication is released in any case.
 */
static CMPIStatus do_send(const CMPIBroker *cb,
                          const CMPIContext *ctx,
                          CMPIInstance *indication)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIObjectPath *iop;
    CMPIString *tmp;
    const gchar *namespace;
    const gchar *opstr;

    if ((iop = CMGetObjectPath(indication, &status)) == NULL) {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_FAILED,
                "Failed to get indication's object path!");
    } else if (!CMClassPathIsA(cb, iop, "CIM_Indication", &status) || status.rc)
    {
        if (!status.rc)
            CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                    "Can not set indication which does not inherit from"
                    " CIM_Indication!");
    } else if ((tmp = CMGetNameSpace(iop, &status)) == NULL
       || status.rc
       || (namespace = CMGetCharsPtr(tmp, &status)) == NULL
       || status.rc)
    {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_FAILED,
                "Failed to get indication's namespace!");
    } else {
        if (  (tmp = CMObjectPathToString(iop, NULL))
           && (opstr = CMGetCharsPtr(tmp, NULL)))
        {
            lmi_info("Delivering indication \"%s\".", opstr);
            CMRelease(tmp);
        } else {
            lmi_info("Delivering indication.");
        }
        status = CBDeliverIndication(cb, ctx, namespace, indication);
    }
    CMRelease(indication);
    if (status.rc)
        lmi_error("Failed to deliver indication: %s",
                status.msg ? CMGetCharsPtr(status.msg, NULL)
                           : "unknown reason");
    return status;
}

/**
 * Make indication class name. This class name must be registered
 * with broker. It's composed of <ORGID>, name prefix and suffix.
 * Choose suffix from "InstCreation", "InstModification" and
 * "InstDeletion".
 *
 * @return number of characters printed to *cls_name_buf*.
 */
static gint make_indication_class_name(
        gchar *cls_name_buf,
        guint buflen,
        const gchar *suffix)
{
    return g_snprintf(cls_name_buf, buflen, LMI_ORGID "_%s%s",
                _name_prefix, suffix);
}

/**
 * Create indication instance. Just a minimal set of properties will
 * be filled.
 *
 * @param class_name_suffix is an argument for make_indication_class_name().
 * @param source_instance will be copied and assigned to SourceInstance
 *      property.
 * @param indication points to a place where pointer to newly allocated
 *      indication instance will be stored.
 */
static CMPIStatus make_indication_inst(const CMPIBroker *cb,
                                       const CMPIContext *ctx,
                                       const gchar *class_name_suffix,
                                       CMPIInstance *source_instance,
                                       const gchar *filter_id,
                                       CMPIInstance **indication)
{
    g_assert(indication);
    CMPIStatus status;
    gchar filter_name[BUFLEN];
    const gchar *namespace;
    gchar clsname[BUFLEN];
    CMPIObjectPath *src_op;
    CMPIString *src_clsname;
    CMPIString *tmp;
    CMPIObjectPath *iop;
    CMPIValue value;

    if ((iop = CMGetObjectPath(source_instance, &status)) == NULL) {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "Failed to get indication's object path!");
    } else if (  (tmp = CMGetNameSpace(iop, &status)) == NULL || status.rc
              || (namespace = CMGetCharsPtr(tmp, &status)) == NULL || status.rc)
    {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "Failed to get indication's namespace!");
    } else {
        make_indication_class_name(clsname, BUFLEN, class_name_suffix);
        if (  (iop = CMNewObjectPath(cb, namespace, clsname, &status)) == NULL
           || status.rc)
            goto err;
        if ((*indication = CMNewInstance(cb, iop, &status)) == NULL || status.rc)
            goto op_err;
        if (  (value.inst = CMClone(source_instance, &status)) == NULL
           || status.rc)
            goto indication_err;
        if (CMSetProperty(*indication, "SourceInstance",
                        &value, CMPI_instance).rc)
            goto source_instance_err;
        if ((value.string = CMNewString(cb, lmi_get_system_name_safe(ctx),
                        &status)) == NULL || status.rc)
            goto indication_err;
        if (CMSetProperty(*indication, "SourceInstanceHost",
                    &value, CMPI_string).rc)
            goto string_err;
        if ((src_op = CMGetObjectPath(source_instance, &status)) == NULL ||
                status.rc)
            goto indication_err;
        if ((value.string = CMObjectPathToString(src_op, &status)) == NULL ||
                    status.rc)
            goto indication_err;
        if (CMSetProperty(*indication, "SourceInstanceModelPath",
                    &value, CMPI_string).rc)
            goto string_err;
        if ((src_clsname = CMGetClassName(src_op, &status)) == NULL)
            goto indication_err;
        make_filter_name(filter_name, BUFLEN,
            CMGetCharsPtr(src_clsname, NULL), filter_id);
        if (  (value.string = CMNewString(cb, filter_name, &status)) == NULL
           || status.rc)
            goto indication_err;
        if (CMSetProperty(*indication, "IndicationFilterName",
                    &value, CMPI_string).rc)
            goto string_err;
        value.uint16 = 2;
        if (CMSetProperty(*indication, "PerceivedSeverity", &value, CMPI_uint16).rc)
            goto indication_err;
    }

    return status;

source_instance_err:
    CMRelease(value.inst);
    goto indication_err;
string_err:
    CMRelease(value.string);
indication_err:
    CMRelease(*indication);
op_err:
    CMRelease(iop);
err:
    lmi_error("Memory allocation failed");
    CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    return status;
}

static CMPIStatus send_indication(const CMPIBroker *cb,
                                  const CMPIContext *ctx,
                                  CMPIInstance *indication,
                                  const gchar *filter_id)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    gboolean send_it = (filter_id == NULL);
    CMPIData tmp;

    if (filter_id) {
        tmp = CMGetProperty(indication, "SourceInstance", &status);
        if (status.rc || tmp.type != CMPI_instance) {
            CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                    "Expected indication instance with"
                    " \"SourceInstance\" property.");
        } else {
            send_it = is_subscribed_inst(tmp.value.inst, filter_id);
        }
    }
    if (!status.rc && send_it) {
        status = do_send(cb, ctx, indication);
    } else {
        CMRelease(indication);
    }
    return status;
}

CMPIStatus ind_sender_init(const gchar *name_prefix)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};

    pthread_mutex_lock(&_filter_mutex);
    g_assert(_init_count >= 0);
    g_assert(_init_count > 0 || _filter_table == NULL);

    if (_init_count == 0) {
        if ((_name_prefix = g_strdup(name_prefix)) == NULL)
            goto err;

        if (  ((_filter_table = g_hash_table_new_full(
                    lmi_str_lcase_hash_func, lmi_str_icase_equal,
                    g_free, g_free)) == NULL)
           || ((_subscribed_filters = g_tree_new_full(
                    keys_compare, NULL, g_free, NULL)) == NULL))
            goto name_alloced_err;

        lmi_debug("Indication sender initialized for %s", name_prefix);
    }
    ++_init_count;

    pthread_mutex_unlock(&_filter_mutex);
    return status;

name_alloced_err:
    if (_filter_table) {
        g_hash_table_unref(_filter_table);
        _filter_table = NULL;
    }
    g_free(_name_prefix);
err:
    pthread_mutex_unlock(&_filter_mutex);
    lmi_error("Memory allocation failed");
    CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    return status;
}

CMPIStatus ind_sender_cleanup()
{
    pthread_mutex_lock(&_filter_mutex);
    g_assert(_init_count > 0);
    g_assert(_filter_table);

    if (_init_count == 1) {
        g_hash_table_destroy(_filter_table);
        g_tree_destroy(_subscribed_filters);
        _indications_enabled = FALSE;
    }
    --_init_count;

    pthread_mutex_unlock(&_filter_mutex);
    lmi_debug("Cleanup of indication sender finished.");
    CMReturn(CMPI_RC_OK);
}

CMPIStatus ind_sender_add_static_filters(const gchar *class_name,
                                         const StaticFilter *filters,
                                         gint n_filters)
{
    g_assert(filters);
    g_assert(n_filters > 0);

    gchar buf[BUFLEN];
    gchar *key;
    gchar *query;
    CMPIStatus status = {CMPI_RC_OK, NULL};

    lmi_debug("Adding static filters for class %s.", class_name);
    pthread_mutex_lock(&_filter_mutex);
    g_assert(_filter_table);

    for (gint i=0; i < n_filters; ++i) {
        make_filter_key(buf, BUFLEN, class_name, filters[i].id);
        if (g_hash_table_lookup(_filter_table, buf) != NULL) {
            lmi_error("Trying to add already registered filter \"%s\".", buf);
            CMSetStatus(&status, CMPI_RC_ERR_ALREADY_EXISTS);
            break;
        }
        if ((query = g_strdup(filters[i].query)) == NULL) {
            lmi_error("Memory allocation failed");
            CMSetStatus(&status, CMPI_RC_ERR_FAILED);
            break;
        }

        if ((key = g_strdup(buf)) == NULL)
            goto err;
        g_hash_table_insert(_filter_table, key, query);
    }
    pthread_mutex_unlock(&_filter_mutex);

    lmi_debug("Added static filters for class %s.", class_name);

    return status;

err:
    pthread_mutex_unlock(&_filter_mutex);
    lmi_error("Memory allocation failed");
    CMSetStatus(&status, CMPI_RC_ERR_FAILED);
    return status;
}

gboolean ind_sender_authorize_filter(const CMPISelectExp *filter,
                                     const gchar *class_name,
                                     const CMPIObjectPath *class_op,
                                     const gchar *owner)
{
    CMPIString *str = CMGetSelExpString(filter, NULL);
    const gchar *query = CMGetCharPtr(str);
    GHashTableIter iter;
    const gchar *value;
    const gchar *cls_name_end;
    gboolean found = FALSE;

    pthread_mutex_lock(&_filter_mutex);
    g_assert(_filter_table);

    g_hash_table_iter_init(&iter, _filter_table);
    while (g_hash_table_iter_next(&iter, NULL, (gpointer *) &value)) {
        if (g_ascii_strcasecmp(query, value))
            continue;
        cls_name_end = index(value, ':');
        if (g_ascii_strncasecmp(class_name, value, cls_name_end - value))
            continue;
        found = TRUE;
        break;
    }

    pthread_mutex_unlock(&_filter_mutex);

    if (found) {
        lmi_info("Authorized static filter \"%s\" for class \"%s\".",
                cls_name_end + 1, class_name);
    }else {
        lmi_info("Refused to authorize filter \"%s\".", query);
    }
    return found;
}

CMPIStatus ind_sender_activate_filter(const CMPISelectExp *filter,
                                      const gchar *class_name,
                                      const CMPIObjectPath *class_op,
                                      gboolean first_activation)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIString *str = CMGetSelExpString(filter, NULL);
    const gchar *query = CMGetCharPtr(str);
    gchar *fltr_key;
    gchar *filter_id;

    pthread_mutex_lock(&_filter_mutex);
    g_assert(_filter_table);

    status = get_matching_filter(query, &fltr_key, NULL, &filter_id);
    if (status.rc != CMPI_RC_OK) {
        if (status.rc == CMPI_RC_ERR_NOT_FOUND)
            lmi_warn("Can't activate not registered filter \"%s\"!", query);
    } else if (is_subscribed(class_name, filter_id)) {
        lmi_info("Static filter \"%s\" for class \"%s\" is already active.",
                filter_id, class_name);
    } else {
        g_tree_insert(_subscribed_filters, fltr_key, NULL);
        lmi_info("Activated static filter \"%s\" for class \"%s\".",
                filter_id, class_name);
    }

    pthread_mutex_unlock(&_filter_mutex);
    return status;
}

CMPIStatus ind_sender_deactivate_filter(const CMPISelectExp *filter,
                                        const gchar *class_name,
                                        const CMPIObjectPath *class_op,
                                        gboolean last_activation)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIString *str = CMGetSelExpString(filter, NULL);
    const gchar *query = CMGetCharPtr(str);
    gchar *fltr_key;
    gchar *filter_id;

    pthread_mutex_lock(&_filter_mutex);
    g_assert(_filter_table);

    status = get_matching_filter(query, &fltr_key, NULL, &filter_id);
    if (status.rc != CMPI_RC_OK) {
        if (status.rc == CMPI_RC_ERR_NOT_FOUND)
            lmi_warn("Can't deactivate not registered filter \"%s\"!", query);
    }else if (!g_tree_remove(_subscribed_filters, fltr_key)) {
        lmi_warn("Can't deactivate inactive filter \"%s\" for class \"%s\"!",
                filter_id, class_name);
        CMSetStatus(&status, CMPI_RC_ERR_NOT_FOUND);
    }else {
        lmi_info("Deactivated static filter \"%s\" for class \"%s\".",
                filter_id, class_name);
    }

    pthread_mutex_unlock(&_filter_mutex);
    return status;
}

void enable_indications() {
    pthread_mutex_lock(&_filter_mutex);
    if (_indications_enabled <= 0) {
        _indications_enabled = 1;
        lmi_info("Indication sending enabled.");
    } else {
        ++_indications_enabled;
    }
    pthread_mutex_unlock(&_filter_mutex);
}

void disable_indications() {
    pthread_mutex_lock(&_filter_mutex);
    if (_indications_enabled <= 1) {
        _indications_enabled = 0;
        lmi_info("Indication sending disabled.");
    } else {
        --_indications_enabled;
    }
    pthread_mutex_unlock(&_filter_mutex);
}

CMPIStatus ind_sender_send_indication(const CMPIBroker *cb,
                                      const CMPIContext *ctx,
                                      CMPIInstance *indication,
                                      const gchar *filter_id)
{
    CMPIStatus status = {CMPI_RC_OK, NULL};

    pthread_mutex_lock(&_filter_mutex);
    status = send_indication(cb, ctx, indication, filter_id);
    pthread_mutex_unlock(&_filter_mutex);
    return status;
}

CMPIStatus ind_sender_send_instcreation(const CMPIBroker *cb,
                                        const CMPIContext *ctx,
                                        CMPIInstance *source_instance,
                                        const gchar *filter_id)
{
    CMPIObjectPath *iop;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIInstance *indication;
    gchar *clsname;

    iop = CMGetObjectPath(source_instance, &status);
    if (status.rc || !iop) {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "Missing object path in source instance!");
    } else if (  (clsname = CMGetCharPtr(CMGetClassName(iop, &status))) == NULL
              || status.rc)
    {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "Missing class name in source instance name!");
    } else {
        pthread_mutex_lock(&_filter_mutex);
        if (is_subscribed(clsname, filter_id)) {
            if (!(status = make_indication_inst(cb, ctx, "InstCreation",
                            source_instance, filter_id, &indication)).rc)
                status = send_indication(cb, ctx, indication, filter_id);
        } else {
            lmi_debug("Ignoring indication matching inactive filter %s:%s",
                    clsname, filter_id);
        }
        pthread_mutex_unlock(&_filter_mutex);
    }
    return status;
}

CMPIStatus ind_sender_send_instmodification(const CMPIBroker *cb,
                                            const CMPIContext *ctx,
                                            CMPIInstance *old_instance,
                                            CMPIInstance *new_instance,
                                            const gchar *filter_id)
{
    CMPIObjectPath *iop;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIInstance *indication;
    gchar *clsname;
    CMPIValue value;

    iop = CMGetObjectPath(new_instance, &status);
    if (status.rc || !iop) {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "MIssing object path in new instance!");
    } else if (  (clsname = CMGetCharPtr(CMGetClassName(iop, &status))) == NULL
              || status.rc)
    {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "Missing class name in new instance name!");
    } else {
        pthread_mutex_lock(&_filter_mutex);
        if (is_subscribed(clsname, filter_id)) {
            if (!(status = make_indication_inst(
                        cb, ctx, "InstModification", new_instance,
                        filter_id, &indication)).rc)
            {
                if ((value.inst = CMClone(old_instance, &status)) == NULL) {
                    lmi_error("Memory allocation failed");
                    CMSetStatus(&status, CMPI_RC_ERR_FAILED);
                } else if (CMSetProperty(indication, "PreviousInstance",
                            &value, CMPI_instance).rc)
                {
                    lmi_error("Memory allocation failed");
                    CMSetStatus(&status, CMPI_RC_ERR_FAILED);
                } else {
                    status = send_indication(cb, ctx, indication, filter_id);
                }
            }
        } else {
            lmi_debug("Ignoring indication matching inactive filter %s:%s",
                    clsname, filter_id);
        }
        pthread_mutex_unlock(&_filter_mutex);
    }
    return status;
}

CMPIStatus ind_sender_send_instdeletion(const CMPIBroker *cb,
                                        const CMPIContext *ctx,
                                        CMPIInstance *source_instance,
                                        const gchar *filter_id)
{
    CMPIObjectPath *iop;
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIInstance *indication;
    gchar *clsname;

    iop = CMGetObjectPath(source_instance, &status);
    if (status.rc || !iop) {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "MIssing object path in source instance!");
    } else if (  (clsname = CMGetCharPtr(CMGetClassName(iop, &status))) == NULL
              || status.rc)
    {
        CMSetStatusWithChars(cb, &status, CMPI_RC_ERR_INVALID_PARAMETER,
                "Missing class name in source instance name!");
    } else {
        pthread_mutex_lock(&_filter_mutex);
        if (is_subscribed(clsname, filter_id)) {
            if (!(status = make_indication_inst(cb, ctx, "InstDeletion",
                            source_instance, filter_id, &indication)).rc)
                status = send_indication(cb, ctx, indication, filter_id);
        } else {
            lmi_debug("Ignoring indication matching inactive filter %s:%s",
                    clsname, filter_id);
        }
        pthread_mutex_unlock(&_filter_mutex);
    }
    return status;
}
