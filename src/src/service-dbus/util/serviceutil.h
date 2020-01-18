/*
 * Copyright (C) 2012-2013 Red Hat, Inc.  All rights reserved.
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
 * Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
 */


#ifndef SERVICEUTIL_H
#define SERVICEUTIL_H

#include <stdio.h>
#include "openlmi.h"

#define ARRAY_SIZE(name) (sizeof(name) / sizeof(name[0]))

const char *provider_name;
const ConfigEntry *provider_config_defaults;

enum OperationalStatus {OS_UNKNOWN = 0, OS_OK = 2, OS_ERROR = 6, OS_STARTING = 8, OS_STOPPING = 9, OS_STOPPED = 10, OS_COMPLETED = 17};
enum ServiceEnabledDefault {ENABLED = 2, DISABLED = 3, NOT_APPLICABLE = 5};

struct _Service {
  char *svSystemCCname;
  char *svSystemname;
  char *svCCname;
  char *svName; /* "rsyslog.service", "httpd.service", ... */
  char *svCaption; /* "Description" field from unit file */
  enum OperationalStatus svOperationalStatus[2]; /* see enum definition - current status of the element */
  int svOperationalStatusCnt;
  char *svStatus; /* "Stopped", "OK" - deprecated, but recommended to fill */
  enum ServiceEnabledDefault svEnabledDefault;
  int svStarted; /* 0, 1 */
};

struct _SList {
  char **name;
  int cnt;
  int nalloc;
};

typedef struct _Service Service;
typedef struct _SList SList;

void service_free_slist(SList *slist);
SList *service_find_all(char *output, int output_len);

int service_get_properties(Service *svc, const char *service, char *output, int output_len);

unsigned int service_operation(const char *service, const char *method, char *output, int output_len);

#endif
