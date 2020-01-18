
set(PROVIDER_NAME Fan)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_Fan.mof)
set(FAN_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

set(provider_SRCS
    fan.c
    LMI_FanProvider.c
)

find_package(LMSensors REQUIRED)

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

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMPI_INCLUDE_DIR})

target_link_libraries(${LIBRARY_NAME} openlmicommon ${KONKRETCMPI_LIBRARIES} sensors)

# Create registration file
set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_Fan_Profile.mof")
profile_mof_generate("90_LMI_Fan_Profile.mof.skel" "${TARGET_MOF}" "${CIM_CLASSES}")
cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers "${TARGET_MOF}")

install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi/)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
install(PROGRAMS ${FAN_SCRIPT} DESTINATION libexec/pegasus)
