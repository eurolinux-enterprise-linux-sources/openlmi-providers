/*
 * Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
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
 * Authors: Michal Minar <miminar@redhat.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <sensors/sensors.h>
#include <sensors/error.h>

#include "fan.h"

const char *provider_name = "fan";
const ConfigEntry *provider_config_defaults = NULL;

//* constants *****************************************************************
static bool MODULE_INITIALIZED = false;

static char const * const _fan_errors[] = {
    "success",
    "failed to get fan by identification supplied",
    "given identification data is not enough to identify uniquely",
    "no chip with given system path found",
    "no fan with given name and chip found",
    "unknown property",
    "this property is not supported for setting",
    "property is not writable",
    "input/output error",
    "initialization failed",
    "unknown error occured"
};

//* static functions **********************************************************
static cim_fan_error_t prop_type2sf_num(
        cim_fan_prop_type_t pt,
        int *sf_num)
{
    switch (pt) {
        case CIM_FAN_MIN_SPEED: *sf_num = SENSORS_SUBFEATURE_FAN_MIN;    break;
        case CIM_FAN_MAX_SPEED:
#ifdef SENSORS_SUBFEATURE_FAN_MAX
            *sf_num = SENSORS_SUBFEATURE_FAN_MAX; break;
#else
            return CIM_FAN_NOT_SUPPORTED_PROP;
#endif
        case CIM_FAN_DIV:       *sf_num = SENSORS_SUBFEATURE_FAN_DIV;    break;
        case CIM_FAN_PULSES:    *sf_num = SENSORS_SUBFEATURE_FAN_PULSES; break;
        case CIM_FAN_BEEP:      *sf_num = SENSORS_SUBFEATURE_FAN_BEEP;   break;
        default:                return CIM_FAN_UNKNOWN_PROP; break;
    }
    return CIM_FAN_SUCCESS;
}

static double prop_val2double(
        cim_fan_prop_type_t pt,
        cim_fan_prop_value_t const *val) {
    switch (pt) {
        case CIM_FAN_MIN_SPEED: return val->min_speed;
        case CIM_FAN_MAX_SPEED: return val->max_speed;
        case CIM_FAN_DIV:       return val->divisor;
        case CIM_FAN_PULSES:    return val->pulses;
        case CIM_FAN_BEEP:      return val->beep;
    }
    return 0;
}

static cim_fan_error_t libsensors2cim_fan_error(int err) {
    if (!err) return CIM_FAN_SUCCESS;
    if (err < 0) err = -err;
    if (err < SENSORS_ERR_WILDCARDS || err > SENSORS_ERR_RECURSION) {
        return CIM_FAN_OTHER_ERROR;
    }
    return (cim_fan_error_t) (0x100 + err);
}

static const char * sprintf_chip_name(sensors_chip_name const *name) {
    /** @return NULL on error, cstring otherwise, it needs to be freed */
    int charcnt;
    char buf[BUFLEN];
    if ((charcnt = sensors_snprintf_chip_name(buf, BUFLEN, name)) < 0)
        return NULL;
    return strndup(buf, charcnt);
}

static cim_fan_error_t reload_config_file(char const * fp) {
    /**
     * @param fp if NULL -> default configuration file will be loaded
     * @return 0 on success */
    FILE *config_file = NULL;
    int err;
    char errbuf[BUFLEN];

    if (fp) {
        if (!(config_file = fopen(fp, "r"))) {
            lmi_error("Cound not open config file \"%s\": %s\n",
                      fp, strerror_r(errno, errbuf, sizeof(errbuf)));
        }
    }
    err = sensors_init(config_file);
    if (err) {
        lmi_error("sensors_init: %s\n", sensors_strerror(err));
        if (config_file) fclose(config_file);
    }else {
        if (config_file) fclose(config_file);
    }
    return libsensors2cim_fan_error(err);
}

static double _get_value( sensors_chip_name const *chip
                        , sensors_subfeature const *sf)
{
    double val;
    if (sensors_get_value(chip, sf->number, &val)) return 0.;
    return val;
}

static int _assign_sf_value(
        sensors_chip_name const *chip,
        sensors_feature const *feature,
        int subfeature_type,
        struct cim_fan *fdest)
{
    sensors_subfeature const *sf;
    double val;

    sf = sensors_get_subfeature(chip, feature, subfeature_type);
    val = sf ? _get_value(chip, sf):0.;
    switch (subfeature_type) {
        case SENSORS_SUBFEATURE_FAN_INPUT: fdest->speed = val; break;
        case SENSORS_SUBFEATURE_FAN_MIN: fdest->min_speed = val; break;
#ifdef SENSORS_SUBFEATURE_FAN_MAX
        case SENSORS_SUBFEATURE_FAN_MAX: fdest->max_speed = val; break;
#endif
        case SENSORS_SUBFEATURE_FAN_DIV: fdest->divisor = val; break;
        case SENSORS_SUBFEATURE_FAN_PULSES: fdest->pulses = val; break;
        case SENSORS_SUBFEATURE_FAN_FAULT: fdest->fault = val != 0; break;
        case SENSORS_SUBFEATURE_FAN_BEEP: fdest->beep = val != 0; break;
        case SENSORS_SUBFEATURE_FAN_ALARM: fdest->alarm = val != 0; break;
#ifdef SENSORS_SUBFEATURE_ALARM_MIN
        case SENSORS_SUBFEATURE_FAN_ALARM_MIN:
            fdest->alarm_min = val != 0;
            break;
#endif
#ifdef SENSORS_SUBFEATURE_FAN_ALARM_MAX
        case SENSORS_SUBFEATURE_FAN_ALARM_MAX:
            fdest->alarm_max = val != 0;
            break;
#endif
        default: return 1;
    }
    return !sf;
}

static unsigned int _load_accessible_features(
        sensors_chip_name const *chip,
        sensors_feature const *feature)
{
    sensors_subfeature const *sf;
    unsigned int res = 0;
    unsigned int i = 1;
    int sf_type;
    while(i <= CIM_FAN_AF_FEATURE_MAX) {
        switch (i) {
            case CIM_FAN_AF_MIN_SPEED:
                sf_type = SENSORS_SUBFEATURE_FAN_MIN; break;
#ifdef SENSORS_SUBFEATURE_FAN_MAX
            case CIM_FAN_AF_MAX_SPEED:
                sf_type = SENSORS_SUBFEATURE_FAN_MAX; break;
#endif
            case CIM_FAN_AF_DIV:
                sf_type = SENSORS_SUBFEATURE_FAN_DIV; break;
            case CIM_FAN_AF_PULSES:
                sf_type = SENSORS_SUBFEATURE_FAN_PULSES; break;
            case CIM_FAN_AF_FAULT:
                sf_type = SENSORS_SUBFEATURE_FAN_FAULT; break;
            case CIM_FAN_AF_BEEP:
                sf_type = SENSORS_SUBFEATURE_FAN_BEEP; break;
            case CIM_FAN_AF_ALARM:
                sf_type = SENSORS_SUBFEATURE_FAN_ALARM; break;
#ifdef SENSORS_SUBFEATURE_FAN_ALARM_MIN
            case CIM_FAN_AF_ALARM_MIN:
                sf_type = SENSORS_SUBFEATURE_ALARM_MIN: break;
#endif
#ifdef SENSORS_SUBFEATURE_ALARM_MAX
            case CIM_FAN_AF_ALARM_MAX:
                sf_type = SENSORS_SUBFEATURE_ALARM_MAX; break;
#endif
            default:
                sf_type = -1; break;
        }
        if (sf_type != -1) {
            sf = sensors_get_subfeature(chip, feature, sf_type);
            res |= sf ? i : 0;
        }
        i = i << 1;
    }
    return res;
}


static struct cim_fan * _load_fan_data( sensors_chip_name const *chip
                                      , sensors_feature const *feature)
{
    struct cim_fan *f;
    char * tmp;
    int length;
    int const sfs[] = {
        SENSORS_SUBFEATURE_FAN_INPUT,
        SENSORS_SUBFEATURE_FAN_MIN,
#ifdef SENSORS_SUBFEATURE_FAN_MAX
        SENSORS_SUBFEATURE_FAN_MAX,
#endif
        SENSORS_SUBFEATURE_FAN_DIV,
        SENSORS_SUBFEATURE_FAN_PULSES,
        SENSORS_SUBFEATURE_FAN_FAULT,
        SENSORS_SUBFEATURE_FAN_BEEP,
        SENSORS_SUBFEATURE_FAN_ALARM,
#ifdef SENSORS_SUBFEATURE_FAN_MIN_ALARM
        SENSORS_SUBFEATURE_FAN_MIN_ALARM,
#endif
#ifdef SENSORS_SUBFEATURE_FAN_MAX_ALARM
        SENSORS_SUBFEATURE_FAN_MAX_ALARM,
#endif
        -1, // terminator
    };


    if (!(f = calloc(1, sizeof(struct cim_fan)))) {
        return NULL;
    }
    if (!(f->chip_name = sprintf_chip_name(chip))) {
        lmi_error("could not get chip name\n");
        goto lab_err_free_fan;
    }
    f->sys_path = chip->path;
    if (!(f->name = sensors_get_label(chip, feature))) {
        lmi_error("could not get fan name for chip: %s\n",
                f->chip_name);
        goto lab_err_chip_name;
    }
    length = strlen(f->sys_path);
    if (!(f->device_id = malloc((length + strlen(f->name) + 2)
                    * sizeof(char)))) {
        perror("malloc");
        goto lab_err_fan_name;
    }
    tmp = strcpy((char*) f->device_id, f->sys_path);
    tmp += length * sizeof(char);
    if (f->sys_path[length - 1] != '/') {
        //this will be almost certainly true
        *(tmp++) = '/';
    }
    strcpy(tmp, f->name);

    for (int const *sf_ptr=sfs; *sf_ptr != -1; ++sf_ptr) {
        _assign_sf_value(chip, feature, *sf_ptr, f);
    }
    f->accessible_features = _load_accessible_features(chip, feature);
    return f;

lab_err_fan_name:
    free((char*) f->name);
lab_err_chip_name:
    free((char*) f->chip_name);
lab_err_free_fan:
    free(f);
    return NULL;
}

static cim_fan_error_t _find_fan(
        char const *sys_path,
        char const *fan_name,
        sensors_chip_name const **chip,
        sensors_feature const **feature)
{
    int chip_nr = 0, feature_nr = 0;
    char * sp = (char*) sys_path;
    char const *label;
    int length;
    cim_fan_error_t ret = CIM_FAN_OTHER_ERROR;

    if (!sys_path || !fan_name) return ret;

    length = strlen(sys_path);
    if (length && sys_path[length - 1] == '/') {
        if (!(sp = strdup(sys_path))) {
            perror("strdup");
            return ret;
        }
        sp[length - 1] = '\0';
    }

    while ((*chip = sensors_get_detected_chips(NULL, &chip_nr))) {
        if ((*chip)->path && !strcmp((*chip)->path, sp)) break;
    }
    if (sp != sys_path) free(sp);
    if (!(*chip)) {
        ret = CIM_FAN_NO_SUCH_CHIP;
        return ret;
    }
    while ((*feature = sensors_get_features(*chip, &feature_nr))) {
        if ((*feature)->type != SENSORS_FEATURE_FAN) continue;
        if (!(label = sensors_get_label(*chip, *feature))) {
            continue;
        }
        if (!strcmp(label, fan_name)) {
            free((char*) label);
            break;
        }
        free((char*) label);
    }
    ret =  (*feature) ? CIM_FAN_SUCCESS : CIM_FAN_NO_SUCH_FAN_NAME;
    return ret;
}

static cim_fan_error_t _find_fan_by_id(
        char const *device_id,
        sensors_chip_name const **chip,
        sensors_feature const **feature)
{
    char *sys_path;
    char *fan_name;
    char *tmp1, *tmp2;
    cim_fan_error_t ret;

    if (!(tmp1 = strdup(device_id))) {
        perror("strdup");
        return CIM_FAN_OTHER_ERROR;
    }
    if (!(tmp2 = strdup(device_id))) {
        perror("strdup");
        free(tmp1);
        return CIM_FAN_OTHER_ERROR;
    }
    sys_path = dirname(tmp1);
    fan_name = basename(tmp2);
    ret = _find_fan(sys_path, fan_name, chip, feature);
    free(tmp1);
    free(tmp2);

    return ret;
}

static cim_fan_error_t _set_fan_prop(
        sensors_chip_name const *chip,
        cim_fan_prop_type_t prop_type,
        cim_fan_prop_value_t const *prop)
{
    int sf_num;
    cim_fan_error_t err;
    double val;

    if (!(err = prop_type2sf_num(prop_type, &sf_num))) {
        val = prop_val2double(prop_type, prop);
        err = libsensors2cim_fan_error(
                    sensors_set_value(chip, sf_num, val));
    }
    return err;
}

//* module interface **********************************************************
char const * cim_fan_strerror(cim_fan_error_t error) {
    if (  ((int) error) >  0x100
       && ((int) error) <= 0x100 + SENSORS_ERR_RECURSION) {
        return sensors_strerror(((int) error) - 0x100);
    }
    if (  ((int) error) < CIM_FAN_SUCCESS
       || ((int) error) > CIM_FAN_OTHER_ERROR) {
        return "unknown error code";
    }
    return _fan_errors[(int) error];
}

cim_fan_error_t enum_all_fans(struct fanlist **lptr) {
    struct fanlist * lptrhelp = NULL;
    sensors_chip_name const *chip;
    sensors_feature const *feature;
    int chip_nr = 0, feature_nr;

    if (!(lptrhelp = calloc(1, sizeof(struct fanlist)))) {
        perror("calloc");
        return CIM_FAN_OTHER_ERROR;
    }
    *lptr = lptrhelp;
    while ((chip = sensors_get_detected_chips(NULL, &chip_nr))) {
        feature_nr = 0;
        while ((feature = sensors_get_features(chip, &feature_nr))) {
            if (feature->type != SENSORS_FEATURE_FAN) continue;
            if (lptrhelp->f) {
                if (!(lptrhelp->next = calloc(1, sizeof(struct fanlist)))) {
                    free_fanlist(*lptr);
                    perror("calloc");
                    return CIM_FAN_OTHER_ERROR;
                }
                lptrhelp = lptrhelp->next;
            }
            if (!(lptrhelp->f = _load_fan_data(chip, feature))) {
                free_fanlist(*lptr);
                return CIM_FAN_OTHER_ERROR;
            }
        }
    }
    if (!(*lptr)->f) {
        free_fanlist(*lptr);
        *lptr = NULL;
    }
    return CIM_FAN_SUCCESS;
}

cim_fan_error_t get_fan_data_by_id(
        char const *device_id,
        struct cim_fan **sptr)
{
    sensors_chip_name const *chip;
    sensors_feature const *feature;
    cim_fan_error_t ret;

    if ((ret = _find_fan_by_id(device_id, &chip, &feature))
            == CIM_FAN_SUCCESS) {
        if (!(*sptr = _load_fan_data(chip, feature))) {
            ret = CIM_FAN_OTHER_ERROR;
        }
    }
    return ret;
}

cim_fan_error_t get_fan_data(
        char const *sys_path,
        char const *fan_name,
        struct cim_fan **sptr)
{
    sensors_chip_name const *chip;
    sensors_feature const *feature;
    cim_fan_error_t ret;

    if ((ret = _find_fan(sys_path, fan_name, &chip, &feature))
            == CIM_FAN_SUCCESS) {
        if (!(*sptr = _load_fan_data(chip, feature))) {
            ret = CIM_FAN_OTHER_ERROR;
        }
    }
    return ret;
}

cim_fan_error_t set_fan_prop_by_id(
        char const *device_id,
        cim_fan_prop_type_t prop_type,
        cim_fan_prop_value_t const *prop)
{
    sensors_chip_name const *chip;
    sensors_feature const *feature;
    cim_fan_error_t err;

    if (!(err = _find_fan_by_id(device_id, &chip, &feature)))
        err = _set_fan_prop(chip, prop_type, prop);
    return err;
}

cim_fan_error_t set_fan_prop(
        char const *sys_path,
        char const *fan_name,
        cim_fan_prop_type_t prop_type,
        cim_fan_prop_value_t const *prop)
{
    sensors_chip_name const *chip;
    sensors_feature const *feature;
    cim_fan_error_t err;

    if (!(err = _find_fan(sys_path, fan_name, &chip, &feature)))
        err = _set_fan_prop(chip, prop_type, prop);
    return err;
}

void free_fanlist(struct fanlist *lptr) {
    struct fanlist *tmp;
    while (lptr) {
        tmp = lptr->next;
        if (lptr->f) free_fan(lptr->f);
        free(lptr);
        lptr = tmp;
    }
}

void free_fan(struct cim_fan *fan) {
    if (fan) {
        if (fan->name) free((char*) fan->name);
        if (fan->chip_name) free((char*) fan->chip_name);
        if (fan->device_id) free((char*) fan->device_id);
        free(fan);
    }
}

cim_fan_error_t init_linux_fan_module() {
    if (!MODULE_INITIALIZED) {
        if (reload_config_file(NULL)) return CIM_FAN_INIT_FAILED;
        MODULE_INITIALIZED = true;
    }
    return CIM_FAN_SUCCESS;
}

void cleanup_linux_fan_module() {
    if (MODULE_INITIALIZED) {
        sensors_cleanup();
        MODULE_INITIALIZED = false;
    }
}

char const *fan_get_current_state(struct cim_fan const *sptr)
{
    if (sptr->accessible_features & CIM_FAN_AF_MIN_SPEED) {
        if (sptr->speed < sptr->min_speed) return "Below Minimum";
        if (sptr->speed == sptr->min_speed) return "At Minimum";
    }
    if (sptr->accessible_features & CIM_FAN_AF_MAX_SPEED) {
        if (sptr->speed > sptr->max_speed) return "Above Maximum";
        if (sptr->speed == sptr->max_speed) return "At Maximum";
    }
    return "Normal";
}
