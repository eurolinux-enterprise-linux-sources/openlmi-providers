
# This macro takes names of the MOFs for one provider and creates the
# headerfiles using konkretcmpi. It also generates provider skeleton
# if it doesn't exist.
#
# @param[in] MOFS filenames of the MOF files which will be used to generate
#            or a list with multiple MOF files, where relative paths mean mof/ directory
#            of the project root (Note: list is semi-colon separated, enclosed in quotes)
# @param[out] CIM_PROVIDERS list of sources of the provider generated from the MOFS
# @param[out] CIM_HEADERS list of header files generated from the MOFS
# @param[out] CIM_CLASSES list of CIM classes defined in the MOFS
# @param[in] the rest of the argument are mof files that should be included
# in konkret header generation but are not scanned for classes
#
macro(konkretcmpi_generate MOFS CIM_PROVIDERS CIM_HEADERS CIM_CLASSES)
    # Create list of MOF files for konkret generator and
    # list of classes defined in the MOF files
    set(KONKRET_MOF_FILES "")
    set(CIM_CLASS_NAMES "")

    # Add includes first (needs to be parsed before others)
    foreach(MOF ${ARGN})
        # Check if path is absolute, make it absolute if not
        if (IS_ABSOLUTE ${MOF})
            set(MOF_FILE ${MOF})
        else (IS_ABSOLUTE ${MOF})
            set(MOF_FILE ${CMAKE_SOURCE_DIR}/mof/${MOF})
        endif (IS_ABSOLUTE ${MOF})

        # Check if MOF file exists
        if (NOT EXISTS ${MOF_FILE})
            message(FATAL_ERROR "MOF file ${MOF} not found")
        endif (NOT EXISTS ${MOF_FILE})

        message(STATUS "Using mof ${MOF} ${MOF_FILE}")
        set(KONKRET_MOF_FILES ${KONKRET_MOF_FILES} -m ${MOF_FILE})
    endforeach(MOF ${ARGN})

    # Read the MOFs with class definition
    foreach(MOF ${MOFS})
        # Check if path is absolute, make it absolute if not
        if (IS_ABSOLUTE ${MOF})
            set(MOF_FILE ${MOF})
        else (IS_ABSOLUTE ${MOF})
            set(MOF_FILE ${CMAKE_SOURCE_DIR}/mof/${MOF})
        endif (IS_ABSOLUTE ${MOF})

        # Check if MOF file exists
        if (NOT EXISTS ${MOF_FILE})
            message(FATAL_ERROR "MOF file ${MOF} not found")
        endif (NOT EXISTS ${MOF_FILE})

        message(STATUS "Using mof ${MOF} ${MOF_FILE}")
        set(KONKRET_MOF_FILES ${KONKRET_MOF_FILES} -m ${MOF_FILE})

        # Read CIM classes out of MOF files
        file(READ ${MOF_FILE} MOF_CONTENT)
        string(REGEX MATCHALL "\nclass [A-Za-z0-9_]+" CIM_CLASSESX ${MOF_CONTENT})
        foreach(CLASSX ${CIM_CLASSESX})
            string(REPLACE "\nclass " "" CLASS ${CLASSX})
            set(CIM_CLASS_NAMES ${CIM_CLASS_NAMES} ${CLASS})
        endforeach(CLASSX ${CIM_CLASSESX})
    endforeach(MOF in LISTS MOFS)


    list(LENGTH CIM_CLASS_NAMES LEN)
    if (${LEN} EQUAL 0)
        message(FATAL_ERROR "No class found in the MOF files ${MOFS}")
    else (${LEN} EQUAL 0)
        # Get headers and sources names from the list of CIM classes
        set(HEADERS "")
        set(PROVIDERS "")
        set(GENERATE_PROVIDERS "")
        set(NEW_PROVIDERS "")
        foreach(CLASS ${CIM_CLASS_NAMES})
            # Add generated header to the list
            set(HEADERS ${HEADERS} ${CLASS}.h)
            # Get name of the source file
            set(PROVIDER ${CLASS}Provider.c)
            # If the provider doesn't exist, generate it
            if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER})
                # Part of generating command - passed to konkret
                set(GENERATE_PROVIDERS ${GENERATE_PROVIDERS} -s ${CLASS})
                # List of freshly generated providers
                set(NEW_PROVIDERS ${NEW_PROVIDERS} ${PROVIDER})
            endif (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER})
            # Add provider source to the list
            set(PROVIDERS ${PROVIDERS} ${PROVIDER})
        endforeach(CLASS ${CIM_CLASS_NAMES})

        # Generate headers for CIM classes
        set(ENV{KONKRET_SCHEMA_DIR} "/usr/share/mof/cim-current")
        execute_process(COMMAND ${KONKRETCMPI_KONKRET}
                                ${KONKRET_MOF_FILES}
                                ${GENERATE_PROVIDERS}
                                ${CIM_CLASS_NAMES}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        RESULT_VARIABLE RES
                        OUTPUT_VARIABLE OUT
                        ERROR_VARIABLE ERR
                    )

        # Show error message when konkret fails
        if (NOT ${RES} EQUAL 0)
            message(FATAL_ERROR "KonkretCMPI failed: ${RES} ${ERR}")
        endif (NOT ${RES} EQUAL 0)

        # Move pregenerated sources for providers to source directory
        foreach(PROVIDER ${NEW_PROVIDERS})
            file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/${PROVIDER} ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER})
        endforeach(PROVIDER ${NEW_PROVIDERS})

        # Return to caller
        set(${CIM_HEADERS} ${HEADERS})
        set(${CIM_PROVIDERS} ${PROVIDERS})
        set(${CIM_CLASSES} ${CIM_CLASS_NAMES})
    endif (${LEN} EQUAL 0)
endmacro(konkretcmpi_generate MOF PROVIDERS HEADERS)

# This macro creates registration file from shared library
#
# @param[in] PROVIDER_NAME human-readable name of the provider
# @param[in] LIBRARY_NAME name of the library without lib prefix and .so suffix (same as for add_library)
# @param[in] MOF name of the MOF file
# @param[in] DEST destination directory where to install .reg file (use "" to skip installation)
# @param[in] ARGN optional varargs argument: path(s) to the profile(s) to be registered
#
macro(cim_registration PROVIDER_NAME LIBRARY_NAME MOF DEST)
    string(REPLACE ".mof" ".reg" REG ${MOF})
    # Create registration out of shared library
    add_custom_command(TARGET ${LIBRARY_NAME}
                       POST_BUILD
                       COMMAND ${KONKRETCMPI_KONKRETREG} lib${LIBRARY_NAME}.so > ${REG}
                       COMMENT "Generating .reg file from library for ${PROVIDER_NAME}"
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    # Install it
    if (NOT ${DEST} STREQUAL "")
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${REG} DESTINATION ${DEST})
    endif (NOT ${DEST} STREQUAL "")

    # Add custom target for registration
    find_file(MOF_FILE
              ${MOF}
              PATHS ${CMAKE_SOURCE_DIR}/mof/
    )
    set(PROFILE_DO_REG "false")
    if (NOT "${ARGN}" STREQUAL "")
        set(PROFILE_DO_REG "true")
    endif (NOT "${ARGN}" STREQUAL "")

    add_custom_target(register-${PROVIDER_NAME}
                      ${OPENLMI_MOF_REGISTER} -v ${OPENLMI_VERSION} register ${MOF_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${REG} && 
                      if ${PROFILE_DO_REG}\; then ${OPENLMI_MOF_REGISTER} --just-mofs -n root/interop -c tog-pegasus register ${ARGN}\; fi)
    add_custom_target(unregister-${PROVIDER_NAME}
                      ${OPENLMI_MOF_REGISTER} -v ${OPENLMI_VERSION} unregister ${MOF_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${REG} && 
                      if ${PROFILE_DO_REG}\; then ${OPENLMI_MOF_REGISTER} --just-mofs -n root/interop -c tog-pegasus unregister ${ARGN}\; fi)
endmacro(cim_registration)


# This macro creates mof file with instances to profile registration
#
# @param[in]  CIM_CLASS array of classes for which generate instances
# @param[in]  SKEL_MOF filename where is skeleton for instances
# @param[out] OUT_MOF filename where to write generated instances
#
# todo:
# check file exists
#
macro(profile_mof_generate SKEL_MOF OUT_MOF CIM_CLASSES)
    SET(INSTANCE_SKEL "")
    file(READ "${SKEL_MOF}" INSTANCE_SKEL)
    file(WRITE "${OUT_MOF}" "")
    foreach(CLASS ${CIM_CLASSES})
        string(REPLACE "\@CLASS\@" ${CLASS} INSTANCE "${INSTANCE_SKEL}")
        string(REPLACE "\@VERSION\@" ${OPENLMI_VERSION} INSTANCE "${INSTANCE}")
        file(APPEND "${OUT_MOF}" "${INSTANCE}")
    endforeach(CLASS ${CIM_CLASSES})
    message(STATUS "Generated profile mof ${OUT_MOF}")
endmacro(profile_mof_generate)
