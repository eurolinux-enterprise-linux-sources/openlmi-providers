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
 * Authors: Roman Rakus <rrakus@redhat.com>
 */

#include "aux_lu.h"
#include <libuser/entity.h>
#include <libuser/user.h>
#include <utmp.h>
#include <string.h>

/*
 * Get the value of type from the given entity
 * returns string
 */
const char* aux_lu_get_str(struct lu_ent* ent, char* type)
{
  return g_value_get_string(g_value_array_get_nth(lu_ent_get(ent, type), 0));
}

/*
 * Get the value of type from the given entity
 * returns long int
 */
long aux_lu_get_long(struct lu_ent* ent, char* type)
{
  return g_value_get_long(g_value_array_get_nth(lu_ent_get(ent, type), 0));
}

/*
 * Get the latest login time for given user name
 * return value greater than 0 on success
 * return 0 when not found
 * return -1 on error and errno properly set
 */
time_t aux_utmp_latest(const char* name)
{
  time_t last = 0, check = 0;
  unsigned int found = 0;
  struct utmp *rec = NULL;
  if (utmpname(WTMP_FILE))
    {
      return -1;
    }
  setutent();
  while ((rec = getutent()) != NULL)
    {
      if (rec->ut_type == USER_PROCESS && strcmp(rec->ut_user, name) == 0)
        {
          found = 1;
          check = rec->ut_tv.tv_sec;
          last = last > check ? last : check;
        }
    }
  endutent();
  return found ? last : -1;
}

