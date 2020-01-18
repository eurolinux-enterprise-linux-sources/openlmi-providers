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
#include "LMI_JournalRecordInLog.h"

#include <systemd/sd-journal.h>

#include "journal.h"
#include "instutil.h"

static const CMPIBroker* _cb;

static void LMI_JournalRecordInLogInitialize(const CMPIContext *ctx)
{
    lmi_init(JOURNAL_CIM_LOG_NAME, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_JournalRecordInLogCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalRecordInLogEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_JournalRecordInLogEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_JournalRecordInLog record_in_log;
    LMI_JournalMessageLogRef message_log_ref;
    CMPIObjectPath *message_log_ref_op;
    LMI_JournalLogRecordRef log_record_ref;
    const char *ns = KNameSpace(cop);
    sd_journal *journal;
    int r;
    CMPIStatus rc;
    unsigned long count = 0;
    char errbuf[BUFLEN];

    LMI_JournalMessageLogRef_Init(&message_log_ref, _cb, ns);
    LMI_JournalMessageLogRef_Set_CreationClassName(&message_log_ref, LMI_JournalMessageLog_ClassName);
    LMI_JournalMessageLogRef_Set_Name(&message_log_ref, JOURNAL_MESSAGE_LOG_NAME);

    message_log_ref_op = LMI_JournalMessageLogRef_ToObjectPath(&message_log_ref, &rc);
    message_log_ref_op->ft->setClassName(message_log_ref_op, JOURNAL_MESSAGE_LOG_NAME);

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

        LMI_JournalRecordInLog_Init(&record_in_log, _cb, ns);
        LMI_JournalRecordInLog_SetObjectPath_MessageLog(&record_in_log, message_log_ref_op);
        LMI_JournalRecordInLog_Set_LogRecord(&record_in_log, &log_record_ref);

        KReturnInstance(cr, record_in_log);

        /* Instance number limiter */
        count++;
        if (JOURNAL_MAX_INSTANCES_NUM > 0 && count > JOURNAL_MAX_INSTANCES_NUM)
            break;
    }
    sd_journal_close(journal);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalRecordInLogGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_JournalRecordInLogCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_JournalRecordInLogModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_JournalRecordInLogDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_JournalRecordInLogExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_JournalRecordInLogAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalRecordInLogAssociators(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole,
    const char** properties)
{
    return KDefaultAssociators(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_JournalRecordInLog_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_JournalRecordInLogAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return KDefaultAssociatorNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_JournalRecordInLog_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_JournalRecordInLogReferences(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    return KDefaultReferences(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_JournalRecordInLog_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_JournalRecordInLogReferenceNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role)
{
    return KDefaultReferenceNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_JournalRecordInLog_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_JournalRecordInLog,
    LMI_JournalRecordInLog,
    _cb,
    LMI_JournalRecordInLogInitialize(ctx))

CMAssociationMIStub(
    LMI_JournalRecordInLog,
    LMI_JournalRecordInLog,
    _cb,
    LMI_JournalRecordInLogInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_JournalRecordInLog",
    "LMI_JournalRecordInLog",
    "instance association")
