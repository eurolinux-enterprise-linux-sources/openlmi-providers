/*
 * Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
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

#ifndef INSTUTIL_H_
#define INSTUTIL_H_

#include <glib.h>
#include <konkret/konkret.h>
#include <systemd/sd-journal.h>

#include <ind_manager.h>
#include "LMI_JournalLogRecord.h"

int create_LMI_JournalLogRecordRef(sd_journal *j, LMI_JournalLogRecordRef *ref, const CMPIBroker *_cb);
int create_LMI_JournalLogRecord(sd_journal *j, LMI_JournalLogRecord *rec, const CMPIBroker *_cb);

int match_journal_record(sd_journal *j, const char *message, const char *code_func);

void ind_init();
bool ind_watcher(void **data);
bool ind_filter_cb(const CMPISelectExp *filter);
bool ind_gather(const IMManager *manager, CMPIInstance **old, CMPIInstance **new, void *data);
void ind_destroy();

gchar * journal_iter_new(const gchar *req_cursor, sd_journal **journal_out);
bool    journal_iter_parse_iterator_string(const char *iter_id, gchar **out_iter_id_short, gpointer *out_iter_ptr, gchar **out_iter_cursor);
bool    journal_iter_validate_id(gchar **iter_id, sd_journal **journal_out, gchar **prefix_out, const CMPIBroker *_cb, CMPIStatus *status);
bool    journal_iter_cancel(const gchar *iter_id);
bool    journal_iter_seek(gchar **iter_id, sd_journal *journal, gint64 position);
gchar * journal_iter_get_data(gchar **iter_id, sd_journal *journal, gboolean step_next);
void    journal_iters_destroy();


#endif /* INSTUTIL_H_ */
