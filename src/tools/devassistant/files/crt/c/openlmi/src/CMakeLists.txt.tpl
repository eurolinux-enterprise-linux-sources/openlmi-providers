set(PROVIDER_NAME {{ PROJECT_NAME }})
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_{{ PROJECT_NAME }}.mof)
set(CIMPROVAGT_SCRIPT cmpiLMI_{{ PROJECT_NAME }}-cimprovagt)

set(provider_SRCS
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
                    )

target_link_libraries(${LIBRARY_NAME}
                      openlmicommon
                      ${KONKRETCMPI_LIBRARIES}
                      )

set(CIM_PROVIDERS_CLASSES "")
foreach(CIM_CLASS  ${CIM_CLASSES})
    if(NOT ${CIM_CLASS} MATCHES "Indication")
        set(CIM_PROVIDERS_CLASSES ${CIM_PROVIDERS_CLASSES} ${CIM_CLASS})
    endif(NOT ${CIM_CLASS} MATCHES "Indication")
endforeach(CIM_CLASS  ${CIM_CLASSES})

set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_{{ PROJECT_NAME }}_Profile.mof")
profile_mof_generate("90_LMI_{{ PROJECT_NAME }}_Profile.mof.skel" "${TARGET_MOF}" "${CIM_PROVIDERS_CLASSES}")

# Create registration file
cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers "${TARGET_MOF}")

install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi/)
install(PROGRAMS ${CIMPROVAGT_SCRIPT} DESTINATION libexec/pegasus)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
