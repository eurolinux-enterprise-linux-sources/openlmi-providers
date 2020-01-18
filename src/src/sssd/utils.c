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

#include <stdlib.h>
#include "openlmi.h"
#include "utils.h"

void LMI_SSSDService_Get_Ref(const CMPIContext *cc,
                             const CMPIBroker *cb,
                             const char *namespace,
                             LMI_SSSDServiceRef *ref)
{
    const char *hostname = lmi_get_system_name_safe(cc);
    const char *sysclass = lmi_get_system_creation_class_name();

    LMI_SSSDServiceRef_Init(ref, cb, namespace);
    LMI_SSSDServiceRef_Set_Name(ref, SERVICE_NAME);
    LMI_SSSDServiceRef_Set_SystemName(ref, hostname);
    LMI_SSSDServiceRef_Set_CreationClassName(ref, LMI_SSSDService_ClassName);
    LMI_SSSDServiceRef_Set_SystemCreationClassName(ref, sysclass);
}

void hash_delete_cb(hash_entry_t *item,
                    hash_destroy_enum type,
                    void *pvt)
{
    free(item->value.ptr);
}
