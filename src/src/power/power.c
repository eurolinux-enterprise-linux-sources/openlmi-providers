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
 */

#include "power.h"

#include <stdlib.h>
#include <assert.h>

#ifdef HAS_GDBUS
#  include <gio/gio.h>

#  define LOGIND_NAME "org.freedesktop.login1"
#  define LOGIND_PATH "/org/freedesktop/login1"
#  define LOGIND_INTERFACE "org.freedesktop.login1.Manager"

#  define Proxy GDBusProxy
#else
   // Fake GDBus types
#  define Proxy void
#endif

#include "LMI_AssociatedPowerManagementService.h"
#include "LMI_PowerConcreteJob.h"

const char *provider_name = "powermanagement";
const ConfigEntry *provider_config_defaults = NULL;

struct _Power {
    unsigned int instances;
    unsigned short requestedPowerState;
    unsigned short transitioningToPowerState;
    const CMPIBroker *broker;
    CMPI_MUTEX_TYPE mutex;
    GList *jobs; // list of PowerStateChangeJob
};

#define MUTEX_LOCK(power) power->broker->xft->lockMutex(power->mutex)
#define MUTEX_UNLOCK(power) power->broker->xft->unlockMutex(power->mutex)

struct _PowerStateChangeJob {
    size_t id;
    const CMPIBroker *broker;
    Power *power;
    unsigned short requestedPowerState;
    unsigned short jobState;
    int timeOfLastChange;
    int timeBeforeRemoval;
    int cancelled;
    int superseded; // There is another job that overrides this job
    char *error;
    CMPI_THREAD_TYPE thread;
    CMPI_MUTEX_TYPE mutex;
};

static size_t job_max_id = 0;

Power *_power = NULL;

Power *power_new(const CMPIBroker *_cb, const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
    Power *power = malloc(sizeof(Power));
    if (power == NULL) {
        return NULL;
    }
    power->broker = _cb;
    power->instances = 0;
    power->requestedPowerState = LMI_AssociatedPowerManagementService_RequestedPowerState_Unknown;
    power->transitioningToPowerState = LMI_AssociatedPowerManagementService_TransitioningToPowerState_No_Change;
    power->mutex = _cb->xft->newMutex(0);
    power->jobs = NULL;
#if !defined(GLIB_VERSION_2_36)
    g_type_init();
#endif
    return power;
}

void power_destroy(Power *power)
{
}

Power *power_ref(const CMPIBroker *_cb, const CMPIContext *ctx)
{
    if (_power == NULL) {
        if ((_power = power_new(_cb, ctx)) == NULL) {
            return NULL;
        }
    }
    MUTEX_LOCK(_power);
    _power->instances++;
    MUTEX_UNLOCK(_power);
    return _power;
}

void power_unref(Power *power)
{
    if (power == NULL) {
        return;
    }
    MUTEX_LOCK(power);
    power->instances--;
    MUTEX_UNLOCK(power);
    if (power->instances == 0) {
        power_destroy(power);
        power = NULL;
        _power = NULL;
    }
}

Proxy *power_create_logind()
{
#ifdef HAS_GDBUS
    GError *err = NULL;
    GDBusProxy *logind_proxy;
    if ((logind_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE, NULL, LOGIND_NAME, LOGIND_PATH,
        LOGIND_INTERFACE, NULL, &err)) == NULL) {

        lmi_error("Unable to connect to logind via DBus: %s", err->message);
        g_error_free(err);
        return NULL;
    }
    // Logind always has some properties, if no property is cached,
    // the proxied object doesn't exist => don't use logind
    if (g_dbus_proxy_get_cached_property_names(logind_proxy) == NULL) {
        g_object_unref(logind_proxy);
        lmi_debug("Logind DBus interface is not available");
        return NULL;
    }
    return logind_proxy;
#else
    return NULL;
#endif
}

bool power_call_logind(Proxy *proxy, const char *method)
{
#ifdef HAS_GDBUS
    GVariant *result;
    GError *err = NULL;
    if (proxy != NULL) {
        if ((result = g_dbus_proxy_call_sync(proxy, method,
                g_variant_new("(b)", false), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                &err)) == NULL) {

            lmi_error("Unable to call: %s", method, err->message);
            g_error_free(err);
            return false;
        } else {
            g_variant_unref(result);
            return true;
        }
    }
    return false;
#endif
    return false;
}

bool power_check_logind(Proxy *proxy, const char *method)
{
#ifdef HAS_GDBUS
    GVariant *result;
    GError *err = NULL;
    if (proxy != NULL) {
        if ((result = g_dbus_proxy_call_sync(proxy, method, g_variant_new("()"),
            G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err)) == NULL) {

            lmi_error("Unable to call %s: %s", method, err->message);
            g_error_free(err);
            return false;
        } else {
            const char *r = g_variant_get_string(g_variant_get_child_value(result, 0), NULL);
            if (strcasecmp(r, "yes") == 0) {
                g_variant_unref(result);
                return true;
            }
            g_variant_unref(result);
            return true;
        }
    }
#endif
    return false;
}

unsigned short power_requested_power_state(Power *power)
{
    return power->requestedPowerState;
}

unsigned short power_transitioning_to_power_state(Power *power)
{
    return power->transitioningToPowerState;
}

void *state_change_thread(void *data)
{
    PowerStateChangeJob *powerStateChangeJob = data;
    MUTEX_LOCK(powerStateChangeJob);
    powerStateChangeJob->jobState = LMI_PowerConcreteJob_JobState_Running;
    powerStateChangeJob->timeOfLastChange = time(NULL);
    MUTEX_UNLOCK(powerStateChangeJob);


    // Check if the job was cancelled
    if (powerStateChangeJob->cancelled) {
        MUTEX_LOCK(powerStateChangeJob);
        powerStateChangeJob->jobState = LMI_PowerConcreteJob_JobState_Terminated;
        powerStateChangeJob->timeOfLastChange = time(NULL);
        MUTEX_UNLOCK(powerStateChangeJob);

        if (!powerStateChangeJob->superseded) {
            // There is no job that replaced this job
            MUTEX_LOCK(powerStateChangeJob->power);
            powerStateChangeJob->power->transitioningToPowerState = LMI_AssociatedPowerManagementService_TransitioningToPowerState_No_Change;
            MUTEX_UNLOCK(powerStateChangeJob->power);
        }

        lmi_debug("State change thread cancelled\n");
        return NULL;
    }

    Proxy *logind_proxy = power_create_logind();
    // Execute the job

    int succeeded = 0;
    switch (powerStateChangeJob->requestedPowerState) {
        case LMI_AssociatedPowerManagementService_PowerState_Sleep__Deep:
            // Sleep
            succeeded = power_call_logind(logind_proxy, "Suspend");

            if (!succeeded) {
                succeeded = system("pm-suspend") == 0;
            }
            break;
        case LMI_AssociatedPowerManagementService_PowerState_Power_Cycle_Off___Soft:
            // Reboot (without shutting down programs)
#ifdef HAS_SYSTEMCTL
            succeeded = system("systemctl --force reboot &") == 0;
#else
            succeeded =  system("reboot --force &") == 0;
#endif
            break;
        case LMI_AssociatedPowerManagementService_PowerState_Hibernate_Off___Soft:
            // Hibernate
            succeeded = power_call_logind(logind_proxy, "Hibernate");

            if (!succeeded) {
                succeeded = system("pm-hibernate") == 0;
            }
            break;
        case LMI_AssociatedPowerManagementService_PowerState_Off___Soft:
            // Poweroff (without shutting down programs)
#ifdef HAS_SYSTEMCTL
            succeeded = system("systemctl --force poweroff &") == 0;
#else
            succeeded =  system("poweroff --force &") == 0;
#endif
            break;
        case LMI_AssociatedPowerManagementService_PowerState_Off___Soft_Graceful:
            // Poweroff (shut down programs first)
            succeeded = power_call_logind(logind_proxy, "PowerOff");

            if (!succeeded) {
                succeeded = system("poweroff &") == 0;
            }
            break;
        case LMI_AssociatedPowerManagementService_PowerState_Power_Cycle_Off___Soft_Graceful:
            // Reboot (shut down programs first)
            succeeded = power_call_logind(logind_proxy, "Reboot");

            if (!succeeded) {
                succeeded =  system("reboot &") == 0;
            }
            break;
    }

    MUTEX_LOCK(powerStateChangeJob->power);
    powerStateChangeJob->power->transitioningToPowerState = LMI_AssociatedPowerManagementService_TransitioningToPowerState_No_Change;
    MUTEX_UNLOCK(powerStateChangeJob->power);

    MUTEX_LOCK(powerStateChangeJob);
    if (succeeded) {
        powerStateChangeJob->jobState = LMI_PowerConcreteJob_JobState_Completed;
    } else {
        powerStateChangeJob->jobState = LMI_PowerConcreteJob_JobState_Exception;
#ifdef HAS_UPOWER
        if (error != NULL) {
            powerStateChangeJob->error = error->message;
        }
#endif
    }
    powerStateChangeJob->timeOfLastChange = time(NULL);
    MUTEX_UNLOCK(powerStateChangeJob);

    lmi_debug("State change thread finished\n");
    return NULL;
}

int power_request_power_state(Power *power, unsigned short state)
{
    int rc = CMPI_RC_OK;

    int count, found = 0;
    unsigned short *states = power_available_requested_power_states(power, &count);
    if (states == NULL) {
        return CMPI_RC_ERR_FAILED;
    }
    for (int i = 0; i < count; ++i) {
        if (states[i] == state) {
            found = 1;
            break;
        }
    }
    free(states);
    if (!found) {
        lmi_error("Invalid state requested: %d\n",  state);
        return CMPI_RC_ERR_INVALID_PARAMETER;
    }

    PowerStateChangeJob *powerStateChangeJob = malloc(sizeof(PowerStateChangeJob));
    if (powerStateChangeJob == NULL) {
        lmi_error("Memory allocation failed");
        return CMPI_RC_ERR_FAILED;
    }
    powerStateChangeJob->id = job_max_id++;
    powerStateChangeJob->broker = power->broker;
    powerStateChangeJob->power = power;
    powerStateChangeJob->mutex = power->broker->xft->newMutex(0);
    powerStateChangeJob->requestedPowerState = state;
    powerStateChangeJob->jobState = LMI_PowerConcreteJob_JobState_New;
    powerStateChangeJob->cancelled = 0;
    powerStateChangeJob->superseded = 0;
    powerStateChangeJob->timeOfLastChange = time(NULL);
    powerStateChangeJob->timeBeforeRemoval = 300;
    powerStateChangeJob->error = NULL;

    MUTEX_LOCK(power);
    power->requestedPowerState = state;
    power->transitioningToPowerState = state;

    PowerStateChangeJob *job;
    GList *plist = power->jobs;
    while (plist) {
        job = plist->data;
        MUTEX_LOCK(job);
        if (job->jobState != LMI_PowerConcreteJob_JobState_Suspended &&
            job->jobState != LMI_PowerConcreteJob_JobState_Killed &&
            job->jobState != LMI_PowerConcreteJob_JobState_Terminated) {

            job->cancelled = 1;
            job->superseded = 1;
            job->jobState = LMI_PowerConcreteJob_JobState_Shutting_Down;
            job->timeOfLastChange = time(NULL);
        }
        MUTEX_UNLOCK(job);
        plist = g_list_next(plist);
    }
    powerStateChangeJob->thread = power->broker->xft->newThread(state_change_thread, powerStateChangeJob, 1);
    power->jobs = g_list_append(power->jobs, powerStateChangeJob);
    MUTEX_UNLOCK(power);
    lmi_debug("State change thread started\n");

    return rc;
}

unsigned short *power_available_requested_power_states(Power *power, int *count)
{
    unsigned short *list = malloc(17 * sizeof(unsigned short));
    if (list == NULL) {
        lmi_error("Memory allocation failed");
        return NULL;
    }
    int i = 0;

#ifdef HAS_GDBUS
    GDBusProxy *logind_proxy = power_create_logind();
#else
    void *logind_proxy = NULL;
#endif


    /* 1 Other
     * LMI_AssociatedPowerManagementService_PowerState_Other
     */

    /* 2 On
     * corresponding to ACPI state G0 or S0 or D0.
     *
     * Bring system to full On from any state (Sleep, Hibernate, Off)
     *
     * LMI_AssociatedPowerManagementService_PowerState_On
     */
    // not supported

    /* 3 Sleep - Light
     * corresponding to ACPI state G1, S1/S2, or D1.
     *
     * Standby
     *
     * LMI_AssociatedPowerManagementService_PowerState_Sleep___Light
     */
    // not supported

    /* 4 Sleep - Deep
     * corresponding to ACPI state G1, S3, or D2.
     *
     * Suspend
     *
     * LMI_AssociatedPowerManagementService_PowerState_Sleep__Deep
     */
    // Sleep
    if (logind_proxy) {
        if (power_check_logind(logind_proxy, "CanSuspend")) {
            list[i++] = LMI_AssociatedPowerManagementService_PowerState_Sleep__Deep;
        }
    } else {
        if (system("pm-is-supported --suspend") == 0) {
            list[i++] = LMI_AssociatedPowerManagementService_PowerState_Sleep__Deep;
        }
    }

    /* 5 Power Cycle (Off - Soft)
     * corresponding to ACPI state G2, S5, or D3, but where the managed
     * element is set to return to power state "On" at a pre-determined time.
     *
     * Reset system without removing power
     *
     * LMI_AssociatedPowerManagementService_PowerState_Power_Cycle_Off___Soft
     */
    // Reboot (without shutting down programs)
    list[i++] = LMI_AssociatedPowerManagementService_PowerState_Power_Cycle_Off___Soft;

    /* 6 Off - Hard
     * corresponding to ACPI state G3, S5, or D3.
     *
     * Power Off performed through mechanical means like unplugging
     * power cable or UPS On
     *
     * LMI_AssociatedPowerManagementService_PowerState_Off___Hard
     */

    /* 7 Hibernate (Off - Soft)
     * corresponding to ACPI state S4, where the state of the managed element
     * is preserved and will be recovered upon powering on.
     *
     * System context and OS image written to non-volatile storage;
     * system and devices powered off
     *
     * LMI_AssociatedPowerManagementService_PowerState_Hibernate_Off___Soft
     */
    // Hibernate
    if (logind_proxy) {
        if (power_check_logind(logind_proxy, "CanHibernate")) {
            list[i++] = LMI_AssociatedPowerManagementService_PowerState_Hibernate_Off___Soft;
        }
    } else {
        if (system("pm-is-supported --hibernate") == 0) {
            list[i++] = LMI_AssociatedPowerManagementService_PowerState_Hibernate_Off___Soft;
        }
    }

    /* 8 Off - Soft
     * corresponding to ACPI state G2, S5, or D3.
     *
     * System power off but auxiliary or flea power may be available
     *
     * LMI_AssociatedPowerManagementService_PowerState_Off___Soft
     */
    // Poweroff (without shutting down programs)
    list[i++] = LMI_AssociatedPowerManagementService_PowerState_Off___Soft;

    /* 9 Power Cycle (Off-Hard)
     * corresponds to the managed element reaching the ACPI state G3
     * followed by ACPI state S0.
     *
     * Equivalent to Off–Hard followed by On
     *
     * LMI_AssociatedPowerManagementService_PowerState_Power_Cycle_Off_Hard
     */
    // not implemented

    /* 10 Master Bus Reset
     * corresponds to the system reaching ACPI state S5 followed by ACPI
     * state S0. This is used to represent system master bus reset.
     *
     * Hardware reset
     *
     * LMI_AssociatedPowerManagementService_PowerState_Master_Bus_Reset
     */
    // not implemented

    /* 11 Diagnostic Interrupt (NMI)
     * corresponding to the system reaching ACPI state S5 followed by ACPI
     * state S0. This is used to represent system non-maskable interrupt.
     *
     * Hardware reset
     *
     * LMI_AssociatedPowerManagementService_PowerState_Diagnostic_Interrupt_NMI
     */
    // not implemented

    /* 12 Off - Soft Graceful
     * equivalent to Off Soft but preceded by a request to the managed element
     * to perform an orderly shutdown.
     *
     * System power off but auxiliary or flea power may be available but preceded
     * by a request to the managed element to perform an orderly shutdown.
     *
     * LMI_AssociatedPowerManagementService_PowerState_Off___Soft_Graceful
     */
    // Poweroff (shut down programs first)
    list[i++] = LMI_AssociatedPowerManagementService_PowerState_Off___Soft_Graceful;

    /* 13 Off - Hard Graceful
     * equivalent to Off Hard but preceded by a request to the managed element
     * to perform an orderly shutdown.
     *
     * Power Off performed through mechanical means like unplugging power cable
     * or UPS On but preceded by a request to the managed element to perform
     * an orderly shutdown.
     *
     * LMI_AssociatedPowerManagementService_PowerState_Off___Hard_Graceful
     */
    // not implemented

    /* 14 Master Bus Rest Graceful
     * equivalent to Master Bus Reset but preceded by a request to the managed
     * element to perform an orderly shutdown.
     *
     * Hardware reset but preceded by a request to the managed element
     * to perform an orderly shutdown.
     *
     * LMI_AssociatedPowerManagementService_PowerState_Master_Bus_Reset_Graceful
     */
    // not implemented

    /* 15 Power Cycle (Off - Soft Graceful)
     * equivalent to Power Cycle (Off - Soft) but preceded by a request
     * to the managed element to perform an orderly shutdown.
     *
     * Reset system without removing power but preceded by a request
     * to the managed element to perform an orderly shutdown.
     *
     * LMI_AssociatedPowerManagementService_PowerState_Power_Cycle_Off___Soft_Graceful
     */
    // Reboot (shut down programs first)
    list[i++] = LMI_AssociatedPowerManagementService_PowerState_Power_Cycle_Off___Soft_Graceful;

    /* 16 Power Cycle (Off - Hard Graceful)
     * equivalent to Power Cycle (Off - Hard) but preceded by a request
     * to the managed element to perform an orderly shutdown.
     *
     * Equivalent to Off–Hard followed by On but preceded by a request
     * to the managed element to perform an orderly shutdown.
     *
     * LMI_AssociatedPowerManagementService_PowerState_Power_Cycle_Off___Hard_Graceful
     */
    // not implemented

    *count = i;
    return list;
}

void job_free(PowerStateChangeJob *job)
{
    job->broker->xft->destroyMutex(job->mutex);
}

GList *power_get_jobs(Power *power)
{
    PowerStateChangeJob *powerStateChangeJob;
    GList *plist = power->jobs;
    while (plist) {
        powerStateChangeJob = plist->data;
        MUTEX_LOCK(powerStateChangeJob);
        if ((powerStateChangeJob->jobState == LMI_PowerConcreteJob_JobState_Completed ||
             powerStateChangeJob->jobState == LMI_PowerConcreteJob_JobState_Killed ||
             powerStateChangeJob->jobState == LMI_PowerConcreteJob_JobState_Terminated) &&
            time(NULL) - powerStateChangeJob->timeOfLastChange > powerStateChangeJob->timeBeforeRemoval) {

            MUTEX_LOCK(power);
            power->jobs = g_list_remove_link(power->jobs, plist);
            MUTEX_UNLOCK(power);
            job_free(powerStateChangeJob);
        }
        MUTEX_UNLOCK(powerStateChangeJob);
        plist = g_list_next(plist);
    }
    return power->jobs;
}

unsigned short job_state(PowerStateChangeJob *state)
{
    return state->jobState;
}

int job_timeOfLastChange(PowerStateChangeJob *state)
{
    return state->timeOfLastChange;
}

int job_timeBeforeRemoval(PowerStateChangeJob *state)
{
    return state->timeBeforeRemoval;
}

size_t job_id(PowerStateChangeJob *state)
{
    return state->id;
}
