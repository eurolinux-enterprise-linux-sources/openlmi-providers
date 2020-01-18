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
 *          Radek Novacek <rnovacek@redhat.com>
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "serviceutil.h"

#define MAX_SLIST_CNT 1000

char *suscript = "/usr/libexec/serviceutil.sh";
char *sdscript = "/usr/libexec/servicedisc.sh";

const char *provider_name = "service";
const ConfigEntry *provider_config_defaults = NULL;

typedef struct {
  FILE *fp;
  FILE *fp2;
} Control;

void
Service_Free_SList(SList *slist)
{
  int i;

  if (slist == NULL)
    return;

  for(i = 0; i < slist->cnt; i++)
    free(slist->name[i]);
  free(slist->name);
  free(slist);

  return;
}

SList *
Service_Find_All(void)
{
  char svname[BUFLEN];
  Control *cc = malloc(sizeof(Control));
  SList *slist;

  if (cc)
  {
    cc->fp = popen(sdscript, "r");
    if (cc->fp)
    {
      slist = malloc(sizeof(SList));
      slist->name = malloc(MAX_SLIST_CNT * sizeof(char *));
      slist->cnt = 0;
      while (fgets(svname, sizeof(svname), cc->fp) != NULL)
      {
        slist->name[slist->cnt] = strndup(svname, strlen(svname) - 1);
        slist->cnt++;
      }
      pclose(cc->fp);
      free(cc);
      return slist;
    }
    else
    {
      free(cc);
      cc=NULL;
      return NULL;
    }
  }
  else
  {
    free(cc);
    return NULL;
  }
}

void *
Service_Begin_Enum(const char *service)
{
  char cmdbuffer[BUFLEN];
  Control *cc = malloc(sizeof(Control));

  memset(&cmdbuffer, '\0', sizeof(cmdbuffer));

  if (cc)
  {
    snprintf(cmdbuffer, BUFLEN, "%s status %s", suscript, service);
    cc->fp = popen(cmdbuffer, "r");
    if (cc->fp)
    {
      snprintf(cmdbuffer, BUFLEN, "%s is-enabled %s", suscript, service);
      cc->fp2 = popen(cmdbuffer, "r");
      if (!cc->fp2)
      {
        pclose(cc->fp);
        free(cc);
        cc=NULL;
      }
    }
    else
    {
      free(cc);
      cc=NULL;
    }
  }

  return cc;
}

int
Service_Next_Enum(void *handle, Service* svc, const char *service)
{
  char result[BUFLEN];
  char svname[BUFLEN];
  int pid = 0;
  Control *cc = (Control *) handle;
  int state = 0, ret = 0;

  memset(&result, '\0', sizeof(result));
  memset(&svname, '\0', sizeof(svname));

  if (cc && svc)
  {
    svc->svEnabledDefault = 5;
    while (fgets(result, sizeof(result), cc->fp) != NULL)
    {
      if (strncmp(result, "stopped", 7) == 0)
      {
        svc->pid = 0;
        ret = 1;
      }
      else
      {
        state = sscanf(result,"%d %255s", &pid, svname);
        svc->pid = pid;
        if (state) ret = 1;
      }
    }
    svc->svName = strdup(service);

    while (fgets(result, sizeof(result), cc->fp2) != NULL)
    {
      if (strncmp(result, "enabled", 7) == 0)
        svc->svEnabledDefault = 2;
      if (strncmp(result, "disabled", 8) == 0)
        svc->svEnabledDefault = 3;
    }
  }

  if (svc)
  {
    if (svc->pid)
    {
      svc->svStarted = 1;
      svc->svStatus = strdup("OK");
    }
    else
    {
      svc->svStarted = 0;
      svc->svStatus = strdup("Stopped");
    }
  }

  return ret;
}

void
Service_End_Enum(void *handle)
{
  Control *cc = (Control *) handle;

  if (cc) {
    pclose(cc->fp);
    pclose(cc->fp2);
    free(cc);
  }
}

unsigned int
Service_Operation(const char *service, const char *method, char *result, int resultlen)
{
  int res = 0;
  char cmdbuffer[BUFLEN];
  const char *const proc_path = "/proc/self/fd/";
  char template[] = "/tmp/Service_OperationXXXXXX";

  int tfd = mkstemp(template);
  if (tfd == -1) {
    return -1;
  }
  unlink(template);

  const int cmd_size = snprintf(NULL, 0, "%s%ul", proc_path, tfd);
  char cmd[cmd_size];
  snprintf(cmd, cmd_size, "%s%ul", proc_path, tfd);

  snprintf(cmdbuffer, BUFLEN, "%s %s %s > %s", suscript, method, service, cmd);

  res = system(cmdbuffer);

  /* we got some output? */
  read(tfd, result, resultlen);
  close(tfd);

  return WEXITSTATUS(res);
}
