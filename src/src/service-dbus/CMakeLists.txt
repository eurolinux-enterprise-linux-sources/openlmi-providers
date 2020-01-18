
set(PROVIDER_NAME Service)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_Service.mof)
set(CIMPROVAGT_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

set(provider_SRCS
    util/serviceutil.c
)

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

# Require libgio
pkg_check_modules(GIO REQUIRED gio-2.0)

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMPI_INCLUDE_DIR}
                    ${GIO_INCLUDE_DIRS}
                    ${CMAKE_SOURCE_DIR}/src/libs/indmanager)
target_link_libraries(${LIBRARY_NAME}
                      openlmicommon
                      openlmiindmanager
                      ${KONKRETCMPI_LIBRARIES}
                      ${GIO_LIBRARIES})

# Create registration file
set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_Service_Profile.mof")
profile_mof_generate("90_LMI_Service_Profile.mof.skel" "${TARGET_MOF}" "LMI_Service")

cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers "${TARGET_MOF}")


install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi)
install(PROGRAMS ${CIMPROVAGT_SCRIPT} DESTINATION libexec/pegasus)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
