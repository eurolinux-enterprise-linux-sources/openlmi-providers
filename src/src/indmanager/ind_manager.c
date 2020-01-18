/*
 * Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
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

#include "ind_manager.h"

#include <cmpi/cmpidt.h>
#include <cmpi/cmpift.h>
#include <cmpi/cmpimacs.h>

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

/* Debugging macros */
#define DEBUG(...) \
    _im_debug(manager->broker, __FILE__, __func__, __LINE__, __VA_ARGS__)
#include <stdarg.h>

static inline void _im_debug(const CMPIBroker *b, const char *file, const char *func,
                             int line, const char *format, ...)
{
    char text[2048] = "";
    snprintf(text, 2047, "%s:%d(%s):", file, line, func);
    va_list args;
    va_start(args, format);
        vsnprintf(text+strlen(text), 2047-strlen(text), format, args);
    va_end(args);
    CMTraceMessage(b, CMPI_LEV_VERBOSE, "LMI indication manager", text, NULL);
}

// Get char * representation of name space from object path
#define NS_STR_FROM_OP(op) CMGetCharPtr(CMGetNameSpace((op), NULL))

// Get char * representation of class name from object path
#define CN_STR_FROM_OP(op) CMGetCharPtr(CMGetClassName((op), NULL))

typedef struct _IMPolledDiff {
    CMPIInstance *iold;
    CMPIInstance *inew;
    struct _IMPolledDiff *next;
} IMPolledDiff;

typedef struct _IMPolledDiffs {
    IMPolledDiff *first;
} IMPolledDiffs;

typedef enum _IMIDiff {
    IM_I_SAME,      // Instances are the same
    IM_I_DIFFERENT, // Instances are different in key properties or value types
    IM_I_CHANGE     // Instances are different in non-key properties
} IMIDiff;

static IMIDiff compare_instances(CMPIInstance *, CMPIInstance *);

/* CMPI rc handler */
CMPIStatus im_rc;

/*
 * Compare 2 object paths
 * return true if they are same, false otherwise
 * is using ft->toString()
 */
static bool compare_objectpaths(CMPIObjectPath *op1, CMPIObjectPath *op2)
{
    return (strcmp(CMGetCharsPtr(CMObjectPathToString(op1, NULL), NULL),
                   CMGetCharsPtr(CMObjectPathToString(op2, NULL), NULL)) == 0);
}

/*
 * Compare 2 cmpi data
 * return true if they are the same, false otherwise
 * note: inspired by value.c:sfcb_comp_CMPIValue() from sblim-sfcb
 */
static bool compare_values(CMPIValue *v1, CMPIValue *v2, CMPIType type)
{
// Checks for NULL, but it is incorrect
#define CHECK_PTRS \
    if (!v1->array && !v2->array) {\
        return true;\
    }\
    if (!v1->array || !v2->array) {\
        return false;\
    }\

    if (type & CMPI_ARRAY) {
        CHECK_PTRS;
        CMPICount c = CMGetArrayCount(v1->array, NULL);
        if (c != CMGetArrayCount(v2->array, NULL)) {
            return false;
        }
        for (; c > 0; c--) {
            CMPIValue val1 = CMGetArrayElementAt(v1->array, c-1, NULL).value;
            CMPIValue val2 = CMGetArrayElementAt(v2->array, c-1, NULL).value;
            if (!compare_values(&val1, &val2, type & ~CMPI_ARRAY)) {
                return false;
            }
        }
        return true;
    }
    switch (type) {
        case CMPI_boolean:
        case CMPI_uint8:
        case CMPI_sint8:
            return (v1->Byte == v2->Byte);
        case CMPI_char16:
        case CMPI_uint16:
        case CMPI_sint16:
            return (v1->Short == v2->Short);
        case CMPI_uint32:
        case CMPI_sint32:
            return (v1->Int == v2->Int);
        case CMPI_real32:
            return (v1->Float == v2->Float);
        case CMPI_uint64:
        case CMPI_sint64:
            return (v1->Long == v2->Long);
        case CMPI_real64:
            return(v1->Double == v2->Double);
        case CMPI_instance:
            CHECK_PTRS;
            return (compare_instances(v1->inst, v2->inst) == IM_I_SAME);
        case CMPI_ref:
            CHECK_PTRS;
            return (compare_objectpaths(v1->ref, v2->ref));
        case CMPI_string:
            CHECK_PTRS;
            return (strcmp(CMGetCharsPtr(v1->string, NULL),
                           CMGetCharsPtr(v2->string, NULL)) == 0);
        case CMPI_dateTime:
            CHECK_PTRS;
            return (strcmp(CMGetCharsPtr(
                        CMGetStringFormat(v1->dateTime, NULL), NULL),
                           CMGetCharsPtr(
                        CMGetStringFormat(v2->dateTime, NULL), NULL)) == 0);
         default:
            return true;
      }
    return true;
}

/*
 * Compare 2 instances.
 * They can be same, different in key properties or different in non-key props
 */
static IMIDiff compare_instances(CMPIInstance *i1, CMPIInstance *i2)
{
    CMPIString *prop_name = NULL;
    // At first compare key properties. They will differ in most cases
    // Then compare non-key properties
    CMPIObjectPath *op1 = CMGetObjectPath(i1, NULL);
    CMPIObjectPath *op2 = CMGetObjectPath(i2, NULL);
    int pcount1 = CMGetKeyCount(op1, NULL);
    int pcount2 = CMGetKeyCount(op2, NULL);

    if (pcount1 != pcount2) {
        // They differ in count of key properties
//        DEBUG("compare instances: Different count of properties");
        return IM_I_DIFFERENT;
    }
    int i;
    CMPIData data1, data2;
    // Check key properties one by one
    for (i = 0; i < pcount1; i++) {
        data1 = CMGetKeyAt(op1, i, &prop_name, NULL);
        data2 = CMGetKey(op2, CMGetCharsPtr(prop_name, NULL), NULL);
        // XXX - is state really CMPI_badValue?
        if (data2.state == CMPI_badValue || data1.type != data2.type ||
            !compare_values(&data1.value, &data2.value, data1.type)) {
            return IM_I_DIFFERENT;
        }
    }
    // Check all properties one by one
    pcount1 = CMGetPropertyCount(i1, NULL);
    pcount2 = CMGetPropertyCount(i2, NULL);
    if (pcount1 != pcount2) {
        return IM_I_DIFFERENT;
    }
    for (i = 0; i < pcount1; i++) {
        data1 = CMGetPropertyAt(i1, i, &prop_name, NULL);
        data2 = CMGetProperty(i2, CMGetCharsPtr(prop_name, NULL), NULL);
        // XXX - is state really CMPI_badValue?
        if (data2.state == CMPI_badValue || data1.type != data2.type) {
            return IM_I_DIFFERENT;
        } else if (!compare_values(&data1.value, &data2.value, data1.type)) {
            return IM_I_CHANGE;
        }
    }
    return IM_I_SAME;
}


/*
 * Remove (free()) the first polled diff from diff list
 * returns nothing
 * Does not perform any checks
 */
/* Called with lock held */
static void remove_diff(IMPolledDiffs *d)
{
    IMPolledDiff *first = d->first;
    if (first) {
        d->first = first->next;
        free (first);
    }
}

/*
 * add instance pair to the list of diffs
 * First argument is diffs list.
 * Returns true when ok, false if not and set IMError
 */
/* Called with lock held */
static bool add_diff(IMPolledDiffs *d, CMPIInstance *newi, CMPIInstance *oldi,
                     IMError *err)
{
        IMPolledDiff *nd = malloc(sizeof(IMPolledDiff));
        if (!nd) {
            *err = IM_ERR_MALLOC;
            return false;
        }
        nd->inew = newi;
        nd->iold = oldi;
        nd->next = NULL;
        if (!d->first) {
            d->first = nd;
        } else {
            IMPolledDiff *last = d->first;
            while (last->next) last = last->next;
            last->next = nd;
        }
        return true;
}

/*
 * Generate diffs based on polled enumerations.
 * Generated diff will contain:
 *  - pair => there is a change
 *  - old NULL and new value => new instance created
 *  - old value and new NULL => instance deletion
 * Return true when ok, false if not and set IMError
 */
/* Called with lock held */
static bool gen_diffs(IMManager *manager, IMPolledDiffs *d, IMError *err)
{
    DEBUG("gen diffs called");
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }
    if (!manager->enums) {
        return false;
    }
    IMEnumerationPair *pair =  manager->enums->first;
    while (pair) {
//        DEBUG("gen diffs - next pair");
        CMPIArray *penum = pair->prev_enum;
        CMPIArray *tenum = pair->this_enum;
        if (penum && tenum) {
            int pcount = CMGetArrayCount(penum, NULL);
            int tcount = CMGetArrayCount(tenum, NULL);
            bool p_used[pcount], t_used[tcount];
            bzero(p_used, pcount);
            bzero(t_used, tcount);
            int pi = 0, ti = 0;
            CMPIInstance *newi = NULL;
            CMPIInstance *oldi = NULL;
            bool do_compare = true;
            for (pi = 0; pi < pcount; pi++) {
                oldi = CMGetArrayElementAt(penum, pi, NULL).value.inst;
                do_compare = true;
                for (ti = 0; ti < tcount && do_compare; ti++) {
                    if (t_used[ti] == true) continue;
                    newi = CMGetArrayElementAt(tenum, ti, NULL).value.inst; // XXX are them really in same order, on the same index?
                    switch (compare_instances(oldi, newi)) {
                        case IM_I_SAME:
                            DEBUG("gen diffs - same instances");
                            do_compare = false;
                            t_used[ti] = true;
                            p_used[pi] = true;
                            break;
                        case IM_I_CHANGE:
                            DEBUG("gen diffs - change in instances");
                            if (manager->type == IM_IND_MODIFICATION) {
                                if (!add_diff(d, newi, oldi, err)) {
                                    return false;
                                }
                            }
                            do_compare = false;
                            t_used[ti] = true;
                            p_used[pi] = true;
                            break;
                        case IM_I_DIFFERENT:
                            DEBUG("gen diffs - different instances");
                        default: break;
                    }
                }
            }
            if (manager->type == IM_IND_DELETION) {
                for (pi = 0; pi < pcount; pi++) {
                    if (p_used[pi] == false) {
                        DEBUG("gen diffs - deleted instance");
                        if (!add_diff(d, NULL,
                            CMGetArrayElementAt(penum, pi, NULL).value.inst,
                            err)) {
                            return false;
                        }
                    }
                }
            }
            if (manager->type == IM_IND_CREATION) {
                for (ti = 0; ti < tcount; ti++) {
                    if (t_used[ti] == false) {
                        DEBUG("gen diffs - created instance");
                        if (!add_diff(d,
                            CMGetArrayElementAt(tenum, ti, NULL).value.inst,
                            NULL, err)) {
                            return false;
                        }
                    }
                }
            }

        }
        pair = pair->next;
    }
    DEBUG("gen diffs returning true");
    return true;
}


/* helper functions */

/*
 * Extract object path from select expression
 * returns match for the first occurence of ISA operator
 */
static CMPIObjectPath *get_object_path(const CMPIBroker *broker, CMPISelectExp *se)
{
    CMPISelectCond *sc = CMGetDoc(se, NULL);
    CMPICount scount = CMGetSubCondCountAndType(sc,NULL,NULL);
    unsigned int i, j;
    for (i = 0; i < scount; i++) {
        CMPISubCond *subcond = CMGetSubCondAt(sc,i,NULL);
        CMPICount pcount = CMGetPredicateCount(subcond,NULL);
        for (j = 0; j < pcount; j++) {
            CMPIPredicate *pred = CMGetPredicateAt(subcond,j,NULL);
            CMPIType type;
            CMPIPredOp op;
            CMPIString *lhs=NULL;
            CMPIString *rhs=NULL;
            CMGetPredicateData(pred,&type,&op,&lhs,&rhs);
            if (op == CMPI_PredOp_Isa) {
                return CMNewObjectPath(broker, "root/cimv2",
                    CMGetCharsPtr(rhs, NULL), NULL);
            }
        }
    }
    return NULL;
}

// compare 2 object paths, return true when equal, false if not
// equality is set by comparing class names and namespaces
static bool equal_ops(CMPIObjectPath *op1, CMPIObjectPath *op2)
{
    return (strcmp(NS_STR_FROM_OP(op1), NS_STR_FROM_OP(op2)) == 0 &&
        strcmp(CN_STR_FROM_OP(op1), CN_STR_FROM_OP(op2)) == 0);
}

// Pair enumerations. This enum will become prev enum and new this enum
// will be created by new obtained by object path
/* Called with lock held */
static bool pair_enumeration(IMManager *manager, IMEnumerationPair *epair)
{
    CMPIEnumeration *aux_e = CBEnumInstances(manager->broker,
        manager->ctx_manage, epair->op, NULL, NULL);
    if (!aux_e) {
        return false;
    }
    CMPIArray *new_e = CMClone(CMToArray(aux_e, NULL), NULL);
    if (!epair->prev_enum && !epair->this_enum) {
        epair->prev_enum = CMClone(new_e, NULL);
        epair->this_enum = new_e;
        return true;
    }
    if (epair->prev_enum) {
        CMRelease(epair->prev_enum);
    }
    epair->prev_enum = epair->this_enum;
    epair->this_enum = new_e;
    return true;
}

// When the filter is added there is no enum pairs created
// This function will generate pairs where they are missing
/* Called with lock held */
static bool first_poll(IMManager *manager, IMError *err)
{
    DEBUG("IM first poll called");
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }
    if (!manager->enums) {
        // Nothing to poll
        return true;
    }
    IMEnumerationPair *epair = NULL;
    for (epair = manager->enums->first; epair; epair = epair->next) {
        if (epair->this_enum == NULL && epair->prev_enum == NULL) {
            if (!pair_enumeration(manager, epair)) {
                return false;
            }
        }
    }
    return true;
}

// Try to find enumeration with given object path
// if found increase reference count
// if not found add it to the end of list with ref count = 1
/* Called with lock held */
static bool add_enumeration(IMManager *manager, CMPIObjectPath *op, IMError *err)
{
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }
    if (!op) {
        *err = IM_ERR_OP;
        return false;
    }
    if (!manager->enums) {
        if (!(manager->enums = malloc(sizeof(IMEnumerations)))) {
            *err = IM_ERR_MALLOC;
            return false;
        }
        manager->enums->first = NULL;
    }
    IMEnumerationPair *e = manager->enums->first;
    IMEnumerationPair *last = NULL;
    while (e) { // find last enumeration
        if (compare_objectpaths(op, e->op)) {
            e->ref_count++;
            return true;
        }
        last = e;
        e = e->next;
    }
    IMEnumerationPair *new_ime = malloc(sizeof(IMEnumerationPair));
    if (!new_ime) {
        *err = IM_ERR_MALLOC;
        return false;
    }
    new_ime->next = NULL;
    new_ime->op = CMClone(op, NULL);
    new_ime->ref_count = 1;
    new_ime->prev_enum = NULL;
    new_ime->this_enum = NULL;
    if (manager->ctx_manage) {
        /* Fill the enumeration with values valid at the time the filter was registered and
         * only if we're adding new filter when indications have been started already */
        pair_enumeration(manager, new_ime);
    }
    if (!last) { // there isn't any enumeration yet
        manager->enums->first = new_ime;
    } else {
        last->next = new_ime;
    }
    return true;
}

/* Called with lock held */
static bool _im_poll(IMManager *manager, IMError *err)
{
    DEBUG("IM poll called");
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }
    if (!manager->enums) {
        // Nothing to poll
        DEBUG("IM poll nothing to poll");
        return true;
    }
    IMEnumerationPair *epair = NULL;
    // go thru all enumerations' object path
    // Use object path to enumerate instances
    DEBUG("IM poll will go thru all filters");
    for (epair = manager->enums->first; epair; epair = epair->next) {
        DEBUG("IM poll next enumeration");
        pair_enumeration(manager, epair);
    }
    DEBUG("IM poll returning true");
    return true;
}

/*
 * Fill ind_inst with properties and send it
 */
static bool send_creation_indication(CMPIInstance *ind_inst, CMPIInstance *new_inst,
                              IMManager *manager)
{
    DEBUG("Instance creation indication to be send");
    if (!manager || !ind_inst || !new_inst) {
        return false;
    }
    CMPIObjectPath *ind_op = CMGetObjectPath(ind_inst, NULL);
    const char *ns = CMGetCharsPtr(CMGetNameSpace(ind_op, NULL), NULL);
    CMSetProperty(ind_inst, "SourceInstance", &new_inst, CMPI_instance);
    CBDeliverIndication(manager->broker, manager->ctx_manage, ns, ind_inst);
    return true;
}

/*
 * Fill ind_inst with properties and send it
 */
static bool send_deletion_indication(CMPIInstance *ind_inst, CMPIInstance *old_inst,
                              IMManager *manager)
{
    DEBUG("Instance deletion indication to be send");
    if (!manager || !ind_inst || !old_inst) {
        return false;
    }
    CMPIObjectPath *ind_op = CMGetObjectPath(ind_inst, NULL);
    const char *ns = CMGetCharsPtr(CMGetNameSpace(ind_op, NULL), NULL);
    CMSetProperty(ind_inst, "SourceInstance", &old_inst, CMPI_instance);
    CBDeliverIndication(manager->broker, manager->ctx_manage, ns, ind_inst);
    return true;
}

/*
 * Fill ind_inst with properties and send it
 */
static bool send_modification_indication(CMPIInstance *ind_inst, CMPIInstance *new,
                                  CMPIInstance *old, IMManager *manager)
{
    DEBUG("Instance modification indication to be send");
    if (!manager || !ind_inst || !new || !old) {
        return false;
    }
    CMPIObjectPath *ind_op = CMGetObjectPath(ind_inst, NULL);
    const char *ns = CMGetCharsPtr(CMGetNameSpace(ind_op, NULL), NULL);
    CMSetProperty(ind_inst, "SourceInstance", &new, CMPI_instance);
    CMSetProperty(ind_inst, "PreviousInstance", &old, CMPI_instance);
    CBDeliverIndication(manager->broker, manager->ctx_manage, ns, ind_inst);
    return true;
}

/*
 * Will extract class name from select expression
 * example: SELECT * FROM LMI_AccountInstCreationIndication WHERE ...
 *          will return LMI_AccountInstCreationIndication
 * returns NULL when not found or on error
 * Caller has to free memory.
 */
static char *get_classname(CMPISelectExp *se)
{
    if (!se) {
        return NULL;
    }
// TODO remove when working
#if 0
    char *select = (char*)CMGetCharsPtr(CMGetSelExpString(se, NULL), NULL);
    char *after_from = strcasestr(select, "from") + strlen("from");
    char *start = after_from + strspn(after_from, " ");
    char *end = index(start,' ');
    char *cname = malloc(end-start+1);
    bzero(cname, end-start+1);
    return memcpy(cname, start, end-start);
#else
    char *select = (char*)CMGetCharsPtr(CMGetSelExpString(se, NULL), NULL);
    char *rest = NULL, *token = NULL, *ptr = select;
    if (!ptr) {
        return NULL;
    }
    for (; ptr || strcasecmp(token, "from") != 0; ptr = NULL) {
        token = strtok_r(ptr, " ", &rest);
        if (!token) {
            break;
        }
    }
    if (token) {
        token = strtok_r(ptr, " ", &rest);
        return strdup(token);
    }
    return NULL;
#endif
}

/*
 * Main send indication function. Will decide by type of indication what
 * to send.
 * Returns true when everything is OK. False otherwise.
 */
/* Called with lock held */
static bool send_indication(CMPIInstance *old, CMPIInstance *new, IMManager *manager)
{
    if (!manager) {
        return false;
    }

    CMPIObjectPath *op = CMNewObjectPath(
        manager->broker,
        CMGetCharsPtr(CMGetNameSpace(manager->polling ? manager->enums->first->op : CMGetObjectPath(new ? new : old, &im_rc), NULL), NULL),
        manager->filters->class_name,
        NULL);
    DEBUG("Will send indication with this OP: %s",
          CMGetCharsPtr(CDToString(manager->broker, op, NULL), NULL));
    CMPIInstance *ind_inst = CMNewInstance(manager->broker, op, NULL);
    DEBUG("Will send indication with this instance): %s",
          CMGetCharsPtr(CDToString(manager->broker, ind_inst, NULL), NULL), NULL);
    DEBUG("Old instance: %p - %s", old,
          CMGetCharsPtr(CDToString(manager->broker, old, NULL), NULL), NULL);
    DEBUG("New instance: %p - %s", new,
          CMGetCharsPtr(CDToString(manager->broker, new, NULL), NULL), NULL);

    CMPIDateTime *date = CMNewDateTimeFromBinary(manager->broker,
                                                 time(NULL) * 1000000,
                                                 0, NULL);
    CMSetProperty(ind_inst, "IndicationTime", &date, CMPI_dateTime);

    switch (manager->type) {
        case IM_IND_CREATION:
            if (!new) {
                return false;
            }
            send_creation_indication(ind_inst, new, manager);
            break;
        case IM_IND_DELETION:
            if (!old) {
                return false;
            }
            send_deletion_indication(ind_inst, old, manager);
            break;
        case IM_IND_MODIFICATION:
            if (!old || !new) {
                return false;
            }
            send_modification_indication(ind_inst, new, old, manager);
            break;
    }
    CMRelease(date);
    return true;
}

/*
 * Run in separate thread
 */
static IMError manage(IMManager *manager)
{
    IMError err = IM_ERR_OK; // TODO - How to handle potential errors?
    IMPolledDiffs diffs = {NULL};
    CMPIInstance *iold = NULL, *inew = NULL;

    while (1) {
        DEBUG("manage thread in infinite loop");
        // wait until manager is running
        pthread_mutex_lock(&manager->_t_mutex);
        if (manager->cancelled) {
            pthread_mutex_unlock(&manager->_t_mutex);
            return IM_ERR_CANCELLED;
        }
        while (!manager->running) {
            DEBUG("manage thread waiting to indication start");
            pthread_cond_wait(&manager->_t_cond, &manager->_t_mutex);
        }

        if (manager->polling) {
            if (!first_poll(manager, &err)) {
                pthread_mutex_unlock(&manager->_t_mutex);
                return err;
            }
        }
        pthread_mutex_unlock(&manager->_t_mutex);

        DEBUG("manage thread calling watcher");
        if(!manager->watcher(&manager->data)) {
            continue;
        }
        pthread_mutex_lock(&manager->_t_mutex);
        if (manager->cancelled) {
            pthread_mutex_unlock(&manager->_t_mutex);
            return IM_ERR_CANCELLED;
        }
        if (manager->polling) {
            // poll enumerations
            if (!_im_poll(manager, &err)) {
                pthread_mutex_unlock(&manager->_t_mutex);
                return err;
            }
            // create list of diffs
            if (!gen_diffs(manager, &diffs, &err)) {
                while (diffs.first)
                    remove_diff(&diffs);
                pthread_mutex_unlock(&manager->_t_mutex);
                return err;
            }
            manager->data = &diffs;
        }
        DEBUG("manage thread calling gather");
        while (manager->gather(manager, &iold, &inew, manager->data)) {
            send_indication(iold, inew, manager);
        }
        pthread_mutex_unlock(&manager->_t_mutex);
    }
}

static void manage_cleanup(void *data)
{
    IMManager *manager = (IMManager *)data;

    DEBUG("manage thread - detaching thread manager context = %p", manager->ctx_manage);
    CBDetachThread(manager->broker, manager->ctx_manage);
}

/* Wrapper thread function, to isolate code control flow due to sensitive pthread_cleanup_ macros */
static void *manage_wrapper(void *data)
{
    // TODO: Switch Enable/Disable state on some places?
    IMManager *manager = (IMManager *)data;
    IMError err;

    DEBUG("manage thread started");

    /* Allow thread cancellation */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    DEBUG("manage thread - attaching thread manager context = %p", manager->ctx_manage);
    CBAttachThread(manager->broker, manager->ctx_manage);

    /* Main execution block closed in a cleanup handler */
    pthread_cleanup_push(manage_cleanup, data);
    err = manage(manager);
    pthread_cleanup_pop(1);

    DEBUG("manage thread stopped");

    return (void *)err;
}

/*
 * Default gather function using polling
 * It is going thru all polled instances. If there is some pair (previous enum
 * and this enum)
 */
/* Called with lock held */
static bool default_gather (const IMManager *manager, CMPIInstance **old_inst,
                            CMPIInstance **new_inst, void* data)
{
    // passed data is a list of diffs
    // It is going to be called in loop
    // return true if now we are going to create indication
    // return false if there will be no indication and loop will break
    IMPolledDiffs *diffs = (IMPolledDiffs *) data;
    if (diffs->first) {
        *new_inst = diffs->first->inew;
        *old_inst = diffs->first->iold;
        remove_diff(diffs);
        return true;
    }
    return false;
}

/*
 * Remove every polled enums
 * true if ok, false if not
 */
static bool remove_all_enums(IMManager *manager, IMError *err)
{
    if (!manager->enums) {
        return true;
    }
    IMEnumerationPair *current = manager->enums->first;
    IMEnumerationPair *next = NULL;
    while (current) {
        next = current->next;
        CMRelease(current->prev_enum);
        CMRelease(current->this_enum);
        CMRelease(current->op);
        free(current);
        current = next;
    }
    free(manager->enums);
    manager->enums = NULL;
    return true;
}

/*
 * true if ok
 * false if not
 */
static bool remove_all_filters(IMManager *manager, IMError *err)
{
    if (!manager->filters) {
        return true;
    }
    IMFilter *current = manager->filters->first;
    IMFilter *next = NULL;
    while (current) {
        next = current->next;
        free(current->select_exp_string);
        CMRelease(current->op);
        free(current);
        current = next;
    }
    free(manager->filters->class_name);
    free(manager->filters);
    manager->filters = NULL;

    if (!remove_all_enums(manager, err)) {
        return false;
    }

    return true;
}

IMManager* im_create_manager(IMInstGather gather, IMFilterChecker f_checker,
                             bool polling, IMEventWatcher watcher,
                             IMIndType type, const CMPIBroker *broker,
                             IMError *err)
{
    if ((!polling && !gather) || (polling && gather) ) {
        *err = IM_ERR_GATHER;
        return NULL;
    }
    if (!watcher) {
        *err = IM_ERR_WATCHER;
        return NULL;
    }
    if (!broker) {
        *err = IM_ERR_BROKER;
        return NULL;
    }
    IMManager* manager = malloc(sizeof(IMManager));
    if (!manager) {
        *err = IM_ERR_MALLOC;
        return NULL;
    }
    manager->type = type;
    manager->gather = gather ? gather : default_gather;
    manager->watcher = watcher;
    manager->filters = NULL;
    manager->running = false;
    manager->polling = polling;
    manager->cancelled = false;
    manager->broker = broker;
    manager->f_checker = f_checker;
    manager->enums = NULL;
    manager->ctx_main = NULL;
    manager->ctx_manage = NULL;
    manager->data = NULL;
    DEBUG("Manager created");

    if (pthread_cond_init(&manager->_t_cond, NULL) ||
            pthread_mutex_init(&manager->_t_mutex, NULL)) {
        *err = IM_ERR_THREAD;
        free(manager);
        return NULL;
    }

    return manager;
}

bool im_destroy_manager(IMManager *manager, const CMPIContext *ctx,
                        IMError *err)
{
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }
    if (!ctx) {
        *err = IM_ERR_CONTEXT;
        return false;
    }
    if (manager->running)
        im_stop_ind(manager, ctx, err);
    /* No need for locking as all running threads should be stopped at this point */
    if (!remove_all_filters(manager, err))
        return false;
    DEBUG("Destroying manager");
    if (pthread_mutex_destroy(&manager->_t_mutex) ||
        pthread_cond_destroy(&manager->_t_cond)) {
        *err = IM_ERR_THREAD;
        return false;
    }
    free(manager);
    return true;
}

static bool default_ind_filter_cb(IMManager *manager, const CMPISelectExp *filter)
{
    /* Looks for a class to be subscribed and matches it against a list of
     * allowed classes. Filter query may contain more conditions, only those
     * with a ISA operator are looked for.
     */
    CMPIStatus st;
    unsigned int i;

    CMPISelectCond *sec = CMGetDoc(filter, &st);
    if (!sec)
        return false;
    CMPICount count = CMGetSubCondCountAndType(sec, NULL, &st);
    if (count != 1)
        return false;
    CMPISubCond *sub = CMGetSubCondAt(sec, 0, &st);
    if (!sub)
        return false;
    count = CMGetPredicateCount(sub, &st);
    if (count == 0)
        return false;

    for (i = 0; i < count; i++) {
        CMPIType type;
        CMPIPredOp op;
        CMPIString *lhs = NULL;
        CMPIString *rhs = NULL;
        CMPIPredicate *pred = CMGetPredicateAt(sub, i, &st);
        if (!pred)
            return false;
        st = CMGetPredicateData(pred, &type, &op, &lhs, &rhs);
        if (st.rc != CMPI_RC_OK || op != CMPI_PredOp_Isa)
            continue;
        const char *rhs_str = CMGetCharsPtr(rhs, &st);
        if (!rhs_str)
            continue;
        while (manager->f_allowed_classes[i]) {
            if (strcasecmp(rhs_str, manager->f_allowed_classes[i++]) == 0)
                return true;
        }
    }
    return false;
}

bool im_verify_filter(IMManager *manager, const CMPISelectExp *filter,
                      const CMPIContext *ctx, IMError *err)
{
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return NULL;
    }
    if (!ctx) {
        *err = IM_ERR_CONTEXT;
        return NULL;
    }
    if (!filter) {
        *err = IM_ERR_FILTER;
        return NULL;
    }
    if (!manager->f_checker && !manager->f_allowed_classes) {
        *err = IM_ERR_FILTER_CHECKER;
        return NULL;
    }
    manager->ctx_main = ctx;
    if (manager->f_checker) {
        DEBUG("Verifying filter. Manager = %p, filter checker = %p", manager, manager->f_checker);
        return manager->f_checker(filter);
    } else {
        DEBUG("Verifying filter. Manager = %p, using default filter checker", manager);
        return default_ind_filter_cb(manager, filter);
    }
}

/*
 * Because Pegasus cannot easily clone nor copy select expression
 * we need to dig internals and store them directly.
 * Return true when ok, or NULL and error set
 * appropiately
 */
/* Called with lock held */
static bool _im_add_filter(IMManager *manager, CMPISelectExp *se, IMError *err)
{
    if (!se) {
        *err = IM_ERR_SELECT_EXP;
        return false;
    }

    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }

    CMPIString *str = CMGetSelExpString(se, &im_rc);
    if (!str) {
        *err = IM_ERR_CMPI_RC;
        return false;
    }

    const char *query = CMGetCharsPtr(str, &im_rc);
    if (!query) {
        *err = IM_ERR_CMPI_RC;
        return false;
    }
    char *ses = NULL;
    if (!(ses = strdup(query))) {
        *err = IM_ERR_MALLOC;
        return false;
    }
    CMPIObjectPath *op = get_object_path(manager->broker, se);
    if (!op) {
        *err = IM_ERR_SELECT_EXP;
        free(ses);
        return false;
    }

    if(!manager->filters) {
        DEBUG("This is the first filter creation");
        IMFilters *filters = malloc(sizeof(IMFilters));
        if (!filters) {
            *err = IM_ERR_MALLOC;
            CMRelease(op);
            free(ses);
            return false;
        }
        if (!(filters->class_name = get_classname(se))) {
            *err = IM_ERR_MALLOC;
            CMRelease(op);
            free(ses);
            free(filters);
            return false;
        }
        filters->first = NULL;
        manager->filters = filters;
    }

    DEBUG("Adding filter to the list");
    IMFilter *current = manager->filters->first;
    IMFilter *prev = NULL;
    while (current) {
        prev = current;
        current = current->next;
    }
    IMFilter *next = malloc(sizeof(IMFilter));
    if (!next) {
        *err = IM_ERR_MALLOC;
        CMRelease(op);
        free(ses);
        return false;
    }
    next->next = NULL;
    next->select_exp_string = ses;
    next->op = CMClone(op, NULL);
    if (prev) {
        prev->next = next;
    } else {
        manager->filters->first = next;
    }
    CMRelease(op);

    return true;
}

bool im_add_filter(IMManager *manager, CMPISelectExp *filter,
                   const CMPIContext *ctx, IMError *err)
{
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }
    pthread_mutex_lock(&manager->_t_mutex);
    if (!ctx) {
        *err = IM_ERR_CONTEXT;
        pthread_mutex_unlock(&manager->_t_mutex);
        return false;
    }
    if (!filter) {
        *err = IM_ERR_FILTER;
        pthread_mutex_unlock(&manager->_t_mutex);
        return false;
    }
    manager->ctx_main = ctx;
    DEBUG("Adding filter");
    if (!_im_add_filter(manager, filter, err)) {
        pthread_mutex_unlock(&manager->_t_mutex);
        return false;
    }
    if (manager->polling) {
        if (!add_enumeration(manager,
                             get_object_path(manager->broker, filter),
                             err)) {
            pthread_mutex_unlock(&manager->_t_mutex);
            return false;
        }
    }
    pthread_mutex_unlock(&manager->_t_mutex);
    return true;
}

bool im_register_filter_classes(IMManager *manager,
                                const char** allowed_classes, IMError *err)
{
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }
    manager->f_allowed_classes = allowed_classes;
    return true;
}

/*
 * Decrease reference count for polled enumerations. If count is 0, remove
 * the whole enumerations.
 * Returns true/false on success/fail and set err.
 */
/* Called with lock held */
static bool maybe_remove_polled(IMManager *manager, CMPIObjectPath *op, IMError *err)
{
    if (!manager->enums || !manager->enums->first) {
        *err = IM_ERR_NOT_FOUND;
        return false;
    }
    IMEnumerationPair *current = manager->enums->first;
    IMEnumerationPair *prev = NULL;
    while (current) {
        if (compare_objectpaths(op, current->op)) {
            if (--current->ref_count == 0) {
                if (prev) {
                    prev->next = current->next;
                } else {
                    manager->enums->first = current->next;
                }
                if (current->prev_enum)
                    CMRelease(current->prev_enum);
                if (current->this_enum)
                    CMRelease(current->this_enum);
                CMRelease(current->op);
            }
            return true;
        }
        prev = current;
        current = current->next;
    }
    *err = IM_ERR_NOT_FOUND;
    return false;
}

bool im_remove_filter(IMManager *manager, const CMPISelectExp *filter,
                      const CMPIContext *ctx, IMError *err)
{
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }
    if (!ctx) {
        *err = IM_ERR_CONTEXT;
        return false;
    }
    if (!filter) {
        *err = IM_ERR_FILTER;
        return false;
    }
    pthread_mutex_lock(&manager->_t_mutex);
    if (!manager->filters) {
        pthread_mutex_unlock(&manager->_t_mutex);
        *err = IM_ERR_NOT_FOUND;
        return false;
    }
    manager->ctx_main = ctx;
    DEBUG("Removing filter");
    IMFilter *current = manager->filters->first;
    IMFilter *prev = NULL;
    while (current) {
        if (strcmp(current->select_exp_string,
            CMGetCharsPtr(CMGetSelExpString(filter, NULL), NULL)) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                manager->filters->first = current->next;
            }
            if (manager->polling && !maybe_remove_polled(manager, current->op,
                                                         err)) {
                pthread_mutex_unlock(&manager->_t_mutex);
                return false;
            }
            CMRelease(current->op);
            free(current->select_exp_string);
            free(current);
            pthread_mutex_unlock(&manager->_t_mutex);
            return true;
        }
        prev = current;
        current = current->next;
    }
    *err = IM_ERR_NOT_FOUND;
    pthread_mutex_unlock(&manager->_t_mutex);
    return false;
}

// start indications
bool im_start_ind(IMManager *manager, const CMPIContext *ctx, IMError *err)
{
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }
    if (!ctx) {
        *err = IM_ERR_CONTEXT;
        return false;
    }
    if (manager->running) {
        DEBUG("WARNING: thread already exists");
        *err = IM_ERR_THREAD;
        return false;
    }
    manager->cancelled = false;
    manager->ctx_main = ctx;
    manager->ctx_manage = CBPrepareAttachThread(manager->broker,
                                                manager->ctx_main);
    DEBUG("Creating second thread");
    if (pthread_create(&manager->_t_manage, NULL, manage_wrapper, manager)) {
        *err = IM_ERR_THREAD;
        return false;
    }
    DEBUG("Starting indications");
    manager->running = true;

    pthread_cond_signal(&manager->_t_cond);
    return true;
}

// stop indications
bool im_stop_ind(IMManager *manager, const CMPIContext *ctx, IMError *err)
{
    if (!manager) {
        *err = IM_ERR_MANAGER;
        return false;
    }
    if (!ctx) {
        *err = IM_ERR_CONTEXT;
        return false;
    }
    manager->ctx_main = ctx;
    DEBUG("Stopping indications");
    manager->running = false;

    /* First lock the mutex so we are sure the thread does not hold it. */
    pthread_mutex_lock(&manager->_t_mutex);
    /* Then cancel the thread in deferred mode and set a private flag.
     * The thread may be doing unlocked stuff that will cancel just fine
     * or may be waiting for mutex acquisition where it will cancel and
     * unlock right after that using our private flag.
     */
    manager->cancelled = true;
    /* Note that mutex functions ARE NOT cancellation points! */
    if (pthread_cancel(manager->_t_manage)) {
        *err = IM_ERR_THREAD;
        pthread_mutex_unlock(&manager->_t_mutex);
        return false;
    }

    /* Unlock the mutex and give the thread chance to cancel properly. */
    pthread_mutex_unlock(&manager->_t_mutex);

    /* Wait until thread cancellation is finished. */
    if (pthread_join(manager->_t_manage, NULL)) {
        *err = IM_ERR_THREAD;
        return false;
    }

    /* Cleanup */
    remove_all_enums(manager, err);
    manager->data = NULL;
    manager->ctx_manage = NULL;
    manager->cancelled = false;

    return true;
}

