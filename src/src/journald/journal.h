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
 * Authors: Tomas Bzatek <tbzatek@redhat.com>
 */

#ifndef JOURNAL_H_
#define JOURNAL_H_

#include <openlmi.h>

#define JOURNAL_MESSAGE_LOG_NAME "Journal"
#define JOURNAL_CIM_LOG_NAME "openlmi-journald"   /* prefix used in debug messages */


/* Limit the number of LMI_JournalLogRecord instances enumerated */

/* Typical system carries hundreds of thousands of records making stress
 * on the CIMOM itself and tools processing the XML reply. Accidental
 * instance enumeration can lead to denial-of-service in extreme cases,
 * so let's limit the total number of instances for now. */
#define JOURNAL_MAX_INSTANCES_NUM 1000    /* Set to 0 to disable this feature */


/* Logging setup */
const ConfigEntry *provider_config_defaults;

#endif /* JOURNAL_H_ */
