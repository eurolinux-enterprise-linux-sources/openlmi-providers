set(PROVIDER_NAME Software)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_Software.mof)
set(LOGICALFILE_MOF 50_LMI_LogicalFile.mof)
set(CIMPROVAGT_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

set(provider_SRCS
    sw-utils.c
    lmi_sw_job.c
)

konkretcmpi_generate("${CMAKE_BINARY_DIR}/mof/${MOF}"
                     CIM_PROVIDERS
                     CIM_HEADERS
                     CIM_CLASSES
                     ${OPENLMI_QUALIFIERS_MOF}
                     ${OPENLMI_JOBS_MOF}
                     ${LOGICALFILE_MOF}
)

add_library(${LIBRARY_NAME} SHARED
            ${provider_SRCS}
            ${CIM_PROVIDERS}
            ${CIM_HEADERS}
)

pkg_check_modules(PACKAGEKIT REQUIRED packagekit-glib2)
pkg_check_modules(RPM rpm)

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMPI_INCLUDE_DIR}
                    ${PACKAGEKIT_INCLUDE_DIRS}
                    ${CMAKE_SOURCE_DIR}/src/libs/indsender
                    ${CMAKE_SOURCE_DIR}/src/libs/jobmanager)
target_link_libraries(${LIBRARY_NAME}
                      openlmicommon
                      openlmijobmanager
                      ${KONKRETCMPI_LIBRARIES}
                      ${PACKAGEKIT_LIBRARIES}
                      ${RPM_LIBRARIES})

configure_file(config.h.in
               config.h)

# Create registration file
set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_${PROVIDER_NAME}_Profile.mof")
profile_mof_generate("90_LMI_${PROVIDER_NAME}_Profile.mof.skel" "${TARGET_MOF}" "${CIM_CLASSES}")

cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers "${TARGET_MOF}")


install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi)
install(PROGRAMS ${CIMPROVAGT_SCRIPT} DESTINATION libexec/pegasus)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
