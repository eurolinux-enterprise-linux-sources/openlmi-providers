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

#ifndef POWER_H
#define POWER_H

#include "openlmi.h"

const char *provider_name;
const ConfigEntry *provider_config_defaults;

typedef struct _Power Power;
typedef struct _PowerStateChangeJob PowerStateChangeJob;

/**
 * Get reference to global power object
 * \note Don't forget to call power_unref
 */
Power *power_ref(const CMPIBroker *_cb, const CMPIContext *ctx);

/**
 * Decrement reference counter for power struct
 */
void power_unref(Power *power);

/**
 * Get list of available requested power states
 *
 * \param power Pointer to power struct, obtained by power_ref
 * \param count Number of states returned
 * \return list of power states
 */
unsigned short *power_available_requested_power_states(Power *power, int *count);

/**
 * Get requested power state
 *
 * \param power Pointer to power struct, obtained by power_ref
 * \return current requested power state
 */
unsigned short power_requested_power_state(Power *power);

/**
 * Request change power state to \p state
 *
 * \param power Pointer to power struct, obtained by power_ref
 * \param state Requested power state
 * \return CMPI return code
 */
int power_request_power_state(Power *power, unsigned short state);

GList *power_get_jobs(Power *power);

unsigned short power_transitioning_to_power_state(Power *power);


unsigned short job_state(PowerStateChangeJob *state);
int job_timeOfLastChange(PowerStateChangeJob *state);
int job_timeBeforeRemoval(PowerStateChangeJob *state);
size_t job_id(PowerStateChangeJob *state);

#endif // POWER_H
