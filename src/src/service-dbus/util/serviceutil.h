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
 * Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
 */


#ifndef SERVICEUTIL_H
#define SERVICEUTIL_H

#include <stdio.h>
#include <gio/gio.h>
#include <pthread.h>
#include "openlmi.h"
#include "LMI_Service.h"

/* systemd job result string */
#define JR_DONE "done"

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

struct _AllServices {
  Service **svc;
  int cnt;
  int nalloc;
};

typedef struct _AllServices AllServices;

void service_free_slist(SList *slist);
void service_free_all_services(AllServices *svcs);
SList *service_find_all(char *output, int output_len);

AllServices *service_get_properties_all(char *output, int output_len);
int service_get_properties(Service *svc, const char *service, char *output, int output_len);

unsigned int service_operation(const char *service, const char *method, char *output, int output_len);

/* Indications */

struct _ServiceIndication {
  SList *slist;
  GDBusProxy *manager_proxy;
  GDBusProxy **signal_proxy;
  GMainContext *context;
  GMainLoop *loop;
  pthread_t p;
};

typedef struct _ServiceIndication ServiceIndication;

int ind_init(ServiceIndication *si, char *output, int output_len);
bool ind_watcher(void **data);
void ind_destroy(ServiceIndication *si);

#endif
