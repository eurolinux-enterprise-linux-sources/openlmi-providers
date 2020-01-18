
find_path(OPENLMI_INCLUDE_DIR
    NAMES openlmi.h
    HINTS $ENV{OPENLMI_INCLUDE_DIR}
    PATH_SUFFIXES include/openlmi
    PATHS /usr /usr/local
)

find_file(OPENLMI_QUALIFIERS_MOF
    NAMES 05_LMI_Qualifiers.mof
    HINTS $ENV{OPENLMI_MOF_DIR}
    PATH_SUFFIXES share/openlmi-providers
    PATHS /usr /usr/local
)

find_file(OPENLMI_JOBS_MOF
    NAMES 30_LMI_Jobs.mof
    HINTS $ENV{OPENLMI_MOF_DIR}
    PATH_SUFFIXES share/openlmi-providers
    PATHS /usr /usr/local
)

find_library(OPENLMICOMMON_LIBRARY
    NAMES openlmicommon
    HINTS $ENV{OPENLMI_LIB_DIR}
    PATH_SUFFIXES lib64 lib
    PATHS /usr /usr/local
)

find_program(OPENLMI_MOF_REGISTER
    NAMES openlmi-mof-register
    HINTS $ENV{OPENLMI_MOF_REGISTER}
    PATH_SUFFIXES bin
    PATHS ${CMAKE_SOURCE_DIR} /usr /usr/local
)

set(OPENLMI_LIBRARIES ${OPENLMICOMMON_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OPENLMI
        REQUIRED_VARS OPENLMI_LIBRARIES OPENLMI_INCLUDE_DIR
                      OPENLMI_QUALIFIERS_MOF OPENLMI_JOBS_MOF
                      OPENLMI_MOF_REGISTER
        VERSION_VAR OPENLMI_VERSION)

mark_as_advanced(OPENLMI_INCLUDE_DIR OPENLMI_LIBRARIES)
