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
 * Authors: Pavel Březina <pbrezina@redhat.com>
 */

#ifndef LMI_SSSD_UTILS_H_
#define LMI_SSSD_UTILS_H_

#include <dhash.h>
#include "openlmi.h"
#include "LMI_SSSDService.h"
#include "LMI_SSSDMonitor.h"
#include "LMI_SSSDResponder.h"
#include "LMI_SSSDDomain.h"

#define PROVIDER_NAME "sssd"
#define SERVICE_NAME "OpenLMI SSSD Service"

/* SSSD D-Bus methods */
#define SSSD_DBUS_LIST_COMPONENTS "Components"
#define SSSD_DBUS_LIST_RESPONDERS "Responders"
#define SSSD_DBUS_LIST_BACKENDS "Backends"
#define SSSD_DBUS_LIST_DOMAINS "Domains"
#define SSSD_DBUS_FIND_MONITOR "Monitor"
#define SSSD_DBUS_FIND_RESPONDER "ResponderByName"
#define SSSD_DBUS_FIND_BACKEND "BackendByName"
#define SSSD_DBUS_FIND_DOMAIN "DomainByName"

typedef enum {
    SSSD_METHOD_ERROR_OK,
    SSSD_METHOD_ERROR_FAILED,
    SSSD_METHOD_ERROR_NOT_SUPPORTED,
    SSSD_METHOD_ERROR_IO
} sssd_method_error;

#define check_sss_sifp_error(error, ret, val, label) do { \
if (error != SSS_SIFP_OK) { \
    ret = val; \
    goto label; \
} \
} while (0)

void LMI_SSSDService_Get_Ref(const CMPIContext *cc,
                             const CMPIBroker *cb,
                             const char *namespace,
                             LMI_SSSDServiceRef *ref);

void hash_delete_cb(hash_entry_t *item,
                    hash_destroy_enum type,
                    void *pvt);

#endif /* LMI_SSSD_UTILS_H_ */
