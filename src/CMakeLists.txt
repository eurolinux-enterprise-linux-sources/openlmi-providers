
project(openlmi-providers C)

set(OPENLMI_VERSION_MAJOR 0)
set(OPENLMI_VERSION_MINOR 4)
set(OPENLMI_VERSION_REVISION 2)
set(OPENLMI_VERSION "${OPENLMI_VERSION_MAJOR}.${OPENLMI_VERSION_MINOR}.${OPENLMI_VERSION_REVISION}")

cmake_minimum_required(VERSION 2.6)

# Set flags and definitions
add_definitions(-D_XOPEN_SOURCE=500 -D_GNU_SOURCE)
set(CMAKE_C_FLAGS "-std=c99 -fstack-protector-strong -Wall -pedantic -g -Wextra -Wno-unused-parameter -Wformat -Wparentheses -Wl,--no-undefined ${CMAKE_C_FLAGS}")

# Set LIB_SUFFIX to 64 on 64bit architectures
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(LIB_SUFFIX "")
else(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX 64)
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

if(NOT SYSCONF_INSTALL_DIR)
    set(SYSCONF_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/etc)
endif(NOT SYSCONF_INSTALL_DIR)

option(WITH-FAN "Build fan provider" ON)
option(WITH-POWER "Build power provider" ON)
option(WITH-SERVICE "Build service provider" ON)
option(WITH-ACCOUNT "Build account provider" ON)
option(WITH-HARDWARE "Build hardware provider" ON)
option(WITH-LOGICALFILE "Build logical file provider" ON)
option(WITH-REALMD "Build RealmD provider" ON)
option(WITH-PCP "Build PCP provider" ON)
option(WITH-INDMANAGER "Build indication manager" ON)
option(WITH-SOFTWARE "Build software provider" ON)
option(WITH-JOURNALD "Build journald provider" ON)

# Set path to custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(OpenLMIMacros)

# Set some file paths
set(OPENLMI_QUALIFIERS_MOF "${CMAKE_SOURCE_DIR}/mof/05_LMI_Qualifiers.mof")
set(OPENLMI_JOBS_MOF "${CMAKE_SOURCE_DIR}/mof/30_LMI_Jobs.mof")
set(OPENLMI_MOF_REGISTER "${CMAKE_SOURCE_DIR}/openlmi-mof-register")

find_package(PkgConfig)

# Find required packages
find_package(CMPI REQUIRED)
find_package(KonkretCMPI REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)

add_subdirectory(src)
add_subdirectory(mof)
add_subdirectory(tools)
add_subdirectory(doc)

install(PROGRAMS openlmi-mof-register DESTINATION bin)
install(FILES cmake/modules/OpenLMIMacros.cmake DESTINATION share/cmake/Modules)
install(FILES cmake/modules/FindCMPI.cmake DESTINATION share/cmake/Modules)
install(FILES cmake/modules/FindKonkretCMPI.cmake DESTINATION share/cmake/Modules)
install(FILES cmake/modules/FindOpenLMI.cmake DESTINATION share/cmake/Modules)
install(FILES cmake/modules/FindOpenLMIIndManager.cmake DESTINATION share/cmake/Modules)
