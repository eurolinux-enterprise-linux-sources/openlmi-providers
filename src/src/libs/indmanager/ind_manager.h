/*
 * Copyright (C) 2013-2014 Red Hat, Inc.  All rights reserved.
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
 * Authors: Roman Rakus <rrakus@redhat.com>
 */

#ifndef __IND_MANAGER_H__
#define __IND_MANAGER_H__

/*
 * Limitations:
 *  * Queries *
 *  - supports only DMTF:CQL query language
 *  - select expression has to have ISA predicatea
 *
 *  * Indication types *
 *  - supports only CIM_InstCreation, CIM_InstDeletion, CIM_InstModification
 */

#include <cmpi/cmpidt.h>
#include <cmpi/cmpift.h>
#include <cmpi/cmpimacs.h>

#include <stdbool.h>

/*
 * Usage:
 * 1) Define the following callback functions:
 * IMInstGather
  * Used only when not using polling
  * In this function you should set old and/or new pointers to CMPIInstance.
  * These instances will be send as part of indication.
  * The table bellow show which pointers have to be set and are used
  * indication type	old instance		new instance
  * --------------------------------------------------------
  * IM_IND_CREATION	NULL			instance w/ new values
  * IM_IND_DELETION	instance w/ old values 	NULL
  * IM_IND_MODIFICATION instance w/ old values	instance w/ new values
  *
  * You can use data (see below) pointer to pass any data
  *
  * This function is reentrant. When you return true, the function is called
  * again. Return false, when you don't want to be called again.
 *
 * IMFilterChecker
  * In this function implement checking of filter. Passed filter should be
  * checked. Return true when filter is allowed, false otherwise.
 *
 * IMEventWatcher
  * This is supposed to be blocking function. Returning the true will inform
  * the indication manager that the event has occured. You can set data for
  * further processing in IMInstGather.
 *
 * 2) Create manager object using im_create_manager() during provider init
 * 3) Call other functions as necessary
  * im_destroy_manager() in provider cleanup
  * im_verify_filter() in provider's authorize filter (also in activate filter)
  * im_add_filter() in provider's activate filter
  * im_remove_filter() in provider's deactivate filter
  * im_start_ind() in provider's enable indications
  * im_stop_ind() in provider's disable indications
 */

// forward definitions
typedef struct _IMManager IMManager;

// callback functions
typedef bool (*IMInstGather) (const IMManager *manager, CMPIInstance **old, CMPIInstance **new, void *data);
typedef bool (*IMFilterChecker) (const CMPISelectExp *filter);
typedef bool (*IMEventWatcher) (void **data);

// Indication types
// Only one type supported per instance of manager
typedef enum {
    // for instance indications
    IM_IND_CREATION,     // instance creation
    IM_IND_DELETION,     // instance deletion
    IM_IND_MODIFICATION, // instance modification
} IMIndType;


/*
 * Definition of IMFilter
 * Linked list of all filters
 */
typedef struct _IMFilter {
    struct _IMFilter *next;
    char *select_exp_string;
    CMPIObjectPath *op;
} IMFilter;

typedef struct _IMFilters {
    IMFilter *first;
    char *class_name;
} IMFilters;

/*
 * Linked list of polled enumeration pairs of instances
 * enumerations are converted to arrays
 */
typedef struct _IMEnumerationPair {
    struct _IMEnumerationPair *next;
    CMPIObjectPath *op;
    CMPIArray *prev_enum;
    CMPIArray *this_enum;
    unsigned ref_count;
} IMEnumerationPair;

typedef struct _IMEnumerations {
    IMEnumerationPair *first;
} IMEnumerations;

struct _IMManager {
    // callback functions
    IMEventWatcher watcher;
    IMInstGather gather;
    IMFilterChecker f_checker;
    // filters container
    IMFilters *filters;
    const char** f_allowed_classes;
    // others
    IMIndType type;
    bool running;
    bool polling;
    bool cancelled;
    const CMPIBroker *broker;
    const CMPIContext *ctx_main; /* main thread */
    CMPIContext *ctx_manage; /* manage thread */
    IMEnumerations *enums;
    // threading
    pthread_t _t_manage;
    pthread_mutex_t _t_mutex;
    pthread_cond_t _t_cond;
    // passed data, used for communication between gather/watcher/etc.
    void *data;
};

typedef enum {
    IM_ERR_OK,
    IM_ERR_GATHER,           // bad or null gather callback
    IM_ERR_FILTER_CHECKER,   // bad or null filter checker callback
    IM_ERR_WATCHER,          // bad or null watcher callback
    IM_ERR_MALLOC,           // memory allocation error
    IM_ERR_FILTER,           // bad or null filter
    IM_ERR_MANAGER,          // bad or null manager
    IM_ERR_BROKER,           // bad or null broker
    IM_ERR_CONTEXT,          // bad or null context
    IM_ERR_SELECT_EXP,       // bad or null select expression
    IM_ERR_NOT_FOUND,        // specified data were not found
    IM_ERR_THREAD,           // some error on threading
    IM_ERR_CMPI_RC,          // CMPI status not OK
    IM_ERR_OP,               // bad or null object path
    IM_ERR_CANCELLED,        // job has been cancelled
} IMError;

// Create manager with given properties and callbacks.
// gather may not be specified if you want to use polling
// Return newly created IMManager or NULL and appropiate IMError is set
IMManager* im_create_manager(IMInstGather gather, IMFilterChecker f_checker,
                             bool polling, IMEventWatcher watcher,
                             IMIndType type, const CMPIBroker *broker,
                             IMError *err);

// Destroy given manager.
// Return true when ok or false and IMError is set
bool im_destroy_manager(IMManager *manager, const CMPIContext *ctx,
                        IMError *err);

// Call verification callback on given manager and filter.
// Return true if filter is verified. False if not.
// IMError will be set to to other value than IM_ERR_OK
// if verification failed
bool im_verify_filter(IMManager *manager, const CMPISelectExp *filter,
                      const CMPIContext *ctx, IMError *err);

// add given filter to manager.
// Return true when all is ok, otherwise false and IMError
// is as appropriate.
bool im_add_filter(IMManager *manager, CMPISelectExp *filter,
                   const CMPIContext *ctx, IMError *err);

// Remove given filter from manager.
// Return true when removed, false if not and appropriate IMError is set.
bool im_remove_filter(IMManager *manager, const CMPISelectExp *filter,
                      const CMPIContext *ctx, IMError *err);

// Register list of classes to be used with a default filter checker
// when callback not given during im_create_manager()
// The allowed_classes array should be kept accessible all the time
bool im_register_filter_classes(IMManager *manager,
                                const char** allowed_classes, IMError *err);

// Start indications.
// Return true when correctly started, false if not and
// appropriate IMError is set,
bool im_start_ind(IMManager *manager, const CMPIContext *ctx, IMError *err);

// Stop indications.
// Return true when correctly stopped, false if not and
// appropriate IMError is set,
bool im_stop_ind(IMManager *manager, const CMPIContext *ctx, IMError *err);


#endif /* __IND_MANAGER_H__ */
