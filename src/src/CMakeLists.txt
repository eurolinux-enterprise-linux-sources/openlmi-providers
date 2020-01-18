include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMPI_INCLUDE_DIR} ${GLIB_INCLUDE_DIRS})

add_library(openlmicommon SHARED
            openlmi.c
)

target_link_libraries(openlmicommon ${GLIB_LIBRARIES} dl)

set(OPENLMICOMMON_VERSION_MAJOR 0)
set(OPENLMICOMMON_VERSION_MINOR 0)
set(OPENLMICOMMON_VERSION_PATCH 1)
set(OPENLMICOMMON_VERSION "${OPENLMICOMMON_VERSION_MAJOR}.${OPENLMICOMMON_VERSION_MINOR}.${OPENLMICOMMON_VERSION_PATCH}")

set_target_properties(openlmicommon PROPERTIES VERSION ${OPENLMICOMMON_VERSION})
set_target_properties(openlmicommon PROPERTIES SOVERSION ${OPENLMICOMMON_VERSION_MAJOR})

install(TARGETS openlmicommon DESTINATION lib${LIB_SUFFIX})
install(FILES openlmi.h DESTINATION include/openlmi)
configure_file(openlmi.pc.in openlmi.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openlmi.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
install(FILES openlmi.conf DESTINATION ${SYSCONF_INSTALL_DIR}/openlmi)

if (WITH-FAN)
    add_subdirectory(fan)
endif (WITH-FAN)

if (WITH-POWER)
    add_subdirectory(power)
endif (WITH-POWER)

if (WITH-SERVICE)
    add_subdirectory(service-dbus)
endif (WITH-SERVICE)

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
    add_subdirectory(indmanager)
endif (WITH-INDMANAGER)

if (WITH-SOFTWARE)
    add_subdirectory(software)
endif (WITH-SOFTWARE)

if (WITH-JOURNALD)
    add_subdirectory(journald)
endif (WITH-JOURNALD)

file(GLOB INIT_SKELS "*/lmi/*/__init__.skel")
foreach(SKEL ${INIT_SKELS})
    #TODO: find a way how to do it in cmake
    execute_process(COMMAND dirname ${SKEL} OUTPUT_VARIABLE SKEL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    configure_file(${SKEL} "${SKEL_DIR}/__init__.py" @ONLY)
    message(STATUS "Written ${SKEL_DIR}/__init__.py module.")
endforeach(SKEL)
