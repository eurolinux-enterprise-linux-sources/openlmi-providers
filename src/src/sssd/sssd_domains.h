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

#ifndef SSSD_DOMAINS_H_
#define SSSD_DOMAINS_H_

#include <sss_sifp.h>
#include <stdint.h>
#include "LMI_SSSDDomain.h"
#include "utils.h"

sssd_method_error
sssd_domain_set_instance(sss_sifp_ctx *sifp_ctx,
                         const char *path,
                         const CMPIBroker* cb,
                         const char *namespace,
                         LMI_SSSDDomain *_instance);

#endif /* SSSD_DOMAINS_H_ */
