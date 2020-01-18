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

#include <konkret/konkret.h>
#include <cmpi/cmpimacs.h>
#include "LMI_JournalLogRecord.h"
#include "LMI_JournalMessageLog.h"

#include <errno.h>
#include <glib.h>
#include <systemd/sd-journal.h>


#include "journal.h"
#include "instutil.h"


#define WRITE_RECORD_TIMEOUT 5000 /* ms */


static const CMPIBroker* _cb = NULL;
static sd_journal *journal_iter = NULL;

static void LMI_JournalLogRecordInitialize(const CMPIContext *ctx)
{
    sd_journal *journal;
    int r;
    char errbuf[BUFLEN];

    lmi_init(JOURNAL_CIM_LOG_NAME, _cb, ctx, provider_config_defaults);

    r = sd_journal_open(&journal, 0);
    if (r < 0) {
        lmi_error("Error opening journal: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
        return;
    }
    journal_iter = journal;
}

static CMPIStatus LMI_JournalLogRecordCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    if (journal_iter != NULL) {
        sd_journal_close(journal_iter);
        journal_iter = NULL;
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalLogRecordEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    const char *ns = KNameSpace(cop);
    sd_journal *journal;
    int r;
    LMI_JournalLogRecordRef log_record_ref;
    unsigned long count = 0;
    char errbuf[BUFLEN];

    /* Open our own journal instance to prevent losing cursor position in the global instance */
    r = sd_journal_open(&journal, 0);
    if (r < 0)
        KReturn2(_cb, ERR_FAILED, "Error opening journal: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));

    r = sd_journal_seek_tail(journal);
    if (r < 0) {
        sd_journal_close(journal);
        KReturn2(_cb, ERR_NOT_FOUND, "Failed to seek to the end of the journal: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
    }

    SD_JOURNAL_FOREACH_BACKWARDS(journal) {
        LMI_JournalLogRecordRef_Init(&log_record_ref, _cb, ns);
        if (!create_LMI_JournalLogRecordRef(journal, &log_record_ref, _cb))
            continue;

        /* FYI: not testing ability to actually retrieve data, assuming further failures in subsequent calls */

        KReturnObjectPath(cr, log_record_ref);

        /* Instance number limiter */
        count++;
        if (JOURNAL_MAX_INSTANCES_NUM > 0 && count >= JOURNAL_MAX_INSTANCES_NUM)
            break;
    }
    sd_journal_close(journal);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalLogRecordEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus st;
    CMPIEnumeration *e;
    CMPIData cd;
    CMPIInstance *in;

    if (!(e = _cb->bft->enumerateInstanceNames(_cb, cc, cop, &st)))
        KReturn2(_cb, ERR_FAILED, "Unable to enumerate instances of LMI_JournalLogRecord");

    while (CMHasNext(e, &st)) {
        cd = CMGetNext(e, &st);
        if (st.rc || cd.type != CMPI_ref)
            KReturn2(_cb, ERR_FAILED, "Enumerate instances didn't returned list of references");
        in = _cb->bft->getInstance(_cb, cc, cd.value.ref, properties, &st);
        if (st.rc)
            KReturn2(_cb, ERR_FAILED, "Unable to get instance of LMI_JournalLogRecord");
        cr->ft->returnInstance(cr, in);
    }
    KReturn(OK);
}

static CMPIStatus LMI_JournalLogRecordGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_JournalLogRecord log_record;
    int r;
    char errbuf[BUFLEN];

    LMI_JournalLogRecord_InitFromObjectPath(&log_record, _cb, cop);

    if (journal_iter == NULL)
        KReturn2(_cb, ERR_FAILED, "Journal not open, possible failure during initialization\n");

    r = sd_journal_seek_cursor(journal_iter, log_record.RecordID.chars);
    if (r < 0)
        KReturn2(_cb, ERR_NOT_FOUND, "Failed to seek to the requested position: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));

    r = sd_journal_next(journal_iter);
    if (r < 0)
        KReturn2(_cb, ERR_FAILED, "Failed to seek next to the cursor: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));

    r = create_LMI_JournalLogRecord(journal_iter, &log_record, _cb);
    if (r <= 0)
        KReturn2(_cb, ERR_FAILED, "Failed to create instance: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));

    KReturnInstance(cr, log_record);
    KReturn(OK);
}

static const char *
get_string_property(const char *property_name, const CMPIInstance* ci)
{
    CMPIData d;
    CMPIStatus rc = { CMPI_RC_OK, NULL };

    d = CMGetProperty(ci, property_name, &rc);
    if (rc.rc != CMPI_RC_OK || (d.state & CMPI_nullValue) == CMPI_nullValue ||
        (d.state & CMPI_notFound) == CMPI_notFound || d.value.string == NULL)
        return NULL;
    return CMGetCharPtr(d.value.string);
}

#define J_RESULT_CHECK(r,j,msg) \
            if (r < 0) { \
                sd_journal_close(j); \
                KReturn2(_cb, ERR_FAILED, msg ": %s\n", strerror_r(-r, errbuf, sizeof(errbuf))); \
             }

static CMPIStatus LMI_JournalLogRecordCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    const char *cr_cl_n;
    const char *log_cr_cl_n;
    const char *log_name;
    const char *data_format;
    sd_journal *journal;
    int r;
    CMPIrc rc;
    bool found;
    char errbuf[BUFLEN];

    cr_cl_n = get_string_property("CreationClassName", ci);
    log_cr_cl_n = get_string_property("LogCreationClassName", ci);
    log_name = get_string_property("LogName", ci);
    data_format = get_string_property("DataFormat", ci);

    if (cr_cl_n == NULL || log_cr_cl_n == NULL || log_name == NULL || data_format == NULL)
        KReturn(ERR_INVALID_PARAMETER);
    if (strcmp(cr_cl_n, LMI_JournalLogRecord_ClassName) != 0 ||
        strcmp(log_cr_cl_n, LMI_JournalMessageLog_ClassName) != 0 ||
        strcmp(log_name, JOURNAL_MESSAGE_LOG_NAME) != 0) {
        KReturn(ERR_INVALID_PARAMETER);
    }

    r = sd_journal_open(&journal, 0);
    if (r < 0) {
        KReturn2(_cb, ERR_FAILED, "Error opening journal: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
    }

    r = sd_journal_seek_tail(journal);
    J_RESULT_CHECK(r, journal, "Error seeking to the end of the journal");

    /* Need to call at least one of the _previous()/_next() functions to make the iterator set */
    r = sd_journal_previous(journal);
    if (r > 0)
        while ((r = sd_journal_next(journal)) > 0)
            ;
    J_RESULT_CHECK(r, journal, "Error seeking to the end of the journal");

    /* Queue send new record */
    r = sd_journal_send("MESSAGE=%s", data_format,
                        NULL);
    J_RESULT_CHECK(r, journal, "Error writing new record to the journal");

    r = sd_journal_wait(journal, WRITE_RECORD_TIMEOUT * 1000 /* usec */);
    /* First wait call will likely return INVALIDATE, try again */
    if (r == SD_JOURNAL_INVALIDATE)
        r = sd_journal_wait(journal, WRITE_RECORD_TIMEOUT * 1000 /* usec */);
    J_RESULT_CHECK(r, journal, "Error checking the journal for new records");

    found = false;
    while (sd_journal_next(journal) > 0) {
        /* TODO: alternatively, we can use custom property and an unique identifier
         *       but let's not bloat the journal with unneccessary data for now */
        r = match_journal_record(journal, data_format, __func__);
        if (r == 1) {
            LMI_JournalLogRecord log_record;
            char *cursor;

            LMI_JournalLogRecord_InitFromObjectPath(&log_record, _cb, cop);
            LMI_JournalLogRecord_Set_DataFormat(&log_record, data_format);

            /* Get stable cursor string */
            r = sd_journal_get_cursor(journal, &cursor);
            J_RESULT_CHECK(r, journal, "Failed to get cursor position");
            LMI_JournalLogRecord_Set_RecordID(&log_record, cursor);
            free(cursor);

            r = create_LMI_JournalLogRecord(journal, &log_record, _cb);
            if (r <= 0) {
                sd_journal_close(journal);
                KReturn2(_cb, ERR_FAILED, "Failed to create instance: %s\n", strerror_r(-r, errbuf, sizeof(errbuf)));
            }

            CMReturnObjectPath(cr, LMI_JournalLogRecord_ToObjectPath(&log_record, NULL));
            CMReturnDone(cr);
            found = true;
            break;
        }
        J_RESULT_CHECK(r, journal, "Failed to find newly written record");
    }
    sd_journal_close(journal);

    if (! found)
        KReturn2(_cb, ERR_FAILED, "Failed to find newly written record\n");
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalLogRecordModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_JournalLogRecordDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_JournalLogRecordExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

CMInstanceMIStub(
    LMI_JournalLogRecord,
    LMI_JournalLogRecord,
    _cb,
    LMI_JournalLogRecordInitialize(ctx))

static CMPIStatus LMI_JournalLogRecordMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalLogRecordInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_JournalLogRecord_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_JournalLogRecord,
    LMI_JournalLogRecord,
    _cb,
    LMI_JournalLogRecordInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_JournalLogRecord",
    "LMI_JournalLogRecord",
    "instance method")
