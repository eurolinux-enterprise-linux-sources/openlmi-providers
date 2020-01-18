add_library(openlmiindsender SHARED
            ind_sender.c
)

set(OPENLMIINDSENDER_VERSION_MAJOR 0)
set(OPENLMIINDSENDER_VERSION_MINOR 0)
set(OPENLMIINDSENDER_VERSION_PATCH 1)
set(OPENLMIINDSENDER_VERSION "${OPENLMIINDSENDER_VERSION_MAJOR}.${OPENLMIINDSENDER_VERSION_MINOR}.${OPENLMIINDSENDER_VERSION_PATCH}")

set_target_properties(openlmiindsender PROPERTIES VERSION ${OPENLMIINDSENDER_VERSION})
set_target_properties(openlmiindsender PROPERTIES SOVERSION ${OPENLMIINDSENDER_VERSION_MAJOR})


target_link_libraries(openlmiindsender
    openlmicommon
    ${GLIB_LIBRARIES}
)

install(TARGETS openlmiindsender DESTINATION lib${LIB_SUFFIX})
install(FILES ind_sender.h DESTINATION include/openlmi)

configure_file(openlmiindsender.pc.in ${CMAKE_CURRENT_BINARY_DIR}/openlmiindsender.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openlmiindsender.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
