
set(PROVIDER_NAME Account)
set(LIBRARY_NAME cmpiLMI_${PROVIDER_NAME})
set(MOF 60_LMI_Account.mof)
set(ACCOUNT_SCRIPT cmpiLMI_${PROVIDER_NAME}-cimprovagt)

set(provider_SRCS
    aux_lu.c
    account_globals.c
    indication_common.c
    lock.c
)

if (NOT DEFINED CRYPT_ALGS)
  message ("Setting default crypto algorithms")
  set (CRYPT_ALGS "\"DES\"")
endif (NOT DEFINED CRYPT_ALGS)

message ("Using crypto algorithms: ${CRYPT_ALGS}")
add_definitions(-DCRYPT_ALGS=${CRYPT_ALGS} -DINOTIFY_THREAD_SAFE)

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

set(CIM_PROVIDERS_CLASSES "")
foreach(CIM_CLASS  ${CIM_CLASSES})
    if(NOT ${CIM_CLASS} MATCHES "Indication")
        set(CIM_PROVIDERS_CLASSES ${CIM_PROVIDERS_CLASSES} ${CIM_CLASS})
    endif(NOT ${CIM_CLASS} MATCHES "Indication")
endforeach(CIM_CLASS  ${CIM_CLASSES})

set(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_Account_Profile.mof")
profile_mof_generate("90_LMI_Account_Profile.mof.skel" "${TARGET_MOF}" "${CIM_PROVIDERS_CLASSES}")

# Require libuser
pkg_check_modules(LIBUSER REQUIRED libuser>=0.60)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMPI_INCLUDE_DIR} ${GLIB_INCLUDE_DIRS} ${LIBUSER_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src/libs/indmanager)

target_link_libraries(${LIBRARY_NAME} openlmicommon ${KONKRETCMPI_LIBRARIES} ${GLIB_LIBRARIES} ${LIBUSER_LIBRARIES} openlmiindmanager)

# Create registration file
cim_registration(${PROVIDER_NAME} ${LIBRARY_NAME} ${MOF} share/openlmi-providers "${TARGET_MOF}")

install(TARGETS ${LIBRARY_NAME} DESTINATION lib${LIB_SUFFIX}/cmpi/)
install(PROGRAMS ${ACCOUNT_SCRIPT} DESTINATION libexec/pegasus)
install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
