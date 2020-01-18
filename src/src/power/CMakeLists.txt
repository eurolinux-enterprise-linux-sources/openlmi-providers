
set(PROVIDER_NAME PowerManagement)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_PowerManagement.mof)
set(POWERMANAGEMENT_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

set(provider_SRCS
    power.c
)

konkretcmpi_generate(${MOF}
                     CIM_PROVIDERS
                     CIM_HEADERS
                     CIM_CLASSES
                     ${OPENLMI_QUALIFIERS_MOF}
)

add_library(${LIBRARY_NAME} SHARED
            ${provider_SRCS}
            ${CIM_PROVIDERS}
            ${CIM_HEADERS}
)

set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_${PROVIDER_NAME}_Profile.mof")
profile_mof_generate("90_LMI_${PROVIDER_NAME}_Profile.mof.skel" "${TARGET_MOF}" "${CIM_CLASSES}")

# Require GLib
pkg_check_modules(GLIB REQUIRED glib-2.0)
# Try to get Gio
pkg_check_modules(GIO gio-2.0)

if (${GIO_FOUND})
    add_definitions("-DHAS_GDBUS")
    include_directories(${GIO_INCLUDE_DIRS})
    target_link_libraries(${LIBRARY_NAME} ${GIO_LIBRARIES})
endif (${GIO_FOUND})

# Check if we have systemctl
find_program(SYSTEMCTL NAMES systemctl)
if (${SYSTEMCTL_FOUND})
    add_definitions("-DHAS_SYSTEMCTL")
endif (${SYSTEMCTL_FOUND})

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMPI_INCLUDE_DIR} ${GLIB_INCLUDE_DIRS})

target_link_libraries(${LIBRARY_NAME} openlmicommon ${KONKRETCMPI_LIBRARIES} ${GLIB_LIBRARIES})

# Create registration file
cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers "${TARGET_MOF}")

install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi/)
install(PROGRAMS ${POWERMANAGEMENT_SCRIPT} DESTINATION libexec/pegasus)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
