set(PROVIDER_NAME Hardware)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_Hardware.mof)
set(CIMPROVAGT_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

set(provider_SRCS
    utils.c
    dmidecode.c
    lscpu.c
    procfs.c
    sysfs.c
    PCIDev.c
    smartctl.c
    lsblk.c
    virt_what.c
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

pkg_check_modules(LIBPCI REQUIRED libpci)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMPI_INCLUDE_DIR} ${LIBPCI_INCLUDE_DIR})
target_link_libraries(${LIBRARY_NAME} m openlmicommon ${KONKRETCMPI_LIBRARIES} ${LIBPCI_LIBRARIES})

# Create registration file
set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_${PROVIDER_NAME}_Profile.mof")
profile_mof_generate("90_LMI_${PROVIDER_NAME}_Profile.mof.skel" "${TARGET_MOF}" "${CIM_CLASSES}")
set(TARGET_MOF_DMTF "${CMAKE_BINARY_DIR}/mof/90_LMI_${PROVIDER_NAME}_Profile_DMTF.mof")
configure_file("90_LMI_${PROVIDER_NAME}_Profile_DMTF.mof" "${TARGET_MOF_DMTF}" COPYONLY)

set(TARGET_MOFS "${TARGET_MOF}" "${TARGET_MOF_DMTF}")
cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers "${TARGET_MOFS}")

install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi)
install(PROGRAMS ${CIMPROVAGT_SCRIPT} DESTINATION libexec/pegasus)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
install(FILES ${TARGET_MOF_DMTF} DESTINATION share/openlmi-providers/)
