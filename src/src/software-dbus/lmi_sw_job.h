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

#ifndef LMI_SW_JOB_H
#define LMI_SW_JOB_H

#include <cmpimacs.h>
#include "lmi_job.h"

#define INSTALL_OPTION_DEFER_TARGET_SYSTEM_RESET 2
#define INSTALL_OPTION_FORCE_INSTALLATION        3
#define INSTALL_OPTION_INSTALL                   4
#define INSTALL_OPTION_UPDATE                    5
#define INSTALL_OPTION_REPAIR                    6
#define INSTALL_OPTION_REBOOT                    7
#define INSTALL_OPTION_PASSWORD                  8
#define INSTALL_OPTION_UNINSTALL                 9
#define INSTALL_OPTION_LOG                       10
#define INSTALL_OPTION_SILENT_MODE               11
#define INSTALL_OPTION_ADMINISTRATIVE_MODE       12
#define INSTALL_OPTION_SCHEDULE_INSTALL_AT       13

#define INSTALLATION_OPERATION_INSTALL   (1 << 0)
#define INSTALLATION_OPERATION_UPDATE    (1 << 1)
#define INSTALLATION_OPERATION_REMOVE    (1 << 2)
#define INSTALLATION_OPERATION_FORCE     (1 << 3)
#define INSTALLATION_OPERATION_REPAIR    (1 << 4)
#define INSTALLATION_OPERATION_EXCLUSIVE_GROUP ( \
            INSTALLATION_OPERATION_INSTALL | \
            INSTALLATION_OPERATION_UPDATE  | \
            INSTALLATION_OPERATION_REMOVE)

#define INSTALL_METHOD_RESULT_JOB_COMPLETED_WITH_NO_ERROR                      0
#define INSTALL_METHOD_RESULT_NOT_SUPPORTED                                    1
#define INSTALL_METHOD_RESULT_UNSPECIFIED_ERROR                                2
#define INSTALL_METHOD_RESULT_TIMEOUT                                          3
#define INSTALL_METHOD_RESULT_FAILED                                           4
#define INSTALL_METHOD_RESULT_INVALID_PARAMETER                                5
#define INSTALL_METHOD_RESULT_TARGET_IN_USE                                    6
#define INSTALL_METHOD_RESULT_JOB_STARTED                                   4096
#define INSTALL_METHOD_RESULT_UNSUPPORTED_TARGET_TYPE                       4097
#define INSTALL_METHOD_RESULT_UNATTENDED_INSTALLATION_NOT_SUPPORTED         4098
#define INSTALL_METHOD_RESULT_DOWNGRADE_REINSTALL_NOT_SUPPORTED             4099
#define INSTALL_METHOD_RESULT_NOT_ENOUGH_MEMORY                             4100
#define INSTALL_METHOD_RESULT_NOT_ENOUGH_SWAP_SPACE                         4101
#define INSTALL_METHOD_RESULT_UNSUPPORTED_VERSION_TRANSITION                4102
#define INSTALL_METHOD_RESULT_NOT_ENOUGH_DISK_SPACE                         4103
#define INSTALL_METHOD_RESULT_SOFTWARE_AND_TARGET_OPERATING_SYSTEM_MISMATCH 4104
#define INSTALL_METHOD_RESULT_MISSING_DEPENDENCIES                          4105
#define INSTALL_METHOD_RESULT_NOT_APPLICABLE_TO_TARGET                      4106
#define INSTALL_METHOD_RESULT_NO_SUPPORTED_PATH_TO_IMAGE                    4107
#define INSTALL_METHOD_RESULT_CANNOT_ADD_TO_COLLECTION                      4108

#define IN_PARAM_INSTALL_OPTIONS_NAME "InstallOptions"
#define IN_PARAM_TARGET_NAME          "Target"
#define IN_PARAM_COLLECTION_NAME      "Collection"
#define IN_PARAM_SOURCE_NAME          "Source"
#define IN_PARAM_URI_NAME             "URI"
#define OUT_PARAM_AFFECTED_PACKAGES   "AffectedPackages"

/******************************************************************************
 * Software installation job
 *****************************************************************************/
#define LMI_TYPE_SW_INSTALLATION_JOB \
    (lmi_sw_installation_job_get_type ())
#define LMI_SW_INSTALLATION_JOB(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), LMI_TYPE_SW_INSTALLATION_JOB, \
                                 LmiSwInstallationJob))
#define LMI_IS_SW_INSTALLATION_JOB(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LMI_TYPE_SW_INSTALLATION_JOB))
#define LMI_SW_INSTALLATION_JOB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), LMI_TYPE_SW_INSTALLATION_JOB, \
                              LmiSwInstallationJobClass))
#define LMI_IS_SW_INSTALLATION_JOB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), LMI_TYPE_SW_INSTALLATION_JOB))
#define LMI_SW_INSTALLATION_JOB_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), LMI_TYPE_SW_INSTALLATION_JOB, \
                                LmiSwInstallationJobClass))

typedef struct _LmiSwInstallationJob        LmiSwInstallationJob;
typedef struct _LmiSwInstallationJobClass   LmiSwInstallationJobClass;

struct _LmiSwInstallationJob {
    LmiJob parent;
};

struct _LmiSwInstallationJobClass {
    LmiJobClass parent_class;
};

GType lmi_sw_installation_job_get_type();

LmiSwInstallationJob *lmi_sw_installation_job_new();

void lmi_sw_installation_job_process(LmiJob *job,
                                     GCancellable *cancellable);

/******************************************************************************
 * Software verification job
 *****************************************************************************/

#define LMI_TYPE_SW_VERIFICATION_JOB \
    (lmi_sw_verification_job_get_type ())
#define LMI_SW_VERIFICATION_JOB(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), LMI_TYPE_SW_VERIFICATION_JOB, \
                                 LmiSwVerificationJob))
#define LMI_IS_SW_VERIFICATION_JOB(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LMI_TYPE_SW_VERIFICATION_JOB))
#define LMI_SW_VERIFICATION_JOB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), LMI_TYPE_SW_VERIFICATION_JOB, \
                              LmiSwVerificationJobClass))
#define LMI_IS_SW_VERIFICATION_JOB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), LMI_TYPE_SW_VERIFICATION_JOB))
#define LMI_SW_VERIFICATION_JOB_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), LMI_TYPE_SW_VERIFICATION_JOB, \
                                LmiSwVerificationJobClass))

typedef struct _LmiSwVerificationJob        LmiSwVerificationJob;
typedef struct _LmiSwVerificationJobClass   LmiSwVerificationJobClass;

struct _LmiSwVerificationJob {
    LmiJob parent;
};

struct _LmiSwVerificationJobClass {
    LmiJobClass parent_class;
};

GType lmi_sw_verification_job_get_type();

LmiSwVerificationJob *lmi_sw_verification_job_new();

void lmi_sw_verification_job_process(LmiJob *job,
                                     GCancellable *cancellable);

/******************************************************************************
 * CIM related stuff
 *****************************************************************************/
CMPIStatus lmi_sw_job_to_cim_instance(const CMPIBroker *cb,
                                      const CMPIContext *ctx,
                                      const LmiJob *job,
                                      CMPIInstance *instance);

CMPIStatus lmi_sw_job_make_job_parameters(const CMPIBroker *cb,
                                          const CMPIContext *ctx,
                                          const LmiJob *job,
                                          gboolean include_input,
                                          gboolean include_output,
                                          CMPIInstance *instance);

CMPIStatus lmi_sw_job_get_number_from_instance_id(
    const gchar *instance_id,
    guint *number);

CMPIStatus lmi_sw_job_get_number_from_op(const CMPIObjectPath *op,
                                         guint *number);

#endif /* end of include guard: LMI_SW_JOB_H */
