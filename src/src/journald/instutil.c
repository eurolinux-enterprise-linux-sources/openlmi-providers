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

#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#include "instutil.h"
#include "journal.h"

#include "LMI_JournalLogRecord.h"
#include "LMI_JournalMessageLog.h"


/* Assuming no thread safety */
static sd_journal *ind_journal = NULL;

/* LMI_JournalMessageLog iterators */
static GHashTable *cmpi_iters = NULL;
G_LOCK_DEFINE_STATIC(cmpi_iters);
static sig_atomic_t cmpi_iters_count = 0;

#define JOURNAL_ITER_PREFIX "LMI_JournalMessageLog_CMPI_Iter_"
#define JOURNAL_ITER_SEPARATOR "#"
#define JOURNAL_ITER_EOF "<EOF>"


int create_LMI_JournalLogRecordRef(sd_journal *j,
                                   LMI_JournalLogRecordRef *ref,
                                   const CMPIBroker *_cb)
{
    char *cursor;
    uint64_t usec;
    CMPIDateTime *date;

    LMI_JournalLogRecordRef_Set_CreationClassName(ref, LMI_JournalLogRecord_ClassName);
    LMI_JournalLogRecordRef_Set_LogCreationClassName(ref, LMI_JournalMessageLog_ClassName);
    LMI_JournalLogRecordRef_Set_LogName(ref, JOURNAL_MESSAGE_LOG_NAME);

    /* Get stable cursor string */
    if (sd_journal_get_cursor(j, &cursor) < 0)
        return 0;
    LMI_JournalLogRecordRef_Set_RecordID(ref, cursor);
    free(cursor);

    /* Get timestamp */
    if (sd_journal_get_realtime_usec(j, &usec) < 0)
        return 0;
    date = CMNewDateTimeFromBinary(_cb, usec, 0, NULL);
    LMI_JournalLogRecordRef_Set_MessageTimestamp(ref, date);

    return 1;
}

static int dup_journal_data(
    sd_journal *j,
    const char *key,
    gchar **out)
{
    const char *d;
    size_t l;
    int r;

    *out = NULL;
    r = sd_journal_get_data(j, key, (const void **) &d, &l);
    if (r < 0)
        return r;
    if (l < strlen(key) + 1)
        return -EBADMSG;

    *out = g_strndup(d + strlen(key) + 1, l - strlen(key) - 1);
    if (*out == NULL)
        return -EBADMSG;

    return 0;
}

static int get_journal_data_int(
    sd_journal *j,
    const char *key,
    long int *out)
{
    int r;
    gchar *d;

    *out = -1;
    r = dup_journal_data(j, key, &d);
    if (r >= 0 && d != NULL && strlen(d) > 0) {
        char *conv_err = NULL;
        long int i = strtol(d, &conv_err, 10);
        if (conv_err == NULL || *conv_err == '\0')
            *out = i;
        g_free(d);
        return 0;
    }
    return -1;
}

static int get_record_message(sd_journal *j, gboolean full_format, gchar **out)
{
    int r;
    gchar *d;
    gchar *syslog_identifier = NULL;
    gchar *comm = NULL;
    gchar *pid = NULL;
    gchar *fake_pid = NULL;
    gchar *realtime = NULL;
    gchar *hostname = NULL;
    GString *str;

    str = g_string_new(NULL);
    if (! str)
        return -ENOMEM;

    /* Message format is inspired by journalctl short output, containing the identifier (process name),
     * PID and the message. In other words, it's the traditional syslog record format.
     * Keep in sync with systemd:src/shared/logs-show.c:output_short() */
    r = dup_journal_data(j, "MESSAGE", &d);
    if (r < 0)
        return r;

    dup_journal_data(j, "SYSLOG_IDENTIFIER", &syslog_identifier);
    dup_journal_data(j, "_COMM", &comm);
    dup_journal_data(j, "_PID", &pid);
    dup_journal_data(j, "SYSLOG_PID", &fake_pid);

    if (full_format) {
        /* Include timestamp and hostname */
        char buf[64];
        guint64 x = 0;
        time_t t;
        struct tm tm;
        gchar *endptr = NULL;

        dup_journal_data(j, "_SOURCE_REALTIME_TIMESTAMP", &realtime);
        dup_journal_data(j, "_HOSTNAME", &hostname);
        if (realtime) {
            g_ascii_strtoull(realtime, &endptr, 10);
            if (endptr != NULL && *endptr != '\0')
                x = 0;
        }
        if (x == 0)
            r = sd_journal_get_realtime_usec(j, &x);
        if (r >= 0) {
            t = (time_t) (x / 1000000ULL);
            r = strftime(buf, sizeof(buf), "%b %d %H:%M:%S", localtime_r(&t, &tm));
            if (r > 0)
                g_string_append_printf(str, "%s ", buf);
        }
        if (hostname)
            g_string_append_printf(str, "%s ", hostname);
    }
    if (syslog_identifier || comm)
        g_string_append(str, syslog_identifier ? syslog_identifier : comm);
    if (pid || fake_pid)
        g_string_append_printf(str, "[%s]", pid ? pid : fake_pid);
    if (str->len > 0)
        g_string_append(str, ": ");
    g_string_append(str, d);
    *out = g_string_free(str, FALSE);
    g_free(d);
    g_free(syslog_identifier);
    g_free(comm);
    g_free(pid);
    g_free(fake_pid);
    g_free(realtime);
    g_free(hostname);

    return 0;
}

int create_LMI_JournalLogRecord(sd_journal *j,
                                LMI_JournalLogRecord *rec,
                                const CMPIBroker *_cb)
{
    int r;
    uint64_t usec;
    CMPIDateTime *date;
    gchar *d;
    long int i;

    LMI_JournalLogRecord_Set_CreationClassName(rec, LMI_JournalLogRecord_ClassName);
    LMI_JournalLogRecord_Set_LogCreationClassName(rec, LMI_JournalMessageLog_ClassName);
    LMI_JournalLogRecord_Set_LogName(rec, JOURNAL_MESSAGE_LOG_NAME);

    /* Construct the message */
    r = get_record_message(j, FALSE, &d);
    if (r < 0)
        return r;
    LMI_JournalLogRecord_Set_DataFormat(rec, d);
    g_free(d);

    /* Set timestamp */
    r = sd_journal_get_realtime_usec(j, &usec);
    if (r < 0)
        return r;
    date = CMNewDateTimeFromBinary(_cb, usec, 0, NULL);
    LMI_JournalLogRecord_Set_MessageTimestamp(rec, date);

    /* Optional: hostname */
    r = dup_journal_data(j, "_HOSTNAME", &d);
    if (r >= 0 && d != NULL && strlen(d) > 0) {
        LMI_JournalLogRecord_Set_HostName(rec, d);
        g_free(d);
    }

    /* Optional: PerceivedSeverity */
    if (get_journal_data_int(j, "PRIORITY", &i) >= 0) {
        switch (i) {
            case LOG_EMERG:
                /* 7 - Fatal/NonRecoverable should be used to indicate an error occurred,
                 *     but it's too late to take remedial action. */
                LMI_JournalLogRecord_Set_PerceivedSeverity_Fatal_NonRecoverable(rec);
                break;
            case LOG_ALERT:
            case LOG_CRIT:
                /* 6 - Critical should be used to indicate action is needed NOW and the scope
                 *     is broad (perhaps an imminent outage to a critical resource will result). */
                LMI_JournalLogRecord_Set_PerceivedSeverity_Critical(rec);
                break;
            case LOG_ERR:
                /* 4 - Minor should be used to indicate action is needed, but the situation
                 *     is not serious at this time. */
                LMI_JournalLogRecord_Set_PerceivedSeverity_Minor(rec);
                break;
            case LOG_WARNING:
                /* 3 - Degraded/Warning should be used when its appropriate to let the user
                 *     decide if action is needed. */
                LMI_JournalLogRecord_Set_PerceivedSeverity_Degraded_Warning(rec);
                break;
            case LOG_NOTICE:
            case LOG_INFO:
            case LOG_DEBUG:
                /* 2 - Information */
                LMI_JournalLogRecord_Set_PerceivedSeverity_Information(rec);
                break;
        }
        if (i >= 0 && i <= LOG_DEBUG)
            LMI_JournalLogRecord_Set_SyslogSeverity(rec, i);
    }

    /* Optional: UID */
    if (get_journal_data_int(j, "_UID", &i) >= 0)
        LMI_JournalLogRecord_Set_UserID(rec, i);

    /* Optional: GID */
    if (get_journal_data_int(j, "_GID", &i) >= 0)
        LMI_JournalLogRecord_Set_GroupID(rec, i);

    /* Optional: PID */
    if (get_journal_data_int(j, "SYSLOG_PID", &i) >= 0 ||  get_journal_data_int(j, "_PID", &i) >= 0)
        LMI_JournalLogRecord_Set_ProcessID(rec, i);

    /* Optional: Syslog facility */
    if (get_journal_data_int(j, "SYSLOG_FACILITY", &i) >= 0 && i < LOG_NFACILITIES)
        LMI_JournalLogRecord_Set_SyslogFacility(rec, i);

    /* Optional: Syslog identifier */
    r = dup_journal_data(j, "SYSLOG_IDENTIFIER", &d);
    if (r >= 0 && d != NULL && strlen(d) > 0) {
        LMI_JournalLogRecord_Set_SyslogIdentifier(rec, d);
        g_free(d);
    }

    /* Optional: Systemd unit */
    r = dup_journal_data(j, "_SYSTEMD_UNIT", &d);
    if (r >= 0 && d != NULL && strlen(d) > 0) {
        LMI_JournalLogRecord_Set_SystemdUnit(rec, d);
        g_free(d);
    }

    return 1;
}

int match_journal_record(sd_journal *j, const char *message, const char *code_func)
{
    gchar *msg = NULL;
    gchar *pid = NULL;
    gchar *cfunc = NULL;
    char *conv_err = NULL;
    long int pid_n;
    int r;

    r = dup_journal_data(j, "MESSAGE", &msg);
    if (r < 0)
        return r;
    dup_journal_data(j, "_PID", &pid);
    dup_journal_data(j, "CODE_FUNC", &cfunc);

    if (pid)
        pid_n = strtol(pid, &conv_err, 10);

    r = msg && pid && cfunc &&
        (strcmp(message, msg) == 0) && (strcmp(code_func, cfunc) == 0) &&
        (conv_err == NULL || *conv_err == '\0') && (pid_n == getpid());

    g_free(msg);
    g_free(pid);
    g_free(cfunc);

    return r;
}

void ind_init()
{
    char errbuf[BUFLEN];

    if (ind_journal == NULL) {
        sd_journal *journal;
        int r;

        r = sd_journal_open(&journal, 0);
        if (r < 0) {
            lmi_error("ind_init(): Error opening journal: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
            return;
        }

        r = sd_journal_seek_tail(journal);
        if (r < 0) {
            lmi_error("ind_init(): Error seeking to the end of the journal: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
            sd_journal_close(journal);
            return;
        }

        /* need to position the marker one step before EOF or otherwise the next sd_journal_next() call will overflow to the beginning */
        r = sd_journal_previous(journal);
        if (r < 0) {
            lmi_error("ind_init(): Error seeking to the end of the journal: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
            sd_journal_close(journal);
            return;
        }
        ind_journal = journal;
    } else
        lmi_warn("ind_init(): indications already initialized, possible bug in the code\n");
}

void ind_destroy()
{
    if (ind_journal != NULL) {
        sd_journal_close(ind_journal);
        ind_journal = NULL;
    }
}

bool ind_watcher(void **data)
{
    int r;
    char errbuf[BUFLEN];

    if (ind_journal == NULL) {
        lmi_error("ind_watcher(): indications have not been initialized yet or error occurred previously\n");
        return false;
    }

    r = sd_journal_wait(ind_journal, (uint64_t) -1);
    if (r == SD_JOURNAL_INVALIDATE) {
        /* Looking at sd-journal sources, the sd_journal_wait() call will likely return
         * SD_JOURNAL_INVALIDATE on a first run because of creating new inotify watch. */
        r = sd_journal_wait(ind_journal, (uint64_t) -1);
    }
    while (r == SD_JOURNAL_NOP) {
        /* received NOP, ignore the event and wait for the next one */
        r = sd_journal_wait(ind_journal, (uint64_t) -1);
    }
    if (r < 0) {
        lmi_warn("ind_watcher(): Error while waiting for new record: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        return false;
    }
    if (r == SD_JOURNAL_INVALIDATE) {
        lmi_warn("ind_watcher(): Journal not valid, reopen needed\n");
        ind_destroy();
        ind_init();
        return false;
    }
    *data = ind_journal;

    return true;
}

bool ind_gather(const IMManager *manager, CMPIInstance **old, CMPIInstance **new, void *data)
{
    sd_journal *journal;
    int r;
    LMI_JournalLogRecord log_record;
    CMPIStatus st;
    char errbuf[BUFLEN];

    g_return_val_if_fail(data != NULL, false);
    journal = data;

    r = sd_journal_next(journal);
    if (r < 0) {
        lmi_error("ind_gather(): Failed to iterate to next entry: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        return false;
    }
    if (r == 0) {
        /* We've reached the end of the journal */
        return false;
    }

    /* FIXME: hardcoded namespace (so does ind_manager.c) */
    LMI_JournalLogRecord_Init(&log_record, manager->broker, "root/cimv2");
    r = create_LMI_JournalLogRecord(journal, &log_record, manager->broker);
    if (r <= 0) {
        lmi_error("ind_gather(): Failed to create instance: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        return false;
    }

    g_assert(new != NULL);
    *new = LMI_JournalLogRecord_ToInstance(&log_record, &st);
    lmi_debug(" ind_gather(): new instance created\n");

    return true;
}

/* --------------------------------------------------------------------------- */

/* TODO: count references to the journal struct -- someone may cancel the iteration
 *       while others are still using the journal struct (racy)  */

#define set_cmpi_status_fmt(CB, STATUS, CODE, MSG, ...) \
           { gchar *errs; \
             errs = g_strdup_printf(MSG, ##__VA_ARGS__); \
             KSetStatus2(CB, STATUS, CODE, errs); \
             g_free(errs);\
           }

static gchar *
make_iterator_string(sd_journal *journal, const char *cursor, gchar *iter_id_short, gboolean eof_set)
{
    return g_strdup_printf("%s%s%p%s%s%s%s", iter_id_short, JOURNAL_ITER_SEPARATOR, (void *)journal, JOURNAL_ITER_SEPARATOR, cursor,
                                             eof_set ? JOURNAL_ITER_SEPARATOR : "", eof_set ? JOURNAL_ITER_EOF : "");
}

bool
journal_iter_parse_iterator_string(const char *iter_id, gchar **out_iter_id_short, gpointer *out_iter_ptr, gchar **out_iter_cursor, gboolean *out_eof_set)
{
    gchar **s = NULL;
    bool res;
    gpointer valid_p;

    if (out_eof_set)
        *out_eof_set = FALSE;

    res = (iter_id && strlen(iter_id) > 0);
    if (res)
        s = g_strsplit(iter_id, JOURNAL_ITER_SEPARATOR, 4);
    res = res && s && (g_strv_length(s) == 3 || g_strv_length(s) == 4) && strlen(s[0]) > 0 && strlen(s[1]) > 0 && strlen(s[2]) > 0 && (g_strv_length(s) == 3 || strlen(s[3]) > 0);
    valid_p = NULL;
    res = res && (sscanf(s[1], "%p", &valid_p) == 1);
    if (res && out_iter_id_short)
        res = res && ((*out_iter_id_short = g_strdup(s[0])) != NULL);
    if (res && out_iter_ptr)
        *out_iter_ptr = valid_p;
    if (res && out_iter_cursor)
        res = res && ((*out_iter_cursor = g_strdup(s[2])) != NULL);
    if (res && out_eof_set && g_strv_length(s) == 4) {
        *out_eof_set = g_strcmp0(s[3], JOURNAL_ITER_EOF) == 0;
        /* the fourth part should only be present with valid string */
        res = res && *out_eof_set;
    }
    g_strfreev(s);

    return res;
}

gchar *
journal_iter_new(const gchar *req_cursor, gboolean seek_tail, sd_journal **journal_out)
{
    gchar *iter_id = NULL;
    gchar *iter_id_full = NULL;
    char *cursor;
    sd_journal *journal;
    int r;
    char errbuf[BUFLEN];
    gboolean eof_set;

    if (journal_out)
        *journal_out = NULL;

    r = sd_journal_open(&journal, 0);
    if (r < 0) {
        lmi_error("Error opening journal: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        return NULL;
    }

    if (req_cursor)
        r = sd_journal_seek_cursor(journal, req_cursor);
    else {
        if (seek_tail)
            r = sd_journal_seek_tail(journal);
        else
            r = sd_journal_seek_head(journal);
    }

    if (r < 0) {
        lmi_error("Error seeking to the requested journal position: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        sd_journal_close(journal);
        return NULL;
    }

    if (seek_tail)
        r = sd_journal_previous(journal);
    else
        r = sd_journal_next(journal);
    if (r < 0) {
        lmi_error("Error stepping next in the journal: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        sd_journal_close(journal);
        return NULL;
    }
    eof_set = r == 0;

    r = sd_journal_get_cursor(journal, &cursor);
    if (r < 0) {
        lmi_error("Error getting current cursor: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        sd_journal_close(journal);
        return NULL;
    }

    G_LOCK(cmpi_iters);
    if (cmpi_iters == NULL)
        cmpi_iters = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) sd_journal_close);
    if (cmpi_iters == NULL) {
        lmi_error("Memory allocation failure\n");
        sd_journal_close(journal);
        G_UNLOCK(cmpi_iters);
        return NULL;
    }
    iter_id = g_strdup_printf("%s%d", JOURNAL_ITER_PREFIX, cmpi_iters_count++);
    if (iter_id)
        iter_id_full = make_iterator_string(journal, cursor, iter_id, eof_set);
    if (iter_id == NULL || iter_id_full == NULL) {
        lmi_error("Memory allocation failure\n");
        sd_journal_close(journal);
    } else {
        g_hash_table_insert(cmpi_iters, iter_id, journal);
    }
    G_UNLOCK(cmpi_iters);

    if (iter_id_full && journal_out)
        *journal_out = journal;

    return iter_id_full;
}

bool
journal_iter_validate_id(gchar **iter_id, sd_journal **journal_out, gchar **prefix_out, const CMPIBroker *_cb, CMPIStatus *status)
{
    gboolean res;
    gchar *iter_id_short, *iter_cursor;
    gpointer iter_ptr;
    gboolean eof_set;

    res = TRUE;
    if (journal_out)
        *journal_out = NULL;
    if (prefix_out)
        *prefix_out = NULL;

    if (! journal_iter_parse_iterator_string(*iter_id, &iter_id_short, &iter_ptr, &iter_cursor, &eof_set)) {
        set_cmpi_status_fmt(_cb, status, ERR_INVALID_PARAMETER, "Malformed IterationIdentifier argument: \'%s\'\n", *iter_id);
        return false;
    }
    KSetStatus(status, OK);

    if (journal_out) {
        G_LOCK(cmpi_iters);
        if (cmpi_iters)
            *journal_out = g_hash_table_lookup(cmpi_iters, iter_id_short);
        G_UNLOCK(cmpi_iters);
        if (*journal_out == NULL || *journal_out != iter_ptr) {
            /* Assume stale iterator ID, reopen journal and try to find the position by the cursor */
            lmi_warn("journal_iter_validate_id(): iterator pointer %p doesn't match with hashtable %p, reopening journal...\n", iter_ptr, *journal_out);
            g_free(*iter_id);
            *iter_id = journal_iter_new(iter_cursor, FALSE, journal_out);
            if (*iter_id == NULL) {
                lmi_error("The IterationIdentifier is not valid anymore: \'%s\'\n", *iter_id);
                res = FALSE;
            }
        }
    }

    if (res && prefix_out) {
        /* No need to check prefix validity as it's supposed to be used in journal_iter_cancel() only */
        res = res && ((*prefix_out = g_strdup(iter_id_short)) != NULL);
    }

    g_free(iter_id_short);
    g_free(iter_cursor);
    return res;
}


bool
journal_iter_cancel(const char *iter_id)
{
    gboolean b;

    g_return_val_if_fail(iter_id != NULL, false);

    G_LOCK(cmpi_iters);
    b = cmpi_iters && g_hash_table_remove(cmpi_iters, iter_id);
    G_UNLOCK(cmpi_iters);
    if (! b) {
        lmi_error("IterationIdentifier \'%s\' not registered\n", iter_id);
        return false;
    }

    return true;
}

static bool
update_iter(gchar **iter_id, gboolean explicit_eof, sd_journal *journal)
{
    gchar *iter_id_short;
    char *cursor;
    int r;
    char errbuf[BUFLEN];
    gboolean eof_set;

    r = sd_journal_get_cursor(journal, &cursor);
    if (r < 0) {
        lmi_error("Error getting current cursor: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        return false;
    }

    if (! journal_iter_parse_iterator_string(*iter_id, &iter_id_short, NULL, NULL, &eof_set))
        return false;
    *iter_id = make_iterator_string(journal, cursor, iter_id_short, explicit_eof);
    return *iter_id != NULL;
}


bool
journal_iter_seek(gchar **iter_id, sd_journal *journal, gint64 position)
{
    int r;
    char errbuf[BUFLEN];

    g_return_val_if_fail(journal != NULL, false);

    if (position == 0) {  /* NOP */
        lmi_warn("journal_iter_seek(): Spurious seek request to relative position 0\n");
        return true;
    }

    if (position > 0)
        r = sd_journal_next_skip(journal, position);
    else
        r = sd_journal_previous_skip(journal, -position);

    if (r < 0) {
        lmi_error("Error seeking to the requested position: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        return false;
    }

    if (! update_iter(iter_id, r == 0, journal)) {
        lmi_error("Error seeking to the requested position\n");
        return false;
    }

    return true;
}

gchar *
journal_iter_get_data(gchar **iter_id, sd_journal *journal, gboolean step_next)
{
    gchar *d;
    int r;
    char errbuf[BUFLEN];
    gboolean eof_set;

    g_return_val_if_fail(journal != NULL, false);

    /* In case of EOF was reached previously, try to seek again to see if new record arrived */
    if (! journal_iter_parse_iterator_string(*iter_id, FALSE, NULL, NULL, &eof_set))
        return false;
    if (eof_set) {
        r = sd_journal_next(journal);
        if (r <= 0)
            return false;
    }

    /* Construct the message */
    r = get_record_message(journal, TRUE, &d);
    if (r < 0) {
        lmi_error("Error getting record message: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        return NULL;
    }

    eof_set = FALSE;
    if (step_next) {
        r = sd_journal_next(journal);
        if (r < 0) {
            lmi_error("Error advancing to the next record: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
            g_free(d);
            return NULL;
        }
        eof_set = (r == 0);
    }

    if (! update_iter(iter_id, eof_set, journal)) {
        lmi_error("Error getting record message\n");
        return NULL;
    }

    return d;
}

/* FIXME: unused for the moment as the hash table is global, shared across instances */
void
journal_iters_destroy()
{
    G_LOCK(cmpi_iters);
    if (cmpi_iters != NULL) {
        g_hash_table_destroy(cmpi_iters);
        cmpi_iters = NULL;
    }
    G_UNLOCK(cmpi_iters);
}
