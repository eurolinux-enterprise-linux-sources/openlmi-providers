
set(PROVIDER_NAME LogicalFile)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_LogicalFile.mof)
set(CIMPROVAGT_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

set(provider_SRCS
    file.c
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

pkg_check_modules(LIBUDEV REQUIRED libudev)
pkg_check_modules(LIBSELINUX libselinux)
if(LIBSELINUX_FOUND EQUAL 1)
    add_definitions(-DLOGICALFILE_SELINUX)
endif(LIBSELINUX_FOUND EQUAL 1)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMPI_INCLUDE_DIR})
target_link_libraries(${LIBRARY_NAME} openlmicommon
                      ${KONKRETCMPI_LIBRARIES}
                      ${LIBUDEV_LIBRARIES}
                      ${LIBSELINUX_LIBRARIES})

# Create registration file
cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers)

set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_LogicalFile_Profile.mof")
profile_mof_generate("90_LMI_LogicalFile_Profile.mof.skel" "${TARGET_MOF}" "${CIM_CLASSES}")

install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi)
install(PROGRAMS ${CIMPROVAGT_SCRIPT} DESTINATION libexec/pegasus)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
