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

#ifndef IND_SENDER_H
#define IND_SENDER_H

#include <cmpimacs.h>
#include <glib.h>

#define STATIC_FILTER_ID_CHANGED \
    ind_sender_static_filter_names[IND_SENDER_STATIC_FILTER_ENUM_CHANGED]
#define STATIC_FILTER_ID_CREATED \
    ind_sender_static_filter_names[IND_SENDER_STATIC_FILTER_ENUM_CREATED]
#define STATIC_FILTER_ID_DELETED \
    ind_sender_static_filter_names[IND_SENDER_STATIC_FILTER_ENUM_DELETED]
#define STATIC_FILTER_ID_FAILED \
    ind_sender_static_filter_names[IND_SENDER_STATIC_FILTER_ENUM_FAILED]
#define STATIC_FILTER_ID_PERCENT_UPDATED \
    ind_sender_static_filter_names[\
        IND_SENDER_STATIC_FILTER_ENUM_PERCENT_UPDATED]
#define STATIC_FILTER_ID_SUCCEEDED \
    ind_sender_static_filter_names[IND_SENDER_STATIC_FILTER_ENUM_SUCCEEDED]

typedef enum {
    IND_SENDER_STATIC_FILTER_ENUM_CHANGED,
    IND_SENDER_STATIC_FILTER_ENUM_CREATED,
    IND_SENDER_STATIC_FILTER_ENUM_DELETED,
    IND_SENDER_STATIC_FILTER_ENUM_FAILED,
    IND_SENDER_STATIC_FILTER_ENUM_PERCENT_UPDATED,
    IND_SENDER_STATIC_FILTER_ENUM_SUCCEEDED,
    IND_SENDER_STATIC_FILTER_ENUM_LAST,
}IndSenderStaticFilterEnum;

extern const gchar *ind_sender_static_filter_names[];

/**
 * Container of static filter's data used for registration with indication
 * sender.
 */
typedef struct _StaticFilter {
    const gchar *id;
    const gchar *query;
} StaticFilter;

/**
 * Intitialize indication sender's data structures.
 *
 * @note call this function on your provider's startup if you want to send
 *      indications. Call it after lmi_init().
 *
 * @param name_prefix defines part of indication class name. It usually matches
 *      name of implemented profile. For example LMI_SoftwareInstCreation class
 *      name results from name_prefix="Software".
 */
CMPIStatus ind_sender_init(const gchar *name_prefix);
/**
 * Call this function in cleanup handler of your provider if ind_sender_init()
 * was used in your init function.
 */
CMPIStatus ind_sender_cleanup();

/**
 * Register static filters with indication handler. No indications will be
 * sent unless matching static filter is registered, authorized and activated.
 * This needs to be called on provider's startup to register any static filters
 * for particular CIM class. Static filters are identified with a pair
 * (class_name, filter_id).
 *
 * @param class_name CIM class name residing in a FORM clause of filter's
 *      query.
 * @param filters an array of static filters to register with particular CIM
 *      class.
 * @param n_filters number of filters in *filters* parameter.
 */
CMPIStatus ind_sender_add_static_filters(const gchar *class_name,
                                         const StaticFilter *filters,
                                         gint n_filters);

/**
 * Callback for CIMOM for use in your provider. It asks us to verify client
 * shall be allowed to subscribe to particular query. Query must exactly match
 * one static query belonging to registered filter.
 *
 * @param filter query to authorize.
 * @param class_name CIM class name matching given query. It must match
 *      associated class name of static filter used at registration time.
 * @param class_op object path indentifying given class.
 * @param owner destination owner.
 * @return TRUE if static filter is registered.
 */
gboolean ind_sender_authorize_filter(const CMPISelectExp *filter,
                                     const gchar *class_name,
                                     const CMPIObjectPath *class_op,
                                     const gchar *owner);

/**
 * Callback for CIMOM for use in your provider. It asks us to send
 * indications matching given static filter generated from now on.
 *
 * @param filter static query that needs to be registered.
 * @param class_name CIM class name matching given query. It must match
 *      associated class name of static filter used at registration time.
 * @param class_op object path indentifying given class.
 * @param first_activation shall be set to TRUE if this is the first filter for
 *      class_name.
 */
CMPIStatus ind_sender_activate_filter(const CMPISelectExp *filter,
                                      const gchar *class_name,
                                      const CMPIObjectPath *class_op,
                                      gboolean first_activation);

/**
 * Callback for CIMOM for use in your provider. It asks us to ignore
 * indications matching given static filter generated from now on.
 *
 * @param filter static query that needs to be registered.
 * @param class_name CIM class name matching given query. It must match
 *      associated class name of static filter used at registration time.
 * @param class_op object path indentifying given class.
 * @param last_activation shall be set to TRUE if this is the last filter for
            class_name.
 */
CMPIStatus ind_sender_deactivate_filter(const CMPISelectExp *filter,
                                        const gchar *class_name,
                                        const CMPIObjectPath *class_op,
                                        gboolean last_activation);

/**
 * Callback for CIMOM for use in your provider. Tells us that indications can
 * now be generated. The MB is now prepared to process indications. The
 * function is normally called by the MB after having done its intialization
 * and processing of persistent subscription requests.
 */
void enable_indications();

/*
 * Callback for CIMOM for use in your provider. Tells us that indications can
 * Tells us that we should stop generating indications. MB will not accept
 * any indications until enabled again. The function is normally called
 * when the MB is shutting down indication services either temporarily or
 * permanently.
 */
void disable_indications();

/**
 * Send indication if the corresponding static filter is active and indications
 * enabled. Matching filter is identified by a class name of indication's
 * source instance and given filter_id.
 *
 * @note indication will be released in any case.
 *
 * @param indication subject to sent. It must inherit from CIM_Instance.
 * @param filter_id identifies static filter associated with particular class.
 */
CMPIStatus ind_sender_send_indication(const CMPIBroker *cb,
                                      const CMPIContext *ctx,
                                      CMPIInstance *indication,
                                      const gchar *filter_id);

/**
 * Create an instance of LMI_<prefix>InstCreation and send it if the corresponding
 * static filter is active and indications enabled. Matching filter is
 * identified by a class name of source instance and given filter_id.
 *
 * @param source_instance value of indication's SourceInstance property.
 *      You can safely release this instance later on.
 * @param filter_id identifies static filter associated with particular class.
 */
CMPIStatus ind_sender_send_instcreation(const CMPIBroker *cb,
                                        const CMPIContext *ctx,
                                        CMPIInstance *source_instance,
                                        const gchar *filter_id);

/**
 * Create an instance of LMI_<prefix>InstModification and send it if the
 * corresponding static filter is active and indications enabled. Matching
 * filter is identified by a class name of source instance and given filter_id.
 *
 * @param old_instance value of indication's SourceInstance property.
 *      You can safely release this instance later on.
 * @param new_instance value of indication's PreviousInstance property.
 *      You can safely release this instance later on.
 * @param filter_id identifies static filter associated with particular class.
 */
CMPIStatus ind_sender_send_instmodification(const CMPIBroker *cb,
                                            const CMPIContext *ctx,
                                            CMPIInstance *old_instance,
                                            CMPIInstance *new_instance,
                                            const gchar *filter_id);

/**
 * Create an instance of LMI_<prefix>InstDeletion and send it if the
 * corresponding static filter is active and indications enabled. Matching
 * filter is identified by a class name of source instance and given filter_id.
 *
 * @param source_instance value of indication's SourceInstance property.
 *      You can safely release this instance later on.
 * @param filter_id identifies static filter associated with particular class.
 */
CMPIStatus ind_sender_send_instdeletion(const CMPIBroker *cb,
                                        const CMPIContext *ctx,
                                        CMPIInstance *source_instance,
                                        const gchar *filter_id);

#endif /* end of include guard: IND_SENDER_H */
