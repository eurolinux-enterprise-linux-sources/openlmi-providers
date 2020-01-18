add_library(openlmiindmanager SHARED
            ind_manager.c
)

set(OPENLMIINDMANAGER_VERSION_MAJOR 1)
set(OPENLMIINDMANAGER_VERSION_MINOR 0)
set(OPENLMIINDMANAGER_VERSION_PATCH 0)
set(OPENLMIINDMANAGER_VERSION "${OPENLMIINDMANAGER_VERSION_MAJOR}.${OPENLMIINDMANAGER_VERSION_MINOR}.${OPENLMIINDMANAGER_VERSION_PATCH}")

set_target_properties(openlmiindmanager PROPERTIES VERSION ${OPENLMIINDMANAGER_VERSION})
set_target_properties(openlmiindmanager PROPERTIES SOVERSION ${OPENLMIINDMANAGER_VERSION_MAJOR})

target_link_libraries(openlmiindmanager pthread)

install(TARGETS openlmiindmanager DESTINATION lib${LIB_SUFFIX})
install(FILES ind_manager.h DESTINATION include/openlmi)

configure_file(openlmiindmanager.pc.in ${CMAKE_CURRENT_BINARY_DIR}/openlmiindmanager.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openlmiindmanager.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
