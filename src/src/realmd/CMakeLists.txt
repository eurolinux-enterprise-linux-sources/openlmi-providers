pkg_check_modules(DBUS1  dbus-1 REQUIRED)
pkg_check_modules(GLIB2  glib-2.0 REQUIRED)

set(PROVIDER_NAME Realmd)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_Realmd.mof)
set(CIMPROVAGT_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

set(provider_SRCS
    LMI_HostedRealmdServiceProvider.c
    LMI_RealmdServiceProvider.c
    rdcp_dbus.c
    rdcp_error.c
    rdcp_util.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

konkretcmpi_generate(${MOF}
                     CIM_PROVIDERS
                     CIM_HEADERS
                     CIM_CLASSES
)

add_library(${LIBRARY_NAME} SHARED
            ${provider_SRCS}
            ${CIM_PROVIDERS}
            ${CIM_HEADERS}
)

# FIXME - /usr/include/openlmi shouldn't be hardcoded, needed for globals.h
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMPI_INCLUDE_DIR}
                    ${DBUS1_INCLUDE_DIRS}
                    ${GLIB2_INCLUDE_DIRS}
                    )

target_link_libraries(${LIBRARY_NAME}
                      openlmicommon
                      ${KONKRETCMPI_LIBRARIES}
                      ${DBUS1_LIBRARIES}
                      ${GLIB2_LIBRARIES}
                      )

# Create registration file
set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_Realmd_Profile.mof")
profile_mof_generate("90_LMI_Realmd_Profile.mof.skel" "${TARGET_MOF}" "${CIM_CLASSES}")

cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers "${TARGET_MOF}")


install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi/)
install(PROGRAMS ${CIMPROVAGT_SCRIPT} DESTINATION libexec/pegasus)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
