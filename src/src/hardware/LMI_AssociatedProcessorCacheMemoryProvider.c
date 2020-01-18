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
#include "LMI_AssociatedProcessorCacheMemory.h"
#include "LMI_Processor.h"
#include "LMI_Hardware.h"
#include "globals.h"
#include "dmidecode.h"
#include "lscpu.h"
#include "sysfs.h"

CMPIUint16 get_cache_level(const unsigned level);
CMPIUint16 get_write_policy(const char *op_mode);
CMPIUint16 get_cache_type(const char *type);
CMPIUint16 get_cache_associativity_dmi(const char *assoc);
CMPIUint16 get_cache_associativity_sysfs(const unsigned ways_of_assoc);

static const CMPIBroker* _cb;

static void LMI_AssociatedProcessorCacheMemoryInitialize(const CMPIContext *ctx)
{
    lmi_init(provider_name, _cb, ctx, provider_config_defaults);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    LMI_AssociatedProcessorCacheMemory lmi_assoc_cache;
    LMI_ProcessorCacheMemoryRef lmi_cpu_cache;
    LMI_ProcessorRef lmi_cpu;
    CMPIUint16 cache_level, write_policy, cache_type, associativity;
    const char *ns = KNameSpace(cop);
    char *error_msg = NULL;
    unsigned i, j, cpus_nb = 0;
    DmiProcessor *dmi_cpus = NULL;
    unsigned dmi_cpus_nb = 0;
    LscpuProcessor lscpu;
    DmiCpuCache *dmi_cpu_caches = NULL;
    unsigned dmi_cpu_caches_nb = 0;
    SysfsCpuCache *sysfs_cpu_caches = NULL;
    unsigned sysfs_cpu_caches_nb = 0;

    /* get processors and caches */
    if (dmi_get_processors(&dmi_cpus, &dmi_cpus_nb) != 0 || dmi_cpus_nb < 1) {
        dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);
    }

    if (dmi_get_cpu_caches(&dmi_cpu_caches, &dmi_cpu_caches_nb) != 0
            || dmi_cpu_caches_nb < 1) {
        dmi_free_cpu_caches(&dmi_cpu_caches, &dmi_cpu_caches_nb);
    }

    /* without dmi cpus, we can't use dmi cache */
    if (dmi_cpus_nb < 1) {
        dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);
        dmi_free_cpu_caches(&dmi_cpu_caches, &dmi_cpu_caches_nb);

        if (lscpu_get_processor(&lscpu) != 0) {
            error_msg = "Unable to get processor information.";
            goto done;
        }

        if (sysfs_get_cpu_caches(&sysfs_cpu_caches, &sysfs_cpu_caches_nb) != 0
                || sysfs_cpu_caches_nb < 1) {
            error_msg = "Unable to get processor cache information.";
            goto done;
        }
    } else if (dmi_cpu_caches_nb < 1) {
        /* but we can do OK with dmi cpu and no dmi cache */
        dmi_free_cpu_caches(&dmi_cpu_caches, &dmi_cpu_caches_nb);

        if (sysfs_get_cpu_caches(&sysfs_cpu_caches, &sysfs_cpu_caches_nb) != 0
                || sysfs_cpu_caches_nb < 1) {
            error_msg = "Unable to get processor cache information.";
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

    /* if we have cpus and caches from dmidecode; */
    /* in this case, we can match exactly cpus and caches */
    if (dmi_cpus_nb > 0 && dmi_cpu_caches_nb > 0) {
        /* loop cpus */
        for (i = 0; i < dmi_cpus_nb; i++) {
            LMI_AssociatedProcessorCacheMemory_Init(&lmi_assoc_cache, _cb, ns);

            LMI_ProcessorRef_Init(&lmi_cpu, _cb, ns);
            LMI_ProcessorRef_Set_SystemCreationClassName(&lmi_cpu,
                    get_system_creation_class_name());
            LMI_ProcessorRef_Set_SystemName(&lmi_cpu, get_system_name());
            LMI_ProcessorRef_Set_CreationClassName(&lmi_cpu,
                    ORGID "_" CPU_CLASS_NAME);
            LMI_ProcessorRef_Set_DeviceID(&lmi_cpu, dmi_cpus[i].id);

            /* loop caches */
            for (j = 0; j < dmi_cpu_caches_nb; j++) {
                /* if this cpu contains this cache */
                if (strcmp(dmi_cpu_caches[j].id,dmi_cpus[i].l1_cache_handle) == 0
                        || strcmp(dmi_cpu_caches[j].id, dmi_cpus[i].l2_cache_handle) == 0
                        || strcmp(dmi_cpu_caches[j].id, dmi_cpus[i].l3_cache_handle) == 0) {
                    LMI_AssociatedProcessorCacheMemory_Init(
                            &lmi_assoc_cache, _cb, ns);

                    LMI_ProcessorCacheMemoryRef_Init(&lmi_cpu_cache, _cb, ns);
                    LMI_ProcessorCacheMemoryRef_Set_SystemCreationClassName(
                            &lmi_cpu_cache, get_system_creation_class_name());
                    LMI_ProcessorCacheMemoryRef_Set_SystemName(&lmi_cpu_cache,
                            get_system_name());
                    LMI_ProcessorCacheMemoryRef_Set_CreationClassName(
                            &lmi_cpu_cache, ORGID "_" CPU_CACHE_CLASS_NAME);
                    LMI_ProcessorCacheMemoryRef_Set_DeviceID(
                            &lmi_cpu_cache, dmi_cpu_caches[j].id);

                    LMI_AssociatedProcessorCacheMemory_Set_Dependent(
                            &lmi_assoc_cache, &lmi_cpu);
                    LMI_AssociatedProcessorCacheMemory_Set_Antecedent(
                            &lmi_assoc_cache, &lmi_cpu_cache);

                    cache_level = get_cache_level(dmi_cpu_caches[j].level);
                    if (cache_level == LMI_AssociatedProcessorCacheMemory_Level_Other) {
                        char other_level[LONG_INT_LEN];
                        snprintf(other_level, LONG_INT_LEN, "%u",
                                dmi_cpu_caches[j].level);
                        LMI_AssociatedProcessorCacheMemory_Set_OtherLevelDescription(
                                &lmi_assoc_cache, other_level);
                    }
                    write_policy = get_write_policy(dmi_cpu_caches[j].op_mode);
                    if (write_policy == LMI_AssociatedProcessorCacheMemory_WritePolicy_Other) {
                        LMI_AssociatedProcessorCacheMemory_Set_OtherWritePolicyDescription(
                                &lmi_assoc_cache, dmi_cpu_caches[j].op_mode);
                    }
                    cache_type = get_cache_type(dmi_cpu_caches[j].type);
                    if (cache_type == LMI_AssociatedProcessorCacheMemory_CacheType_Other) {
                        LMI_AssociatedProcessorCacheMemory_Set_OtherCacheTypeDescription(
                                &lmi_assoc_cache, dmi_cpu_caches[j].type);
                    }
                    associativity = get_cache_associativity_dmi(
                            dmi_cpu_caches[j].associativity);
                    if (associativity == LMI_AssociatedProcessorCacheMemory_Associativity_Other) {
                        LMI_AssociatedProcessorCacheMemory_Set_OtherAssociativityDescription(
                                &lmi_assoc_cache,
                                dmi_cpu_caches[j].associativity);
                    }

                    LMI_AssociatedProcessorCacheMemory_Set_Level(
                            &lmi_assoc_cache, cache_level);
                    LMI_AssociatedProcessorCacheMemory_Set_WritePolicy(
                            &lmi_assoc_cache, write_policy);
                    LMI_AssociatedProcessorCacheMemory_Set_CacheType(
                            &lmi_assoc_cache, cache_type);
                    LMI_AssociatedProcessorCacheMemory_Set_Associativity(
                            &lmi_assoc_cache, associativity);

                    LMI_AssociatedProcessorCacheMemory_Set_ReadPolicy(
                            &lmi_assoc_cache,
                            LMI_AssociatedProcessorCacheMemory_ReadPolicy_Unknown);

                    KReturnInstance(cr, lmi_assoc_cache);
                }
            }
        }
    } else {
        /* in this case, we have all caches for every cpu */
        /* loop caches */
        for (i = 0; i < sysfs_cpu_caches_nb; i++) {
            LMI_AssociatedProcessorCacheMemory_Init(&lmi_assoc_cache, _cb, ns);

            LMI_ProcessorCacheMemoryRef_Init(&lmi_cpu_cache, _cb, ns);
            LMI_ProcessorCacheMemoryRef_Set_SystemCreationClassName(
                    &lmi_cpu_cache, get_system_creation_class_name());
            LMI_ProcessorCacheMemoryRef_Set_SystemName(&lmi_cpu_cache,
                    get_system_name());
            LMI_ProcessorCacheMemoryRef_Set_CreationClassName(&lmi_cpu_cache,
                    ORGID "_" CPU_CACHE_CLASS_NAME);
            LMI_ProcessorCacheMemoryRef_Set_DeviceID(&lmi_cpu_cache,
                    sysfs_cpu_caches[i].id);

            /* determine the right CPU index */
            j = i / (sysfs_cpu_caches_nb / cpus_nb);

            LMI_ProcessorRef_Init(&lmi_cpu, _cb, ns);
            LMI_ProcessorRef_Set_SystemCreationClassName(&lmi_cpu,
                    get_system_creation_class_name());
            LMI_ProcessorRef_Set_SystemName(&lmi_cpu, get_system_name());
            LMI_ProcessorRef_Set_CreationClassName(&lmi_cpu,
                    ORGID "_" CPU_CLASS_NAME);

            if (dmi_cpus_nb > 0) {
                LMI_ProcessorRef_Set_DeviceID(&lmi_cpu, dmi_cpus[j].id);
            } else {
                char cpu_id[LONG_INT_LEN] = "";
                snprintf(cpu_id, LONG_INT_LEN, "%u", j);
                LMI_ProcessorRef_Set_DeviceID(&lmi_cpu, cpu_id);
            }

            LMI_AssociatedProcessorCacheMemory_Set_Dependent(
                    &lmi_assoc_cache, &lmi_cpu);
            LMI_AssociatedProcessorCacheMemory_Set_Antecedent(
                    &lmi_assoc_cache, &lmi_cpu_cache);

            cache_level = get_cache_level(sysfs_cpu_caches[i].level);
            if (cache_level == LMI_AssociatedProcessorCacheMemory_Level_Other) {
                char other_level[LONG_INT_LEN];
                snprintf(other_level, LONG_INT_LEN, "%u",
                        sysfs_cpu_caches[i].level);
                LMI_AssociatedProcessorCacheMemory_Set_OtherLevelDescription(
                        &lmi_assoc_cache, other_level);
            }
            cache_type = get_cache_type(sysfs_cpu_caches[i].type);
            if (cache_type == LMI_AssociatedProcessorCacheMemory_CacheType_Other) {
                LMI_AssociatedProcessorCacheMemory_Set_OtherCacheTypeDescription(
                        &lmi_assoc_cache, sysfs_cpu_caches[i].type);
            }

            LMI_AssociatedProcessorCacheMemory_Set_Level(
                    &lmi_assoc_cache, cache_level);
            LMI_AssociatedProcessorCacheMemory_Set_CacheType(
                    &lmi_assoc_cache, cache_type);
            LMI_AssociatedProcessorCacheMemory_Set_Associativity(
                    &lmi_assoc_cache,
                    get_cache_associativity_sysfs(
                            sysfs_cpu_caches[i].ways_of_assoc));

            LMI_AssociatedProcessorCacheMemory_Set_WritePolicy(
                    &lmi_assoc_cache,
                    LMI_AssociatedProcessorCacheMemory_WritePolicy_Unknown);
            LMI_AssociatedProcessorCacheMemory_Set_ReadPolicy(
                    &lmi_assoc_cache,
                    LMI_AssociatedProcessorCacheMemory_ReadPolicy_Unknown);

            if (sysfs_cpu_caches[i].line_size) {
                LMI_AssociatedProcessorCacheMemory_Set_LineSize(
                        &lmi_assoc_cache, sysfs_cpu_caches[i].line_size);
            }

            KReturnInstance(cr, lmi_assoc_cache);
        }
    }

done:
    /* free lscpu only if it was used */
    if (dmi_cpus_nb < 1) {
        lscpu_free_processor(&lscpu);
    }
    dmi_free_processors(&dmi_cpus, &dmi_cpus_nb);
    dmi_free_cpu_caches(&dmi_cpu_caches, &dmi_cpu_caches_nb);
    sysfs_free_cpu_caches(&sysfs_cpu_caches, &sysfs_cpu_caches_nb);

    if (error_msg) {
        KReturn2(_cb, ERR_FAILED, "%s", error_msg);
    }

    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryAssociators(
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
        LMI_AssociatedProcessorCacheMemory_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryAssociatorNames(
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
        LMI_AssociatedProcessorCacheMemory_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryReferences(
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
        LMI_AssociatedProcessorCacheMemory_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_AssociatedProcessorCacheMemoryReferenceNames(
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
        LMI_AssociatedProcessorCacheMemory_ClassName,
        assocClass,
        role);
}

/*
 * Get CIM Cache Level.
 * @param level
 * @return CIM Cache Level
 */
CMPIUint16 get_cache_level(const unsigned level)
{
    static struct {
        CMPIUint16 cim_level;       /* CIM cache level */
        unsigned level;             /* cache level */
    } levels[] = {
        {0, 0},
        /*
        {1, },
        {2, },
        */
        {3, 1},
        {4, 2},
        {5, 3},
    };

    size_t i, lvl_length = sizeof(levels) / sizeof(levels[0]);

    for (i = 0; i < lvl_length; i++) {
        if (level == levels[i].level) {
            return levels[i].cim_level;
        }
    }

    return 1; /* Other */
}

/*
 * Get CIM Write Policy according to dmidecode.
 * @param op_mode operational mode from dmidecode
 * @return CIM Write Policy
 */
CMPIUint16 get_write_policy(const char *op_mode)
{
    static struct {
        CMPIUint16 write_policy;        /* CIM write policy */
        char *op_mode;                  /* op mode from dmidecode */
    } modes[] = {
        {0, "Unknown"},
        /*
        {1, },
        */
        {2, "Write Back"},
        {3, "Write Through"},
        {4, "Varies With Memory Address"},
        /*
        {5, },
        */
    };

    size_t i, mode_length = sizeof(modes) / sizeof(modes[0]);

    for (i = 0; i < mode_length; i++) {
        if (strcmp(op_mode, modes[i].op_mode) == 0) {
            return modes[i].write_policy;
        }
    }

    return 1; /* Other */
}

/*
 * Get CIM Cache Type according to dmidecode and sysfs.
 * @param type cache type from dmidecode and sysfs
 * @return CIM Cache Type
 */
CMPIUint16 get_cache_type(const char *type)
{
    static struct {
        CMPIUint16 cache_type;      /* CIM cache type */
        char *type;                 /* type from dmidecode and sysfs */
    } types[] = {
        {0, "Unknown"},
        {1, "Other"},
        {2, "Instruction"},
        {3, "Data"},
        {4, "Unified"},
    };

    size_t i, types_length = sizeof(types) / sizeof(types[0]);

    for (i = 0; i < types_length; i++) {
        if (strcmp(type, types[i].type) == 0) {
            return types[i].cache_type;
        }
    }

    return 1; /* Other */
}

/*
 * Get CIM Cache Associativity according to dmidecode.
 * @param assoc of cache from dmidecode
 * @return CIM Cache Associativity
 */
CMPIUint16 get_cache_associativity_dmi(const char *assoc)
{
    static struct {
        CMPIUint16 cache_assoc;      /* CIM cache associativity */
        char *assoc;                 /* associativity from dmidecode */
    } assocs[] = {
        {0,  "Unknown"},
        {1,  "Other"},
        {2,  "Direct Mapped"},
        {3,  "2-way Set-associative"},
        {4,  "4-way Set-associative"},
        {5,  "Fully Associative"},
        {6,  "8-way Set-associative"},
        {7,  "16-way Set-associative"},
        {8,  "12-way Set-associative"},
        {9,  "24-way Set-associative"},
        {10, "32-way Set-associative"},
        {11, "48-way Set-associative"},
        {12, "64-way Set-associative"},
        {13, "20-way Set-associative"},
    };

    size_t i, assocs_length = sizeof(assocs) / sizeof(assocs[0]);

    for (i = 0; i < assocs_length; i++) {
        if (strcmp(assoc, assocs[i].assoc) == 0) {
            return assocs[i].cache_assoc;
        }
    }

    return 1; /* Other */
}

/*
 * Get CIM Cache Associativity according to sysfs.
 * @param ways_of_assoc from sysfs
 * @return CIM Cache Associativity
 */
CMPIUint16 get_cache_associativity_sysfs(const unsigned ways_of_assoc)
{
    static struct {
        CMPIUint16 cache_assoc;     /* CIM cache associativity */
        unsigned ways;              /* ways of associativity from sysfs */
    } assocs[] = {
        {0,  0},
        /*
        {1,  "Other"},
        {2,  "Direct Mapped"},
        */
        {3,  2},
        {4,  4},
        /*
        {5,  "Fully Associative"},
        */
        {6,  8},
        {7,  16},
        {8,  12},
        {9,  24},
        {10, 32},
        {11, 48},
        {12, 64},
        {13, 20},
    };

    size_t i, assocs_length = sizeof(assocs) / sizeof(assocs[0]);

    for (i = 0; i < assocs_length; i++) {
        if (ways_of_assoc == assocs[i].ways) {
            return assocs[i].cache_assoc;
        }
    }

    return 1; /* Other */
}

CMInstanceMIStub( 
    LMI_AssociatedProcessorCacheMemory,
    LMI_AssociatedProcessorCacheMemory,
    _cb,
    LMI_AssociatedProcessorCacheMemoryInitialize(ctx))

CMAssociationMIStub( 
    LMI_AssociatedProcessorCacheMemory,
    LMI_AssociatedProcessorCacheMemory,
    _cb,
    LMI_AssociatedProcessorCacheMemoryInitialize(ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AssociatedProcessorCacheMemory",
    "LMI_AssociatedProcessorCacheMemory",
    "instance association")
