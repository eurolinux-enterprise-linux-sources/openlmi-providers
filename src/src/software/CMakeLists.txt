SET(TARGET_MOF "${CMAKE_BINARY_DIR}/mof/90_LMI_Software_Profile.mof")
SET(SKEL_MOF "90_LMI_Software_Profile.mof.skel")
SET(INSTANCE_SKEL "")

file(READ "${SKEL_MOF}" INSTANCE_SKEL)
file(WRITE "${TARGET_MOF}" "")
string(REPLACE "\@VERSION\@" ${OPENLMI_VERSION} INSTANCE "${INSTANCE_SKEL}")
file(APPEND "${TARGET_MOF}" "${INSTANCE}")
message(STATUS "Generated profile mof ${TARGET_MOF}")

install(FILES ${TARGET_MOF} DESTINATION share/openlmi-providers/)
