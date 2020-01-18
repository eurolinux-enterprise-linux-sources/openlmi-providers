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

#ifndef SSSD_COMPONENTS_H_
#define SSSD_COMPONENTS_H_

#include <sss_sifp.h>
#include <stdint.h>
#include "LMI_SSSDMonitor.h"
#include "LMI_SSSDResponder.h"
#include "LMI_SSSDBackend.h"
#include "utils.h"

typedef enum sssd_component_type {
    SSSD_COMPONENT_MONITOR = 0,
    SSSD_COMPONENT_RESPONDER,
    SSSD_COMPONENT_BACKEND
} sssd_component_type;

KUint32
sssd_component_set_debug_permanently(const char *name,
                                     sssd_component_type type,
                                     const KUint16* debug_level,
                                     CMPIStatus* _status);

KUint32
sssd_component_set_debug_temporarily(const char *name,
                                     sssd_component_type type,
                                     const KUint16* debug_level,
                                     CMPIStatus* _status);

KUint32 sssd_component_enable(const char *name,
                              sssd_component_type type,
                              CMPIStatus* _status);

KUint32 sssd_component_disable(const char *name,
                               sssd_component_type type,
                               CMPIStatus* _status);

sssd_method_error
sssd_monitor_set_instance(sss_sifp_ctx *sifp_ctx,
                          const char *path,
                          const CMPIBroker* cb,
                          const char *namespace,
                          LMI_SSSDMonitor *instance);

sssd_method_error
sssd_responder_set_instance(sss_sifp_ctx *sifp_ctx,
                            const char *path,
                            const CMPIBroker* cb,
                            const char *namespace,
                            LMI_SSSDResponder *instance);

sssd_method_error
sssd_backend_set_instance(sss_sifp_ctx *sifp_ctx,
                          const char *path,
                          const CMPIBroker* cb,
                          const char *namespace,
                          LMI_SSSDBackend *instance);

#endif /* SSSD_COMPONENTS_H_ */
