pkg_check_modules(GLIB2  glib-2.0 REQUIRED)
pkg_check_modules(SYSTEMD-JOURNAL  libsystemd-journal REQUIRED)

set(PROVIDER_NAME Journald)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_Journald.mof)
set(CIMPROVAGT_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

set(provider_SRCS
    instutil.c
    journal.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

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

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMPI_INCLUDE_DIR}
                    ${GLIB2_INCLUDE_DIRS}
                    ${SYSTEMD-JOURNAL_INCLUDE_DIRS}
                    ${CMAKE_SOURCE_DIR}/src/libs/indmanager
                    )

target_link_libraries(${LIBRARY_NAME}
                      openlmicommon
                      openlmiindmanager
                      ${KONKRETCMPI_LIBRARIES}
                      ${GLIB2_LIBRARIES}
                      ${SYSTEMD-JOURNAL_LIBRARIES}
                      )

set(CIM_PROVIDERS_CLASSES "")
foreach(CIM_CLASS  ${CIM_CLASSES})
    if(NOT ${CIM_CLASS} MATCHES "Indication")
        set(CIM_PROVIDERS_CLASSES ${CIM_PROVIDERS_CLASSES} ${CIM_CLASS})
    endif(NOT ${CIM_CLASS} MATCHES "Indication")
endforeach(CIM_CLASS  ${CIM_CLASSES})

set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_Journald_Profile.mof")
profile_mof_generate("90_LMI_Journald_Profile.mof.skel" "${TARGET_MOF}" "${CIM_PROVIDERS_CLASSES}")

# Create registration file
cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers "${TARGET_MOF}")

install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi/)
install(PROGRAMS ${CIMPROVAGT_SCRIPT} DESTINATION libexec/pegasus)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
