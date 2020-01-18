set(OPENLMICOMMON_VERSION_MAJOR 0)
set(OPENLMICOMMON_VERSION_MINOR 0)
set(OPENLMICOMMON_VERSION_PATCH 2)
set(OPENLMICOMMON_VERSION "${OPENLMICOMMON_VERSION_MAJOR}.${OPENLMICOMMON_VERSION_MINOR}.${OPENLMICOMMON_VERSION_PATCH}")

configure_file(openlmi.pc.in ${CMAKE_CURRENT_BINARY_DIR}/openlmi.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openlmi.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
install(FILES openlmi.conf DESTINATION ${SYSCONF_INSTALL_DIR}/openlmi)

add_library(openlmicommon SHARED
            openlmi.c
            openlmi-utils.c
)

pkg_check_modules(LIBUDEV REQUIRED libudev)

target_link_libraries(openlmicommon ${GLIB_LIBRARIES} ${LIBUDEV_LIBRARIES} dl)

set_target_properties(openlmicommon PROPERTIES VERSION ${OPENLMICOMMON_VERSION})
set_target_properties(openlmicommon PROPERTIES SOVERSION ${OPENLMICOMMON_VERSION_MAJOR})

install(TARGETS openlmicommon DESTINATION lib${LIB_SUFFIX})
install(FILES openlmi.h openlmi-utils.h DESTINATION include/openlmi)
