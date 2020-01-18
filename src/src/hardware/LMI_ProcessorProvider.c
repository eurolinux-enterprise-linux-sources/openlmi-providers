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
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include "LMI_Processor.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"
#include "lscpu.h"
#include "procfs.h"

/**
 * CIM schema defines Values for CIM_Processor::Family property since version
 * 2.38.0. With older versions the LMI_Processor_Family_Enum won't be generated
 * by konkret (in LMI_Processor.h).
*/
#ifndef LMI_Processor_Family_Enum
    // CIM schema is older than 2.38.0
    #define LMI_Processor_Family_Other 1
#endif

CMPIUint16 get_family(const char *family);
CMPIUint16 get_cpustatus(const char *status);
CMPIUint16 get_enabledstate(const CMPIUint16 status);
CMPIUint16 get_upgrade_method(const char *dmi_upgrade);
CMPIUint16 get_characteristic(const char *dmi_charact);
CMPIUint16 get_flag(const char *flag, short *stat);

static const CMPIBroker* _cb = NULL;

static void LMI_ProcessorInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_ProcessorCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ProcessorEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    LMI_Processor lmi_cpu;
    const char *ns = KNameSpace(cop);
    CMPICount count;
    CMPIUint16 cpustatus, enabledstate, family, charact, enabled_cores;
    CMPIUint32 current_speed = 0, max_speed = 0, external_clock = 0;
    unsigned i, j, cpus_nb = 0;
    char *other_family = NULL, *architecture = NULL, *cpu_name = NULL,
            *stepping = NULL, *error_msg = NULL,
            instance_id[INSTANCE_ID_LEN];
    short ret1, ret2;
    struct utsname utsname_buf;
    DmiProcessor *dmi_cpus = NULL;
    unsigned dmi_cpus_nb = 0;
    LscpuProcessor lscpu;
    CpuinfoProcessor proc_cpu;

    if (dmi_get_processors(&dmi_cpus, &dmi_cpus_nb) != 0 || dmi_cpus_nb < 1) {
        dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);
    }

    ret1 = lscpu_get_processor(&lscpu);
    ret2 = cpuinfo_get_processor(&proc_cpu);
    if (ret1 != 0 || ret2 != 0) {
        error_msg = "Unable to get processor information.";
        goto done;
    }

    if (uname(&utsname_buf) == 0) {
        architecture = strdup(utsname_buf.machine);
        if (!architecture) {
            error_msg = "Not enough available memory.";
            goto done;
        }
    }

    if (dmi_cpus_nb > 0) {
        cpus_nb = dmi_cpus_nb;
    } else if (lscpu.processors > 0) {
        cpus_nb = lscpu.processors;
    } else {
        error_msg = "Unable to get processor information.";
        goto done;
    }

    for (i = 0; i < cpus_nb; i++) {
        LMI_Processor_Init(&lmi_cpu, _cb, ns);

        LMI_Processor_Set_SystemCreationClassName(&lmi_cpu,
                get_system_creation_class_name());
        LMI_Processor_Set_SystemName(&lmi_cpu, get_system_name());
        LMI_Processor_Set_CreationClassName(&lmi_cpu, ORGID "_" CPU_CLASS_NAME);
        LMI_Processor_Set_Caption(&lmi_cpu, CPU_CLASS_NAME);
        LMI_Processor_Set_Description(&lmi_cpu,
                "This object represents one processor in system.");

        /* do we have output from dmidecode program? */
        if (dmi_cpus_nb > 0) {
            family = get_family(dmi_cpus[i].family);
            if (family == LMI_Processor_Family_Other) {
                other_family = dmi_cpus[i].family;
            }
            cpustatus = get_cpustatus(dmi_cpus[i].status);
            enabledstate = get_enabledstate(cpustatus);
            if (enabledstate == LMI_Processor_EnabledState_Enabled) {
                current_speed = dmi_cpus[i].current_speed;
                max_speed = dmi_cpus[i].max_speed;
                external_clock = dmi_cpus[i].external_clock;
            }
            cpu_name = dmi_cpus[i].name;
            enabled_cores = dmi_cpus[i].enabled_cores;
            stepping = dmi_cpus[i].stepping;
            snprintf(instance_id, INSTANCE_ID_LEN,
                    ORGID ":" ORGID "_" CPU_CLASS_NAME ":%s", dmi_cpus[i].id);

            LMI_Processor_Set_DeviceID(&lmi_cpu, dmi_cpus[i].id);
            LMI_Processor_Set_Family(&lmi_cpu, family);
            LMI_Processor_Set_OtherFamilyDescription(&lmi_cpu, other_family);
            LMI_Processor_Set_UpgradeMethod(&lmi_cpu,
                    get_upgrade_method(dmi_cpus[i].upgrade));
            if (dmi_cpus[i].type && strlen(dmi_cpus[i].type)) {
                LMI_Processor_Set_Role(&lmi_cpu, dmi_cpus[i].type);
            }
            /* CPU Characteristics */
            if (dmi_cpus[i].charact_nb > 0) {
                count = 0;
                LMI_Processor_Init_Characteristics(&lmi_cpu,
                        dmi_cpus[i].charact_nb);
                LMI_Processor_Init_EnabledProcessorCharacteristics(&lmi_cpu,
                        dmi_cpus[i].charact_nb);
                for (j = 0; j < dmi_cpus[i].charact_nb; j++) {
                    charact = get_characteristic(
                            dmi_cpus[i].characteristics[j]);
                    if (charact) {
                        LMI_Processor_Set_Characteristics(&lmi_cpu, count,
                                charact);
                        LMI_Processor_Set_EnabledProcessorCharacteristics(
                                &lmi_cpu, count,
                                LMI_Processor_EnabledProcessorCharacteristics_Unknown);
                        count++;
                    }
                }
            }
        } else {
            char cpu_id[LONG_INT_LEN];
            snprintf(cpu_id, LONG_INT_LEN, "%u", i);
            snprintf(instance_id, INSTANCE_ID_LEN,
                    ORGID ":" ORGID "_" CPU_CLASS_NAME ":%s", cpu_id);
            cpustatus = get_cpustatus("Enabled");
            enabledstate = get_enabledstate(cpustatus);
            if (enabledstate == LMI_Processor_EnabledState_Enabled) {
                current_speed = lscpu.current_speed;
            }
            cpu_name = proc_cpu.model_name;
            enabled_cores = lscpu.cores;
            stepping = lscpu.stepping;

            LMI_Processor_Set_DeviceID(&lmi_cpu, cpu_id);
        }

        LMI_Processor_Set_InstanceID(&lmi_cpu, instance_id);
        LMI_Processor_Set_CPUStatus(&lmi_cpu, cpustatus);
        LMI_Processor_Set_EnabledState(&lmi_cpu, enabledstate);
        LMI_Processor_Set_NumberOfEnabledCores(&lmi_cpu, enabled_cores);
        LMI_Processor_Set_CurrentClockSpeed(&lmi_cpu, current_speed);
        LMI_Processor_Set_MaxClockSpeed(&lmi_cpu, max_speed);
        LMI_Processor_Set_ExternalBusClockSpeed(&lmi_cpu, external_clock);
        LMI_Processor_Init_OperationalStatus(&lmi_cpu, 1);
        LMI_Processor_Set_OperationalStatus(&lmi_cpu, 0,
                LMI_Processor_OperationalStatus_Unknown);
        LMI_Processor_Set_HealthState(&lmi_cpu,
                LMI_Processor_HealthState_Unknown);
        if (cpu_name && strlen(cpu_name)) {
            LMI_Processor_Set_Name(&lmi_cpu, cpu_name);
            LMI_Processor_Set_UniqueID(&lmi_cpu, cpu_name);
            LMI_Processor_Set_ElementName(&lmi_cpu, cpu_name);
        }
        if (stepping && strlen(stepping)) {
            LMI_Processor_Set_Stepping(&lmi_cpu, stepping);
        }
        if (lscpu.data_width) {
            LMI_Processor_Set_DataWidth(&lmi_cpu, lscpu.data_width);
        }
        /* CPU Flags */
        if (proc_cpu.flags_nb > 0) {
            short stat = -1;
            CMPIUint16 flag;
            LMI_Processor_Init_Flags(&lmi_cpu, proc_cpu.flags_nb);
            count = 0;
            for (j = 0; j < proc_cpu.flags_nb; j++) {
                flag = get_flag(proc_cpu.flags[j], &stat);
                if (stat == 0) {
                    LMI_Processor_Set_Flags(&lmi_cpu, count, flag);
                    count++;
                }
            }
        }
        if (proc_cpu.address_size) {
            LMI_Processor_Set_AddressWidth(&lmi_cpu, proc_cpu.address_size);
        }
        if (architecture && strlen(architecture)) {
            LMI_Processor_Set_Architecture(&lmi_cpu, architecture);
        }

        KReturnInstance(cr, lmi_cpu);
    }

done:
    free(architecture);

    dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);
    lscpu_free_processor(&lscpu);
    cpuinfo_free_processor(&proc_cpu);

    if (error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ProcessorCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ProcessorExecQuery(
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
    LMI_Processor,
    LMI_Processor,
    _cb,
    LMI_ProcessorInitialize(ctx))

static CMPIStatus LMI_ProcessorMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ProcessorInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_Processor_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_Processor,
    LMI_Processor,
    _cb,
    LMI_ProcessorInitialize(ctx))

KUint32 LMI_Processor_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Processor_SetPowerState(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorRef* self,
    const KUint16* PowerState,
    const KDateTime* Time,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Processor_Reset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Processor_EnableDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorRef* self,
    const KBoolean* Enabled,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Processor_OnlineDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorRef* self,
    const KBoolean* Online,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Processor_QuiesceDevice(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorRef* self,
    const KBoolean* Quiesce,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Processor_SaveProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_Processor_RestoreProperties(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_ProcessorRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

/*
 * Get CPU status according to the dmidecode.
 * @param status from dmidecode
 * @return CIM id of CPU status
 */
CMPIUint16 get_cpustatus(const char *status)
{
    if (!status) {
        return 0; /* Unknown */
    }

    static struct {
        CMPIUint16 val;     /* CIM value */
        char *stat;         /* dmidecode status */
    } statuses[] = {
        {0, "Unknown"},
        {1, "Enabled"},
        {2, "Disabled By User"},
        {3, "Disabled By BIOS"},
        {4, "Idle"},
    };

    size_t i, st_length = sizeof(statuses) / sizeof(statuses[0]);

    for (i = 0; i < st_length; i++) {
        if (strcmp(status, statuses[i].stat) == 0) {
            return statuses[i].val;
        }
    }

    return 0; /* Unknown */
}

/*
 * Get enabled state according to the cpu status.
 * @param status by CIM
 * @return CIM id of enabled state
 */
CMPIUint16 get_enabledstate(const CMPIUint16 status)
{
    static struct {
        CMPIUint16 cpustatus;       /* CIM cpu status */
        CMPIUint16 enabledstate;    /* CIM enabled state */
    } statuses[] = {
        {0, 0},
        {1, 2},
        {2, 3},
        {3, 3},
        {4, 2},
    };

    size_t i, st_length = sizeof(statuses) / sizeof(statuses[0]);

    for (i = 0; i < st_length; i++) {
        if (status == statuses[i].cpustatus) {
            return statuses[i].enabledstate;
        }
    }

    return 0; /* Unknown */
}

/*
 * Get CPU characteristics according to the dmidecode output.
 * @param dmi_charact characteristics from dmidecode
 * @return characteristic defined by CIM
 */
CMPIUint16 get_characteristic(const char *dmi_charact)
{
    if (!dmi_charact) {
        return 0; /* Unknown */
    }

    /* CIM characteristics based on
       cim_schema_2.35.0Final-MOFs/Device/CIM_Processor.mof */
    /* Dmidecode characts based on dmidecode 2.11 */
    static struct {
        CMPIUint16 value_map;   /* ValueMap defined by CIM */
        char *value;            /* Value defined by CIM (just for reference) */
        char *search;           /* String used to match the dmidecode charact */
    } characts[] = {
        /*
        {0, "Unknown", ""},
        {1, "DMTF Reserved", ""},
        */
        {2, "64-bit Capable", "64-bit capable"},
        /*
        {3, "32-bit Capable", ""},
        */
        {4, "Enhanced Virtualization", "Enhanced Virtualization"},
        {5, "Hardware Thread", "Hardware Thread"},
        {6, "NX-bit", "Execute Protection"},
        {7, "Power/Performance Control", "Power/Performance Control"},
        /*
        {8, "Core Frequency Boosting", ""},
        */
        {32568, "Multi-Core", "Multi-Core"},
    };

    size_t i, characts_length = sizeof(characts) / sizeof(characts[0]);

    for (i = 0; i < characts_length; i++) {
        if (strcmp(dmi_charact, characts[i].search) == 0) {
            return characts[i].value_map;
        }
    }

    return 0; /* Unknown */
}

/*
 * Get CPU upgrade method according to the dmidecode upgrade.
 * @param dmi_upgrade socket string from dmidecode program
 * @return upgrade method id defined by CIM
 */
CMPIUint16 get_upgrade_method(const char *dmi_upgrade)
{
    if (!dmi_upgrade) {
        return 2; /* Unknown */
    }

    /* CIM upgrade method based on
       cim_schema_2.35.0Final-MOFs/Device/CIM_Processor.mof */
    /* Dmidecode upgrade based on dmidecode 2.11 */
    static struct {
        CMPIUint16 value_map;   /* ValueMap defined by CIM */
        char *value;            /* Value defined by CIM (just for reference) */
        char *search;           /* String used to match the dmidecode upgrade */
    } sockets[] = {
        {1,  "Other", "Other"},
        {2,  "Unknown", "Unknown"},
        {3,  "Daughter Board", "Daughter Board"},
        {4,  "ZIF Socket", "ZIF Socket"},
        {5,  "Replacement/Piggy Back", "Replaceable Piggy Back"},
        {6,  "None", "None"},
        {7,  "LIF Socket", "LIF Socket"},
        {8,  "Slot 1", "Slot 1"},
        {9,  "Slot 2", "Slot 2"},
        {10, "370 Pin Socket", "370-pin Socket"},
        {11, "Slot A", "Slot A"},
        {12, "Slot M", "Slot M"},
        {13, "Socket 423", "Socket 423"},
        {14, "Socket A (Socket 462)", "Socket A (Socket 462)"},
        {15, "Socket 478", "Socket 478"},
        {16, "Socket 754", "Socket 754"},
        {17, "Socket 940", "Socket 940"},
        {18, "Socket 939", "Socket 939"},
        {19, "Socket mPGA604", "Socket mPGA604"},
        {20, "Socket LGA771", "Socket LGA771"},
        {21, "Socket LGA775", "Socket LGA775"},
        {22, "Socket S1", "Socket S1"},
        {23, "Socket AM2", "Socket AM2"},
        {24, "Socket F (1207)", "Socket F (1207)"},
        {25, "Socket LGA1366", "Socket LGA1366"},
        {26, "Socket G34", "Socket G34"},
        {27, "Socket AM3", "Socket AM3"},
        {28, "Socket C32", "Socket C32"},
        {29, "Socket LGA1156", "Socket LGA1156"},
        {30, "Socket LGA1567", "Socket LGA1567"},
        {31, "Socket PGA988A", "Socket PGA988A"},
        {32, "Socket BGA1288", "Socket BGA1288"},
        {33, "rPGA988B", "Socket rPGA988B"},
        {34, "BGA1023", "Socket BGA1023"},
        {35, "BGA1224", "Socket BGA1024"},
        {36, "LGA1155", "Socket BGA1155"},
        {37, "LGA1356", "Socket LGA1356"},
        {38, "LGA2011", "Socket LGA2011"},
        {39, "Socket FS1", "Socket FS1"},
        {40, "Socket FS2", "Socket FS2"},
        {41, "Socket FM1", "Socket FM1"},
        {42, "Socket FM2", "Socket FM2"},
        /*
        {43, "Socket LGA2011-3", ""},
        {44, "Socket LGA1356-3", ""},
        */
    };

    size_t i, sockets_length = sizeof(sockets) / sizeof(sockets[0]);

    for (i = 0; i < sockets_length; i++) {
        if (strcmp(dmi_upgrade, sockets[i].search) == 0) {
            return sockets[i].value_map;
        }
    }

    return LMI_Processor_Family_Other;
}

/*
 * Get CPU flag id.
 * @param flag from /proc/cpuinfo file
 * @param stat 0 if flag found, negative value otherwise
 * @return CIM id of flag
 */
CMPIUint16 get_flag(const char *flag, short *stat)
{
    if (!flag) {
        *stat = -1;
        return 0;
    }

    /* Flag IDs and names are based on:
       linux-3.8/arch/x86/include/asm/cpufeature.h */
    static struct {
        CMPIUint16 val;     /* CIM value */
        char *flag;         /* flag name */
    } flags[] = {
        /* 0*32 */
        {0,   "fpu"},
        {1,   "vme"},
        {2,   "de"},
        {3,   "pse"},
        {4,   "tsc"},
        {5,   "msr"},
        {6,   "pae"},
        {7,   "mce"},
        {8,   "cx8"},
        {9,   "apic"},
        {11,  "sep"},
        {12,  "mtrr"},
        {13,  "pge"},
        {14,  "mca"},
        {15,  "cmov"},
        {16,  "pat"},
        {17,  "pse36"},
        {18,  "pn"},
        {19,  "clflush"},
        {21,  "dts"},
        {22,  "acpi"},
        {23,  "mmx"},
        {24,  "fxsr"},
        {25,  "sse"},
        {26,  "sse2"},
        {27,  "ss"},
        {28,  "ht"},
        {29,  "tm"},
        {30,  "ia64"},
        {31,  "pbe"},
        /* 1*32 */
        {43,  "syscall"},
        {51,  "mp"},
        {52,  "nx"},
        {54,  "mmxext"},
        {57,  "fxsr_opt"},
        {58,  "pdpe1gb"},
        {59,  "rdtscp"},
        {61,  "lm"},
        {62,  "3dnowext"},
        {63,  "3dnow"},
        /* 2*32 */
        {64,  "recovery"},
        {65,  "longrun"},
        {67,  "lrti"},
        /* 3*32 */
        {96,  "cxmmx"},
        {97,  "k6_mtrr"},
        {98,  "cyrix_arr"},
        {99,  "centaur_mcr"},
        {100, "k8"},
        {101, "k7"},
        {102, "p3"},
        {103, "p4"},
        {104, "constant_tsc"},
        {105, "up"},
        {106, "fxsave_leak"},
        {107, "arch_perfmon"},
        {108, "pebs"},
        {109, "bts"},
        {110, "syscall32"},
        {111, "sysenter32"},
        {112, "rep_good"},
        {113, "mfence_rdtsc"},
        {114, "lfence_rdtsc"},
        {115, "11ap"},
        {116, "nopl"},
        {118, "xtopology"},
        {119, "tsc_reliable"},
        {120, "nonstop_tsc"},
        {121, "clflush_monitor"},
        {122, "extd_apicid"},
        {123, "amd_dcm"},
        {124, "aperfmperf"},
        {125, "eagerfpu"},
        /* 4*32 */
        {128, "pni"},
        {129, "pclmulqdq"},
        {130, "dtes64"},
        {131, "monitor"},
        {132, "ds_cpl"},
        {133, "vmx"},
        {134, "smx"},
        {135, "est"},
        {136, "tm2"},
        {137, "ssse3"},
        {138, "cid"},
        {140, "fma"},
        {141, "cx16"},
        {142, "xtpr"},
        {143, "pdcm"},
        {145, "pcid"},
        {146, "dca"},
        {147, "sse4_1"},
        {148, "sse4_2"},
        {149, "x2apic"},
        {150, "movbe"},
        {151, "popcnt"},
        {152, "tsc_deadline_timer"},
        {153, "aes"},
        {154, "xsave"},
        {155, "osxsave"},
        {156, "avx"},
        {157, "f16c"},
        {158, "rdrand"},
        {159, "hypervisor"},
        /* 5*32*/
        {162, "rng"},
        {163, "rng_en"},
        {166, "ace"},
        {167, "ace_en"},
        {168, "ace2"},
        {169, "ace2_en"},
        {170, "phe"},
        {171, "phe_en"},
        {172, "pmm"},
        {173, "pmm_en"},
        /* 6*32 */
        {192, "lahf_lm"},
        {193, "cmp_legacy"},
        {194, "svm"},
        {195, "extapic"},
        {196, "cr8_legacy"},
        {197, "abm"},
        {198, "sse4a"},
        {199, "misalignsse"},
        {200, "3dnowprefetch"},
        {201, "osvw"},
        {202, "ibs"},
        {203, "xop"},
        {204, "skinit"},
        {205, "wdt"},
        {207, "lwp"},
        {208, "fma4"},
        {209, "tce"},
        {211, "nodeid_msr"},
        {213, "tbm"},
        {214, "topoext"},
        {215, "perfctr_core"},
        /* 7*32 */
        {224, "ida"},
        {225, "arat"},
        {226, "cpb"},
        {227, "epb"},
        {228, "xsaveopt"},
        {229, "pln"},
        {230, "pts"},
        {231, "dtherm"},
        {232, "hw_pstate"},
        /* 8*32 */
        {256, "tpr_shadow"},
        {257, "vnmi"},
        {258, "flexpriority"},
        {259, "ept"},
        {260, "vpid"},
        {261, "npt"},
        {262, "lbrv"},
        {263, "svm_lock"},
        {264, "nrip_save"},
        {265, "tsc_scale"},
        {266, "vmcb_clean"},
        {267, "flushbyasid"},
        {268, "decodeassists"},
        {269, "pausefilter"},
        {270, "pfthreshold"},
        /* 9*32 */
        {288, "fsgsbase"},
        {289, "tsc_adjust"},
        {291, "bmi1"},
        {292, "hle"},
        {293, "avx2"},
        {295, "smep"},
        {296, "bmi2"},
        {297, "erms"},
        {298, "invpcid"},
        {299, "rtm"},
        {306, "rdseed"},
        {307, "adx"},
        {308, "smap"},
    };

    size_t i, st_length = sizeof(flags) / sizeof(flags[0]);

    for (i = 0; i < st_length; i++) {
        if (strcmp(flag, flags[i].flag) == 0) {
            *stat = 0;
            return flags[i].val;
        }
    }

    *stat = -1;
    return 0;
}

/*
 * Get CPU family id according to the dmidecode family.
 * @param dmi_family family string from dmidecode program
 * @return family id defined by CIM
 */
CMPIUint16 get_family(const char *dmi_family)
{
    if (!dmi_family) {
        return 2; /* Unknown */
    }

    /* CIM CPU families based on
       cim_schema_2.35.0Final-MOFs/Device/CIM_Processor.mof */
    /* Dmidecode families based on dmidecode 2.11 */
    static struct {
        CMPIUint16 value_map;   /* ValueMap defined by CIM */
        char *value;            /* Value defined by CIM (just for reference) */
        char *search;           /* String used to match the dmidecode family */
    } fm[] = {
        {1,   "Other", "Other"},
        {2,   "Unknown", "Unknown"},
        {3,   "8086", "8086"},
        {4,   "80286", "80286"},
        {5,   "80386", "80386"},
        {6,   "80486", "80486"},
        {7,   "8087", "8087"},
        {8,   "80287", "80287"},
        {9,   "80387", "80387"},
        {10,  "80487", "80487"},
        {11,  "Pentium(R) brand", "Pentium"},
        {12,  "Pentium(R) Pro", "Pentium Pro"},
        {13,  "Pentium(R) II", "Pentium II"},
        {14,  "Pentium(R) processor with MMX(TM) technology", "Pentium MMX"},
        {15,  "Celeron(TM)", "Celeron"},
        {16,  "Pentium(R) II Xeon(TM)", "Pentium II Xeon"},
        {17,  "Pentium(R) III", "Pentium III"},
        {18,  "M1 Family", "M1"},
        {19,  "M2 Family", "M2"},
        {20,  "Intel(R) Celeron(R) M processor", "Celeron M"},
        {21,  "Intel(R) Pentium(R) 4 HT processor", "Pentium 4 HT"},

        {24,  "K5 Family", "K5"},
        {25,  "K6 Family", "K6"},
        {26,  "K6-2", "K6-2"},
        {27,  "K6-3", "K6-3"},
        {28,  "AMD Athlon(TM) Processor Family", "Athlon"},
        {29,  "AMD(R) Duron(TM) Processor", "Duron"},
        {30,  "AMD29000 Family", "AMD29000"},
        {31,  "K6-2+", "K6-2+"},
        {32,  "Power PC Family", "Power PC"},
        {33,  "Power PC 601", "Power PC 601"},
        {34,  "Power PC 603", "Power PC 603"},
        {35,  "Power PC 603+", "Power PC 603+"},
        {36,  "Power PC 604", "Power PC 604"},
        {37,  "Power PC 620", "Power PC 620"},
        {38,  "Power PC X704", "Power PC x704"},
        {39,  "Power PC 750", "Power PC 750"},
        {40,  "Intel(R) Core(TM) Duo processor", "Core Duo"},
        {41,  "Intel(R) Core(TM) Duo mobile processor", "Core Duo Mobile"},
        {42,  "Intel(R) Core(TM) Solo mobile processor", "Core Solo Mobile"},
        {43,  "Intel(R) Atom(TM) processor", "Atom"},

        {48,  "Alpha Family", "Alpha"},
        {49,  "Alpha 21064", "Alpha 21064"},
        {50,  "Alpha 21066", "Alpha 21066"},
        {51,  "Alpha 21164", "Alpha 21164"},
        {52,  "Alpha 21164PC", "Alpha 21164PC"},
        {53,  "Alpha 21164a", "Alpha 21164a"},
        {54,  "Alpha 21264", "Alpha 21264"},
        {55,  "Alpha 21364", "Alpha 21364"},
        {56,  "AMD Turion(TM) II Ultra Dual-Core Mobile M Processor Family",
                "Turion II Ultra Dual-Core Mobile M"},
        {57,  "AMD Turion(TM) II Dual-Core Mobile M Processor Family",
                "Turion II Dual-Core Mobile M"},
        {58,  "AMD Athlon(TM) II Dual-Core Mobile M Processor Family",
                "Athlon II Dual-Core M"},
        {59,  "AMD Opteron(TM) 6100 Series Processor", "Opteron 6100"},
        {60,  "AMD Opteron(TM) 4100 Series Processor", "Opteron 4100"},
        {61,  "AMD Opteron(TM) 6200 Series Processor", "Opteron 6200"},
        {62,  "AMD Opteron(TM) 4200 Series Processor", "Opteron 4200"},

        /*
        {63,  "AMD FX(TM) Series Processor", ""},
        */
        {64,  "MIPS Family", "MIPS"},
        {65,  "MIPS R4000", "MIPS R4000"},
        {66,  "MIPS R4200", "MIPS R4200"},
        {67,  "MIPS R4400", "MIPS R4400"},
        {68,  "MIPS R4600", "MIPS R4600"},
        {69,  "MIPS R10000", "MIPS R10000"},
        {70,  "AMD C-Series Processor", "C-Series"},
        {71,  "AMD E-Series Processor", "E-Series"},
        /*
        {72,  "AMD A-Series Processor", ""},
        */
        {73,  "AMD G-Series Processor", "G-Series"},
        /*
        {74,  "AMD Z-Series Processor", ""},
        {75,  "AMD R-Series Processor", ""},
        {76,  "AMD Opteron(TM) 4300 Series Processor", ""},
        {77,  "AMD Opteron(TM) 6300 Series Processor", ""},
        {78,  "AMD Opteron(TM) 3300 Series Processor", ""},
        {79,  "AMD FirePro(TM) Series Processor", ""},
        */

        {80,  "SPARC Family", "SPARC"},
        {81,  "SuperSPARC", "SuperSPARC"},
        {82,  "microSPARC II", "MicroSPARC II"},
        {83,  "microSPARC IIep", "MicroSPARC IIep"},
        {84,  "UltraSPARC", "UltraSPARC"},
        {85,  "UltraSPARC II", "UltraSPARC II"},
        {86,  "UltraSPARC IIi", "UltraSPARC IIi"},
        {87,  "UltraSPARC III", "UltraSPARC III"},
        {88,  "UltraSPARC IIIi", "UltraSPARC IIIi"},

        {96,  "68040", "68040"},
        {97,  "68xxx Family", "68xxx"},
        {98,  "68000", "68000"},
        {99,  "68010", "68010"},
        {100, "68020", "68020"},
        {101, "68030", "68030"},

        {112, "Hobbit Family", "Hobbit"},

        {120, "Crusoe(TM) TM5000 Family", "Crusoe TM5000"},
        {121, "Crusoe(TM) TM3000 Family", "Crusoe TM3000"},
        {122, "Efficeon(TM) TM8000 Family", "Efficeon TM8000"},

        {128, "Weitek", "Weitek"},

        {130, "Itanium(TM) Processor", "Itanium"},
        {131, "AMD Athlon(TM) 64 Processor Family", "Athlon 64"},
        {132, "AMD Opteron(TM) Processor Family", "Opteron"},
        {133, "AMD Sempron(TM) Processor Family", "Sempron"},
        {134, "AMD Turion(TM) 64 Mobile Technology", "Turion 64"},
        {135, "Dual-Core AMD Opteron(TM) Processor Family",
                "Dual-Core Opteron"},
        {136, "AMD Athlon(TM) 64 X2 Dual-Core Processor Family",
                "Athlon 64 X2"},
        {137, "AMD Turion(TM) 64 X2 Mobile Technology", "Turion 64 X2"},
        {138, "Quad-Core AMD Opteron(TM) Processor Family",
                "Quad-Core Opteron"},
        {139, "Third-Generation AMD Opteron(TM) Processor Family",
                "Third-Generation Opteron"},
        {140, "AMD Phenom(TM) FX Quad-Core Processor Family", "Phenom FX"},
        {141, "AMD Phenom(TM) X4 Quad-Core Processor Family", "Phenom X4"},
        {142, "AMD Phenom(TM) X2 Dual-Core Processor Family", "Phenom X2"},
        {143, "AMD Athlon(TM) X2 Dual-Core Processor Family", "Athlon X2"},
        {144, "PA-RISC Family", "PA-RISC"},
        {145, "PA-RISC 8500", "PA-RISC 8500"},
        {146, "PA-RISC 8000", "PA-RISC 8000"},
        {147, "PA-RISC 7300LC", "PA-RISC 7300LC"},
        {148, "PA-RISC 7200", "PA-RISC 7200"},
        {149, "PA-RISC 7100LC", "PA-RISC 7100LC"},
        {150, "PA-RISC 7100", "PA-RISC 7100"},

        {161, "Quad-Core Intel(R) Xeon(R) processor 3200 Series",
                "Quad-Core Xeon 3200"},
        {162, "Dual-Core Intel(R) Xeon(R) processor 3000 Series",
                "Dual-Core Xeon 3000"},
        {163, "Quad-Core Intel(R) Xeon(R) processor 5300 Series",
                "Quad-Core Xeon 5300"},
        {164, "Dual-Core Intel(R) Xeon(R) processor 5100 Series",
                "Dual-Core Xeon 5100"},
        {165, "Dual-Core Intel(R) Xeon(R) processor 5000 Series",
                "Dual-Core Xeon 5000"},
        {166, "Dual-Core Intel(R) Xeon(R) processor LV", "Dual-Core Xeon LV"},
        {167, "Dual-Core Intel(R) Xeon(R) processor ULV",
                "Dual-Core Xeon ULV"},
        {168, "Dual-Core Intel(R) Xeon(R) processor 7100 Series",
                "Dual-Core Xeon 7100"},
        {169, "Quad-Core Intel(R) Xeon(R) processor 5400 Series",
                "Quad-Core Xeon 5400"},
        {170, "Quad-Core Intel(R) Xeon(R) processor", "Quad-Core Xeon"},
        {171, "Dual-Core Intel(R) Xeon(R) processor 5200 Series",
                "Dual-Core Xeon 5200"},
        {172, "Dual-Core Intel(R) Xeon(R) processor 7200 Series",
                "Dual-Core Xeon 7200"},
        {173, "Quad-Core Intel(R) Xeon(R) processor 7300 Series",
                "Quad-Core Xeon 7300"},
        {174, "Quad-Core Intel(R) Xeon(R) processor 7400 Series",
                "Quad-Core Xeon 7400"},
        {175, "Multi-Core Intel(R) Xeon(R) processor 7400 Series",
                "Multi-Core Xeon 7400"},
        {176, "Pentium(R) III Xeon(TM)", "Pentium III Xeon"},
        {177, "Pentium(R) III Processor with Intel(R) SpeedStep(TM) Technology",
                "Pentium III Speedstep"},
        {178, "Pentium(R) 4", "Pentium 4"},
        {179, "Intel(R) Xeon(TM)", "Xeon"},
        {180, "AS400 Family", "AS400"},
        {181, "Intel(R) Xeon(TM) processor MP", "Xeon MP"},
        {182, "AMD Athlon(TM) XP Family", "Athlon XP"},
        {183, "AMD Athlon(TM) MP Family", "Athlon MP"},
        {184, "Intel(R) Itanium(R) 2", "Itanium 2"},
        {185, "Intel(R) Pentium(R) M processor", "Pentium M"},
        {186, "Intel(R) Celeron(R) D processor", "Celeron D"},
        {187, "Intel(R) Pentium(R) D processor", "Pentium D"},
        {188, "Intel(R) Pentium(R) Processor Extreme Edition", "Pentium EE"},
        {189, "Intel(R) Core(TM) Solo Processor", "Core Solo"},
        {190, "K7", "K7"},
        {191, "Intel(R) Core(TM)2 Duo Processor", "Core 2 Duo"},
        {192, "Intel(R) Core(TM)2 Solo processor", "Core 2 Solo"},
        {193, "Intel(R) Core(TM)2 Extreme processor", "Core 2 Extreme"},
        {194, "Intel(R) Core(TM)2 Quad processor", "Core 2 Quad"},
        {195, "Intel(R) Core(TM)2 Extreme mobile processor",
                "Core 2 Extreme Mobile"},
        {196, "Intel(R) Core(TM)2 Duo mobile processor", "Core 2 Duo Mobile"},
        {197, "Intel(R) Core(TM)2 Solo mobile processor",
                "Core 2 Solo Mobile"},
        {198, "Intel(R) Core(TM) i7 processor", "Core i7"},
        {199, "Dual-Core Intel(R) Celeron(R) Processor", "Dual-Core Celeron"},
        {200, "S/390 and zSeries Family", "IBM390"},
        {201, "ESA/390 G4", "G4"},
        {202, "ESA/390 G5", "G5"},
        {203, "ESA/390 G6", "ESA/390 G6"},
        {204, "z/Architectur base", "z/Architectur"}, /* this is not typo */
        {205, "Intel(R) Core(TM) i5 processor", "Core i5"},
        {206, "Intel(R) Core(TM) i3 processor", "Core i3"},

        {210, "VIA C7(TM)-M Processor Family", "C7-M"},
        {211, "VIA C7(TM)-D Processor Family", "C7-D"},
        {212, "VIA C7(TM) Processor Family", "C7"},
        {213, "VIA Eden(TM) Processor Family", "Eden"},
        {214, "Multi-Core Intel(R) Xeon(R) processor", "Multi-Core Xeon"},
        {215, "Dual-Core Intel(R) Xeon(R) processor 3xxx Series",
                "Dual-Core Xeon 3xxx"},
        {216, "Quad-Core Intel(R) Xeon(R) processor 3xxx Series",
                "Quad-Core Xeon 3xxx"},
        {217, "VIA Nano(TM) Processor Family", "Nano"},
        {218, "Dual-Core Intel(R) Xeon(R) processor 5xxx Series",
                "Dual-Core Xeon 5xxx"},
        {219, "Quad-Core Intel(R) Xeon(R) processor 5xxx Series",
                "Quad-Core Xeon 5xxx"},

        {221, "Dual-Core Intel(R) Xeon(R) processor 7xxx Series",
                "Dual-Core Xeon 7xxx"},
        {222, "Quad-Core Intel(R) Xeon(R) processor 7xxx Series",
                "Quad-Core Xeon 7xxx"},
        {223, "Multi-Core Intel(R) Xeon(R) processor 7xxx Series",
                "Multi-Core Xeon 7xxx"},
        {224, "Multi-Core Intel(R) Xeon(R) processor 3400 Series",
                "Multi-Core Xeon 3400"},

        /*
        {228, "AMD Opteron(TM) 3000 Series Processor", ""},
        {229, "AMD Sempron(TM) II Processor Family", ""},
        */
        {230, "Embedded AMD Opteron(TM) Quad-Core Processor Family",
                "Embedded Opteron Quad-Core"},
        {231, "AMD Phenom(TM) Triple-Core Processor Family",
                "Phenom Triple-Core"},
        {232, "AMD Turion(TM) Ultra Dual-Core Mobile Processor Family",
                "Turion Ultra Dual-Core Mobile"},
        {233, "AMD Turion(TM) Dual-Core Mobile Processor Family",
                "Turion Dual-Core Mobile"},
        {234, "AMD Athlon(TM) Dual-Core Processor Family", "Athlon Dual-Core"},
        {235, "AMD Sempron(TM) SI Processor Family", "Sempron SI"},
        {236, "AMD Phenom(TM) II Processor Family", "Phenom II"},
        {237, "AMD Athlon(TM) II Processor Family", "Athlon II"},
        {238, "Six-Core AMD Opteron(TM) Processor Family", "Six-Core Opteron"},
        {239, "AMD Sempron(TM) M Processor Family", "Sempron M"},

        {250, "i860", "i860"},
        {251, "i960", "i960"},

        {260, "SH-3", "SH-3"},
        {261, "SH-4", "SH-4"},
        {280, "ARM", "ARM"},
        {281, "StrongARM", "StrongARM"},
        {300, "6x86", "6x86"},
        {301, "MediaGX", "MediaGX"},
        {302, "MII", "MII"},
        {320, "WinChip", "WinChip"},
        {350, "DSP", "DSP"},
        {500, "Video Processor", "Video Processor"},

        /*
        {254, "Reserved (SMBIOS Extension)", ""},
        {255, "Reserved (Un-initialized Flash Content - Lo)", ""},

        {65534, "Reserved (For Future Special Purpose Assignment)", ""},
        {65535, "Reserved (Un-initialized Flash Content - Hi)", ""},
        */
    };

    size_t i, fm_length = sizeof(fm) / sizeof(fm[0]);

    for (i = 0; i < fm_length; i++) {
        if (strcmp(dmi_family, fm[i].search) == 0) {
            return fm[i].value_map;
        }
    }

    return 1; /* Other */
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_Processor",
    "LMI_Processor",
    "instance method")
