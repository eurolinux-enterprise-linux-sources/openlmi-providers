
find_path(LMSENSORS_INCLUDE_DIR
    NAMES sensors.h error.h
    HINTS $ENV{LMSENSORS_INCLUDE_DIR}
    PATH_SUFFIXES include/sensors include
    PATHS /usr /usr/local
)

find_library(LMSENSORS_LIBRARY
    NAMES sensors
    HINTS $ENV{LMSENSORS_LIB_DIR}
    PATH_SUFFIXES lib64 lib
    PATHS /usr /usr/local
)

if (LMSENSORS_INCLUDE_DIR AND EXISTS "${LMSENSORS_INCLUDE_DIR}/sensors.h")
    file(STRINGS "${LMSENSORS_INCLUDE_DIR}/sensors.h" lmsensors_version_str
         REGEX "^#[\t ]*define[\t ]+SENSORS_API_VERSION[\t ]+0x[0-9]+$")
    string(REGEX REPLACE "^.*SENSORS_API_VERSION[\t ]+0x([0-9]+).*$" "\\1" LMSENSORS_VERSION_STR "${lmsensors_version_str}")
    unset(lmsensors_version_str)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LMSENSORS
        REQUIRED_VARS LMSENSORS_INCLUDE_DIR LMSENSORS_LIBRARY
        VERSION_VAR LMSENSORS_VERSION_STR)

if(LMSENSORS_FOUND)
    set(LMSENSORS_LIBRARIES ${LMSENSORS_LIBRARY})
    set(LMSENSORS_INCLUDE_DIRS ${LMSENSORS_INCLUDE_DIR})
endif(LMSENSORS_FOUND)

mark_as_advanced(LMSENSORS_INCLUDE_DIR LMSENSORS_LIBRARY)
