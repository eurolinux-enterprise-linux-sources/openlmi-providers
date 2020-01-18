
set(PROVIDER_NAME Service)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_Service.mof)

set(provider_SRCS
    util/serviceutil.c
)

konkretcmpi_generate(${MOF}
                     CIM_PROVIDERS
                     CIM_HEADERS
)

add_library(${LIBRARY_NAME} SHARED
            ${provider_SRCS}
            ${CIM_PROVIDERS}
            ${CIM_HEADERS}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMPI_INCLUDE_DIR})
target_link_libraries(${LIBRARY_NAME} openlmicommon ${KONKRETCMPI_LIBRARIES})

# Create registration file
cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers)

install(PROGRAMS util/servicedisc.sh util/serviceutil.sh DESTINATION libexec)

install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi)
