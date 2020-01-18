/*
 * Copyright (C) 2013 Red Hat, Inc. All rights reserved.
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
 * Authors: Peter Schiffer <pschiffe@redhat.com>
 */

#include <konkret/konkret.h>
#include "LMI_ProcessorChip.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"

static const CMPIBroker* _cb = NULL;

static void LMI_ProcessorChipInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_ProcessorChipCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorChipEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ProcessorChipEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_ProcessorChip lmi_cpu_chip;
    const char *ns = KNameSpace(cop);
    char instance_id[INSTANCE_ID_LEN];
    unsigned i;
    DmiProcessor *dmi_cpus = NULL;
    unsigned dmi_cpus_nb = 0;

    if (dmi_get_processors(&dmi_cpus, &dmi_cpus_nb) != 0 || dmi_cpus_nb < 1) {
        goto done;
    }

    for (i = 0; i < dmi_cpus_nb; i++) {
        LMI_ProcessorChip_Init(&lmi_cpu_chip, _cb, ns);

        LMI_ProcessorChip_Set_CreationClassName(&lmi_cpu_chip,
                ORGID "_" CPU_CHIP_CLASS_NAME);

        snprintf(instance_id, INSTANCE_ID_LEN,
                ORGID ":" ORGID "_" CPU_CHIP_CLASS_NAME ":%s", dmi_cpus[i].id);

        LMI_ProcessorChip_Set_Tag(&lmi_cpu_chip, dmi_cpus[i].id);
        LMI_ProcessorChip_Set_ElementName(&lmi_cpu_chip, dmi_cpus[i].name);
        LMI_ProcessorChip_Set_Manufacturer(&lmi_cpu_chip,
                dmi_cpus[i].manufacturer);
        LMI_ProcessorChip_Set_Model(&lmi_cpu_chip, dmi_cpus[i].name);
        LMI_ProcessorChip_Set_SerialNumber(&lmi_cpu_chip,
                dmi_cpus[i].serial_number);
        LMI_ProcessorChip_Set_PartNumber(&lmi_cpu_chip,
                dmi_cpus[i].part_number);
        LMI_ProcessorChip_Set_Caption(&lmi_cpu_chip, "Processor Chip");
        LMI_ProcessorChip_Set_Description(&lmi_cpu_chip,
                "This object represents one chip of processor in system.");
        LMI_ProcessorChip_Set_InstanceID(&lmi_cpu_chip, instance_id);
        LMI_ProcessorChip_Set_Name(&lmi_cpu_chip, dmi_cpus[i].name);

        KReturnInstance(cr, lmi_cpu_chip);
    }

done:
    dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorChipGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ProcessorChipCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorChipModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorChipDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorChipExecQuery(
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
    LMI_ProcessorChip,
    LMI_ProcessorChip,
    _cb,
    LMI_ProcessorChipInitialize(ctx))

static CMPIStatus LMI_ProcessorChipMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorChipInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_ProcessorChip_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_ProcessorChip,
    LMI_ProcessorChip,
    _cb,
    LMI_ProcessorChipInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ProcessorChip",
    "LMI_ProcessorChip",
    "instance method")
