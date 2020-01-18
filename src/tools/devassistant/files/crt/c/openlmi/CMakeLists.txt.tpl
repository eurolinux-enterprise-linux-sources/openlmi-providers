
project({{ PROJECT_NAME|lower }} C)

set(OPENLMI_VERSION_MAJOR 0)
set(OPENLMI_VERSION_MINOR 0)
set(OPENLMI_VERSION_REVISION 1)
set(OPENLMI_VERSION "${OPENLMI_VERSION_MAJOR}.${OPENLMI_VERSION_MINOR}.${OPENLMI_VERSION_REVISION}")

cmake_minimum_required(VERSION 2.6)

# Set flags and definitions
add_definitions(-D_XOPEN_SOURCE=500 -D_GNU_SOURCE)
set(CMAKE_C_FLAGS "-Wall -g -Wextra -Wno-unused-parameter -Wformat -Wparentheses -Wl,--no-undefined ${CMAKE_C_FLAGS}")

# Set LIB_SUFFIX to 64 on 64bit architectures
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(LIB_SUFFIX "")
else(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX 64)
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

# Find OpenLMIMacros when installed in other prefix than /usr (e.g. /usr/local)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_INSTALL_PREFIX})
include(OpenLMIMacros RESULT_VARIABLE LMIMACROS)

if (${LMIMACROS} STREQUAL "NOTFOUND")
    message(FATAL_ERROR "OpenLMIMacros.cmake not found, check if openlmi-providers(-devel) is installed")
endif (${LMIMACROS} STREQUAL "NOTFOUND")

find_package(PkgConfig)

# Find required packages
find_package(CMPI REQUIRED)
find_package(KonkretCMPI REQUIRED)
find_package(OpenLMI REQUIRED)

add_subdirectory(src)
add_subdirectory(mof)
