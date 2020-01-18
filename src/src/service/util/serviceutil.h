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

#define ARRAY_SIZE(name) (sizeof(name) / sizeof(name[0]))

const char *provider_name;
const ConfigEntry *provider_config_defaults;

enum ServiceEnabledDefault { ENABLED = 2, DISABLED = 3, NOT_APPICABLE = 5};

struct _Service {
  char *svSystemCCname;
  char *svSystemname;
  char *svCCname;
  char *svName; /* "rsyslog", "httpd", ... */
  char *svStatus; /* "Stopped", "OK" */
  enum ServiceEnabledDefault svEnabledDefault;
  int svStarted; /* 0, 1 */
  int pid; /* PID */
};

struct _SList {
  char **name;
  int cnt;
};

typedef struct _Service Service;
typedef struct _SList SList;

void Service_Free_SList(SList *slist);
SList *Service_Find_All(void);

void *Service_Begin_Enum(const char *service);
int Service_Next_Enum(void *handle, Service* svc, const char *service);
void Service_End_Enum(void *handle);

unsigned int Service_Operation(const char *service, const char *method, char *result, int resultlen);

#endif
