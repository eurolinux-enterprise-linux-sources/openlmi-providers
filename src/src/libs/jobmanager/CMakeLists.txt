# don't be pedantic over using pointers to functions (needed for marshallers)
STRING(REGEX REPLACE "-pedantic *" "" NEW_C_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS ${NEW_C_FLAGS})

pkg_check_modules(GOBJECT REQUIRED gobject-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(JSON-GLib REQUIRED json-glib-1.0)

add_library(openlmijobmanager SHARED
            lmi_job.c
            lmi_job_serialization.c
            job_manager.c
)

set(OPENLMIJOBMANAGER_VERSION_MAJOR 0)
set(OPENLMIJOBMANAGER_VERSION_MINOR 0)
set(OPENLMIJOBMANAGER_VERSION_PATCH 1)
set(OPENLMIJOBMANAGER_VERSION "${OPENLMIJOBMANAGER_VERSION_MAJOR}.${OPENLMIJOBMANAGER_VERSION_MINOR}.${OPENLMIJOBMANAGER_VERSION_PATCH}")

set_target_properties(openlmijobmanager PROPERTIES VERSION ${OPENLMIJOBMANAGER_VERSION})
set_target_properties(openlmijobmanager PROPERTIES SOVERSION ${OPENLMIJOBMANAGER_VERSION_MAJOR})

#include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMPI_INCLUDE_DIR} ${GLIB_INCLUDE_DIRS} ${LIBUSER_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src/indmanager)
include_directories(
    ${GOBJECT_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${JSON-GLib_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/libs/indsender
)

target_link_libraries(openlmijobmanager
    openlmicommon
    openlmiindsender
    ${GLIB_LIBRARIES}
    ${GOBJECT_LIBRARIES}
    ${GIO_LIBRARIES}
    ${JSON-GLib_LIBRARIES}
    pthread
    dl
)

install(TARGETS openlmijobmanager DESTINATION lib${LIB_SUFFIX})
install(FILES job_manager.h lmi_job.h DESTINATION include/openlmi)

configure_file(openlmijobmanager.pc.in ${CMAKE_CURRENT_BINARY_DIR}/openlmijobmanager.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openlmijobmanager.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
