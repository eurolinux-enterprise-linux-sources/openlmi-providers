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

#include <konkret/konkret.h>
#include "LMI_JournalMessageLog.h"

#include <glib.h>
#include <systemd/sd-journal.h>

#include "globals.h"
#include "journal.h"
#include "instutil.h"


/* As defined in CIM_MessageLog schema */
#define CIM_MESSAGELOG_ITERATOR_RESULT_SUCCESS 0
#define CIM_MESSAGELOG_ITERATOR_RESULT_NOT_SUPPORTED 1
#define CIM_MESSAGELOG_ITERATOR_RESULT_FAILED 2

static const CMPIBroker* _cb = NULL;

static void LMI_JournalMessageLogInitialize(const CMPIContext *ctx)
{
    lmi_init(JOURNAL_CIM_LOG_NAME, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_JournalMessageLogCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalMessageLogEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_JournalMessageLogEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_JournalMessageLog message_log;
    const char *ns = KNameSpace(cop);
    sd_journal *journal;
    int caps = 0;
    uint64_t usec;
    CMPIDateTime *date;

    /* TODO: split into several instances by open flags? */
    if (sd_journal_open(&journal, 0) >= 0) {
        LMI_JournalMessageLog_Init(&message_log, _cb, ns);
        LMI_JournalMessageLog_Set_CreationClassName(&message_log, LMI_JournalMessageLog_ClassName);
        LMI_JournalMessageLog_Set_Name(&message_log, JOURNAL_MESSAGE_LOG_NAME);

        LMI_JournalMessageLog_Init_Capabilities(&message_log, 4);
        LMI_JournalMessageLog_Set_Capabilities_Can_Move_Backward_in_Log(&message_log, caps++);
        LMI_JournalMessageLog_Set_Capabilities_Variable_Formats_for_Records(&message_log, caps++);
        LMI_JournalMessageLog_Set_Capabilities_Variable_Length_Records_Supported(&message_log, caps++);
        LMI_JournalMessageLog_Set_Capabilities_Write_Record_Supported(&message_log, caps++);

        /* Optional: Time of last change */
        if (sd_journal_seek_tail(journal) >= 0 &&
            sd_journal_previous(journal) >= 0 &&
            sd_journal_get_realtime_usec(journal, &usec) >= 0) {
                date = CMNewDateTimeFromBinary(_cb, usec, 0, NULL);
                LMI_JournalMessageLog_Set_TimeOfLastChange(&message_log, date);
        }

        sd_journal_close(journal);
        KReturnInstance(cr, message_log);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalMessageLogGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_JournalMessageLogCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_JournalMessageLogModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_JournalMessageLogDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_JournalMessageLogExecQuery(
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
    LMI_JournalMessageLog,
    LMI_JournalMessageLog,
    _cb,
    LMI_JournalMessageLogInitialize(ctx))

static CMPIStatus LMI_JournalMessageLogMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_JournalMessageLogInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_JournalMessageLog_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_JournalMessageLog,
    LMI_JournalMessageLog,
    _cb,
    LMI_JournalMessageLogInitialize(ctx))

KUint32 LMI_JournalMessageLog_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_JournalMessageLogRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_JournalMessageLog_ClearLog(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_JournalMessageLogRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_JournalMessageLog_PositionToFirstRecord(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_JournalMessageLogRef* self,
    KString* IterationIdentifier,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    gchar *iter_id;

    KSetStatus(status, OK);

    if ((iter_id = journal_iter_new(NULL, NULL)) == NULL) {
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);
        return result;
    }

    KString_Set(IterationIdentifier, _cb, iter_id);
    KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_SUCCESS);
    g_free(iter_id);

    return result;
}

KUint32 LMI_JournalMessageLog_PositionAtRecord(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_JournalMessageLogRef* self,
    KString* IterationIdentifier,
    const KBoolean* MoveAbsolute,
    KSint64* RecordNumber,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    gchar *iter_id;
    sd_journal *journal;

    if (IterationIdentifier->null || ! IterationIdentifier->exists ||
        MoveAbsolute->null || ! MoveAbsolute->exists ||
        RecordNumber->null || ! RecordNumber->exists) {
        KSetStatus(status, ERR_INVALID_PARAMETER);
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);
        return result;
    }

    if (MoveAbsolute->value != 0) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Absolute movement not supported\n");
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);
        return result;
    }

    iter_id = g_strdup(IterationIdentifier->chars);
    if (! journal_iter_validate_id(&iter_id, &journal, NULL, _cb, status)) {
        g_free(iter_id);
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);
        return result;
    }

    if (! journal_iter_seek(&iter_id, journal, RecordNumber->value)) {
        g_free(iter_id);
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);
        return result;
    }

    KString_Set(IterationIdentifier, _cb, iter_id);
    KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_SUCCESS);
    g_free(iter_id);
    return result;
}

KUint32 LMI_JournalMessageLog_GetRecord(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_JournalMessageLogRef* self,
    KString* IterationIdentifier,
    const KBoolean* PositionToNext,
    KUint64* RecordNumber,
    KUint8A* RecordData,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    gchar *iter_id;
    sd_journal *journal;
    char *data;
    guint i;

    if (IterationIdentifier->null || ! IterationIdentifier->exists ||
        PositionToNext->null || ! PositionToNext->exists) {
        KSetStatus(status, ERR_INVALID_PARAMETER);
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);
        return result;
    }

    iter_id = g_strdup(IterationIdentifier->chars);
    if (! journal_iter_validate_id(&iter_id, &journal, NULL, _cb, status)) {
        g_free(iter_id);
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);
        return result;
    }

    data = journal_iter_get_data(&iter_id, journal, PositionToNext->value == 1);
    if (! data) {
        g_free(iter_id);
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);
        return result;
    }

    KUint8A_Init(RecordData, _cb, strlen(data));
    /* FIXME: This will feed the array with unsigned chars, NOT terminated by zero.
     *        Ideally we would like to pass string instead but the model is given. */
    /* TODO: do the real signed vs. unsigned char conversion? */
    for (i = 0; i < strlen(data); i++)
        KUint8A_Set(RecordData, i, (unsigned char) data[i]);

    g_free(data);

    KString_Set(IterationIdentifier, _cb, iter_id);
    KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_SUCCESS);
    g_free(iter_id);
    return result;
}

KUint32 LMI_JournalMessageLog_DeleteRecord(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_JournalMessageLogRef* self,
    KString* IterationIdentifier,
    const KBoolean* PositionToNext,
    KUint64* RecordNumber,
    KUint8A* RecordData,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_JournalMessageLog_WriteRecord(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_JournalMessageLogRef* self,
    KString* IterationIdentifier,
    const KBoolean* PositionToNext,
    const KUint8A* RecordData,
    KUint64* RecordNumber,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_JournalMessageLog_CancelIteration(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_JournalMessageLogRef* self,
    const KString* IterationIdentifier,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    gchar *iter_id_short;

    if (IterationIdentifier->null || ! IterationIdentifier->exists) {
        KSetStatus(status, ERR_INVALID_PARAMETER);
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);
        return result;
    }

    if (! journal_iter_parse_iterator_string(IterationIdentifier->chars, &iter_id_short, NULL, NULL)) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Malformed IterationIdentifier argument");
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);
        return result;
    }

    KSetStatus(status, OK);
    if (journal_iter_cancel(iter_id_short))
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_SUCCESS);
    else
        KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_FAILED);

    g_free(iter_id_short);
    return result;
}

KUint32 LMI_JournalMessageLog_FreezeLog(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_JournalMessageLogRef* self,
    const KBoolean* Freeze,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_JournalMessageLog_FlagRecordForOverwrite(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_JournalMessageLogRef* self,
    KString* IterationIdentifier,
    const KBoolean* PositionToNext,
    KUint64* RecordNumber,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    KUint32_Set(&result, CIM_MESSAGELOG_ITERATOR_RESULT_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_JournalMessageLog",
    "LMI_JournalMessageLog",
    "instance method")
