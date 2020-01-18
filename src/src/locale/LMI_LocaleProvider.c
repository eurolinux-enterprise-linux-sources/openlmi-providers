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

#include <konkret/konkret.h>
#include "LMI_Locale.h"
#include "localed.h"

const char LangEnvVar[] = "LANG=";
const char LCCTypeEnvVar[] = "LC_CTYPE=";
const char LCNumericEnvVar[] = "LC_NUMERIC=";
const char LCTimeEnvVar[] = "LC_TIME=";
const char LCCollateEnvVar[] = "LC_COLLATE=";
const char LCMonetaryEnvVar[] = "LC_MONETARY=";
const char LCMessagesEnvVar[] = "LC_MESSAGES=";
const char LCPaperEnvVar[] = "LC_PAPER=";
const char LCNameEnvVar[] = "LC_NAME=";
const char LCAddressEnvVar[] = "LC_ADDRESS=";
const char LCTelephoneEnvVar[] = "LC_TELEPHONE=";
const char LCMeasurementEnvVar[] = "LC_MEASUREMENT=";
const char LCIdentificationEnvVar[] = "LC_IDENTIFICATION=";

#define SETVAL(NAM) \
if ((NAM)->exists && !(NAM)->null) { \
    if (LocaleCnt >= KNOWN_LOC_STR_CNT) { \
        KSetStatus2(cb, status, ERR_FAILED, "Too many locale values.\n"); \
        KUint32_Set(&result, 2); \
        goto error; \
    } \
    tmps = (char *) calloc((strlen(NAM ## EnvVar) + strlen((NAM)->chars) + 1), sizeof(char)); \
    if (tmps == NULL) { \
        KSetStatus2(cb, status, ERR_FAILED, "Insufficient memory.\n"); \
        KUint32_Set(&result, 2); \
        free(tmps); \
        goto error; \
    } \
    strncpy(tmps, NAM ## EnvVar, strlen(NAM ## EnvVar)); \
    LocaleValue[LocaleCnt] = strncat(tmps, (NAM)->chars, strlen((NAM)->chars)); \
    LocaleCnt++; \
}

#define GETVAL(NAM) \
if (strncmp(cloc->Locale[i], NAM ## EnvVar, strlen(NAM ## EnvVar)) == 0) { \
    LMI_Locale_Set_ ## NAM(&w, cloc->Locale[i] + strlen(NAM ## EnvVar)); \
    continue; \
}

static const CMPIBroker* _cb = NULL;

static void LMI_LocaleInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
    locale_init();
}

static CMPIStatus LMI_LocaleCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_LocaleEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_LocaleEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CimLocale *cloc;

    cloc = locale_get_properties();
    if (cloc == NULL) {
        KReturn(ERR_FAILED);
    }

    LMI_Locale w;
    LMI_Locale_Init(&w, _cb, KNameSpace(cop));

    LMI_Locale_Set_CreationClassName(&w, LMI_Locale_ClassName);
    LMI_Locale_Set_SettingID(&w, "System Locale");
    LMI_Locale_Set_SystemCreationClassName(&w, lmi_get_system_creation_class_name());
    LMI_Locale_Set_SystemName(&w, lmi_get_system_name_safe(cc));

    for (int i = 0; i < cloc->LocaleCnt; i++) {
        GETVAL(Lang)
        GETVAL(LCCType)
        GETVAL(LCNumeric)
        GETVAL(LCTime)
        GETVAL(LCCollate)
        GETVAL(LCMonetary)
        GETVAL(LCMessages)
        GETVAL(LCPaper)
        GETVAL(LCName)
        GETVAL(LCAddress)
        GETVAL(LCTelephone)
        GETVAL(LCMeasurement)
        GETVAL(LCIdentification)
    }

    LMI_Locale_Set_VConsoleKeymap(&w, cloc->VConsoleKeymap);
    LMI_Locale_Set_VConsoleKeymapToggle(&w, cloc->VConsoleKeymapToggle);
    LMI_Locale_Set_X11Layouts(&w, cloc->X11Layouts);
    LMI_Locale_Set_X11Model(&w, cloc->X11Model);
    LMI_Locale_Set_X11Variant(&w, cloc->X11Variant);
    LMI_Locale_Set_X11Options(&w, cloc->X11Options);
    locale_free(cloc);

    KReturnInstance(cr, w);
    KReturn(OK);
}

static CMPIStatus LMI_LocaleGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_LocaleCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LocaleModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LocaleDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LocaleExecQuery(
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
    LMI_Locale,
    LMI_Locale,
    _cb,
    LMI_LocaleInitialize(ctx))

static CMPIStatus LMI_LocaleMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_LocaleInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Locale_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_Locale,
    LMI_Locale,
    _cb,
    LMI_LocaleInitialize(ctx))

KUint32 LMI_Locale_VerifyOKToApplyToMSE(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KRef* MSE,
    const KDateTime* TimeToApply,
    const KDateTime* MustBeCompletedBy,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Locale_ApplyToMSE(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KRef* MSE,
    const KDateTime* TimeToApply,
    const KDateTime* MustBeCompletedBy,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Locale_VerifyOKToApplyToCollection(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KRef* Collection,
    const KDateTime* TimeToApply,
    const KDateTime* MustBeCompletedBy,
    KStringA* CanNotApply,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Locale_ApplyToCollection(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KRef* Collection,
    const KDateTime* TimeToApply,
    const KBoolean* ContinueOnError,
    const KDateTime* MustBeCompletedBy,
    KStringA* CanNotApply,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Locale_VerifyOKToApplyIncrementalChangeToMSE(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KRef* MSE,
    const KDateTime* TimeToApply,
    const KDateTime* MustBeCompletedBy,
    const KStringA* PropertiesToApply,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Locale_ApplyIncrementalChangeToMSE(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KRef* MSE,
    const KDateTime* TimeToApply,
    const KDateTime* MustBeCompletedBy,
    const KStringA* PropertiesToApply,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Locale_VerifyOKToApplyIncrementalChangeToCollection(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KRef* Collection,
    const KDateTime* TimeToApply,
    const KDateTime* MustBeCompletedBy,
    const KStringA* PropertiesToApply,
    KStringA* CanNotApply,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Locale_ApplyIncrementalChangeToCollection(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KRef* Collection,
    const KDateTime* TimeToApply,
    const KBoolean* ContinueOnError,
    const KDateTime* MustBeCompletedBy,
    const KStringA* PropertiesToApply,
    KStringA* CanNotApply,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Locale_SetLocale(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KString* Lang,
    const KString* LCCType,
    const KString* LCNumeric,
    const KString* LCTime,
    const KString* LCCollate,
    const KString* LCMonetary,
    const KString* LCMessages,
    const KString* LCPaper,
    const KString* LCName,
    const KString* LCAddress,
    const KString* LCTelephone,
    const KString* LCMeasurement,
    const KString* LCIdentification,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    char *LocaleValue[KNOWN_LOC_STR_CNT];
    int LocaleCnt = 0;
    char *tmps;

    SETVAL(Lang)
    SETVAL(LCCType)
    SETVAL(LCNumeric)
    SETVAL(LCTime)
    SETVAL(LCCollate)
    SETVAL(LCMonetary)
    SETVAL(LCMessages)
    SETVAL(LCPaper)
    SETVAL(LCName)
    SETVAL(LCAddress)
    SETVAL(LCTelephone)
    SETVAL(LCMeasurement)
    SETVAL(LCIdentification)

    // At least one locale string required
    if (!LocaleCnt) {
        KSetStatus2(cb, status, ERR_FAILED, "At least one locale string is required.\n");
        KUint32_Set(&result, 3);
        goto error;
    }

    KUint32_Set(&result, set_locale(LocaleValue, LocaleCnt));

error:
    for (int i = 0; i < LocaleCnt; i++) {
        free(LocaleValue[i]);
    }

    return result;
}

KUint32 LMI_Locale_SetVConsoleKeyboard(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KString* Keymap,
    const KString* KeymapToggle,
    const KBoolean* Convert,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    int ConvertValue = 0;

    // Keymap is required
    if (!(Keymap->exists && !Keymap->null && !(Keymap->chars[0] == '\0'))) {
        KSetStatus2(cb, status, ERR_FAILED, "Required parameter 'Keymap' not specified.\n");
        KUint32_Set(&result, 1);
        return result;
    }

    // Convert set to TRUE - use it
    if (Convert->exists && !Convert->null && Convert->value)
        ConvertValue = 1;

    if (KeymapToggle->exists && !KeymapToggle->null)
        KUint32_Set(&result, set_v_console_keyboard(Keymap->chars, KeymapToggle->chars, ConvertValue));
    else
        KUint32_Set(&result, set_v_console_keyboard(Keymap->chars, "", ConvertValue));

    return result;
}

KUint32 LMI_Locale_SetX11Keyboard(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LocaleRef* self,
    const KString* Layouts,
    const KString* Model,
    const KString* Variant,
    const KString* Options,
    const KBoolean* Convert,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    int ConvertValue = 0;
    const char *ModelValue, *VariantValue, *OptionsValue;

    // Layout is required
    if (!(Layouts->exists && !Layouts->null && !(Layouts->chars[0] == '\0'))) {
        KSetStatus2(cb, status, ERR_FAILED, "Required parameter 'Layout' not specified.\n");
        KUint32_Set(&result, 1);
        return result;
    }

    // Convert set to TRUE - use it
    if (Convert->exists && !Convert->null && Convert->value)
        ConvertValue = 1;

    // Model, Variant and Options are optional
    if (Model->exists && !Model->null)
        ModelValue = Model->chars;
    else
        ModelValue = "";

    if (Variant->exists && !Variant->null)
        VariantValue = Variant->chars;
    else
        VariantValue = "";

    if (Options->exists && !Options->null)
        OptionsValue = Options->chars;
    else
        OptionsValue = "";

    KUint32_Set(&result, set_x11_keyboard(Layouts->chars, ModelValue, VariantValue, OptionsValue, ConvertValue));

    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_Locale",
    "LMI_Locale",
    "instance method")
