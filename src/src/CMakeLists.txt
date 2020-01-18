include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    libs/libopenlmi
    ${CMPI_INCLUDE_DIR}
    ${GLIB_INCLUDE_DIRS})

add_subdirectory(libs/libopenlmi)

if (WITH-FAN)
    add_subdirectory(fan)
endif (WITH-FAN)

if (WITH-POWER)
    add_subdirectory(power)
endif (WITH-POWER)

if (WITH-SERVICE)
    add_subdirectory(service-dbus)
endif (WITH-SERVICE)

if (WITH-SERVICE-LEGACY)
    add_subdirectory(service)
endif (WITH-SERVICE-LEGACY)

if (WITH-SERVICE AND WITH-SERVICE-LEGACY)
    message(FATAL_ERROR "Service and Service-legacy can't be enabled together")
endif (WITH-SERVICE AND WITH-SERVICE-LEGACY)

if (WITH-ACCOUNT)
    add_subdirectory(account)
endif (WITH-ACCOUNT)

if (WITH-HARDWARE)
    add_subdirectory(hardware)
endif (WITH-HARDWARE)

if (WITH-LOGICALFILE)
    add_subdirectory(logicalfile)
endif (WITH-LOGICALFILE)

if (WITH-REALMD)
    add_subdirectory(realmd)
endif (WITH-REALMD)

if (WITH-INDMANAGER)
    add_subdirectory(libs/indmanager)
endif (WITH-INDMANAGER)

if (WITH-INDSENDER)
    add_subdirectory(libs/indsender)
endif (WITH-INDSENDER)

if (WITH-JOBMANAGER)
    add_subdirectory(libs/jobmanager)
endif (WITH-JOBMANAGER)

if (WITH-SOFTWARE)
    if (WITH-SOFTWARE-DBUS)
        add_subdirectory(software-dbus)
    else (WITH-SOFTWARE-DBUS)
        add_subdirectory(software)
    endif (WITH-SOFTWARE-DBUS)
endif (WITH-SOFTWARE)

if (WITH-JOURNALD)
    add_subdirectory(journald)
endif (WITH-JOURNALD)

if (WITH-PCP)
    add_subdirectory(pcp)
endif (WITH-PCP)

if (WITH-LOCALE)
    add_subdirectory(locale)
endif (WITH-LOCALE)

if (WITH-SSSD)
    add_subdirectory(sssd)
endif (WITH-SSSD)

file(GLOB INIT_SKELS "*/lmi/*/__init__.skel")
foreach(SKEL ${INIT_SKELS})
    #TODO: find a way how to do it in cmake
    execute_process(COMMAND dirname ${SKEL} OUTPUT_VARIABLE SKEL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    configure_file(${SKEL} "${SKEL_DIR}/__init__.py" @ONLY)
    message(STATUS "Written ${SKEL_DIR}/__init__.py module.")
endforeach(SKEL)
