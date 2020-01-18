pkg_check_modules(DBUS1 dbus-1 REQUIRED)
pkg_check_modules(SSS_SIMPLEIFP sss_simpleifp REQUIRED)

set(PROVIDER_NAME SSSD)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_${PROVIDER_NAME}.mof)
set(CIMPROVAGT_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

# Source files
set(provider_SRCS
    utils.c
    sssd_components.c
    sssd_domains.c)

konkretcmpi_generate(${MOF} CIM_PROVIDERS CIM_HEADERS CIM_CLASSES)

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMPI_INCLUDE_DIR}
                    ${GLIB_INCLUDE_DIRS}
                    ${DBUS1_INCLUDE_DIRS}
                    ${SSS_SIMPLEIFP_INCLUDE_DIR})

add_library(${LIBRARY_NAME} SHARED
            ${provider_SRCS}
            ${CIM_PROVIDERS}
            ${CIM_HEADERS})

target_link_libraries(${LIBRARY_NAME}
                      openlmicommon
                      ${KONKRETCMPI_LIBRARIES}
                      ${GLIB_LIBRARIES}
                      ${DBUS1_LIBRARIES}
                      ${SSS_SIMPLEIFP_LIBRARIES})

# Create registration file
cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers)

set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_SSSD_Profile.mof")
profile_mof_generate("90_LMI_SSSD_Profile.mof.skel" "${TARGET_MOF}" "LMI_SSSD")

install(PROGRAMS ${CIMPROVAGT_SCRIPT} DESTINATION libexec/pegasus)
install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
