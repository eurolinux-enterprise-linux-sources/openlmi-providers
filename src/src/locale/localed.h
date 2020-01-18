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
 * Authors: Vitezslav Crhonek <vcrhonek@redhat.com>
 */


#ifndef LOCALED_H
#define LOCALED_H

#include "openlmi.h"
#include "LMI_Locale.h"

#define KNOWN_LOC_STR_CNT 13

const char *provider_name;
const ConfigEntry *provider_config_defaults;

struct _CimLocale {
    char *Locale[KNOWN_LOC_STR_CNT];
        // The following strings are known: LANG=, LC_CTYPE=, LC_NUMERIC=, LC_TIME=,
        // LC_COLLATE=, LC_MONETARY=, LC_MESSAGES=, LC_PAPER=, LC_NAME=, LC_ADDRESS=,
        // LC_TELEPHONE=, LC_MEASUREMENT=, LC_IDENTIFICATION=.
    int LocaleCnt;
    char *VConsoleKeymap;
    char *VConsoleKeymapToggle;
    char *X11Layouts;
    char *X11Model;
    char *X11Variant;
    char *X11Options;
};

typedef struct _CimLocale CimLocale;

void locale_init();

CimLocale *locale_get_properties();
void locale_free(CimLocale *cloc);

int set_locale(char *locale[], int locale_cnt);
int set_v_console_keyboard(const char *keymap, const char* keymap_toggle, int convert);
int set_x11_keyboard(const char *layouts, const char *model, const char *variant, const char *options, int convert);

#endif
