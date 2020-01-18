# Array of names of provider directories.
set(PROVIDERS "")

# Array of names of the providers
set(PROVIDER_CAPTIONS "")

# Array of MOF files to generate class references from
set(PROVIDER_MOFS "")

# Collect properties of enabled providers
if (WITH-ACCOUNT)
    set(PROVIDERS ${PROVIDERS} "account")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "Account")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "60_LMI_Account.mof")
endif (WITH-ACCOUNT)

if (WITH-FAN)
    set(PROVIDERS ${PROVIDERS} "fan")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "Fan")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "60_LMI_Fan.mof")
endif (WITH-FAN)

if (WITH-HARDWARE)
    set(PROVIDERS ${PROVIDERS} "hardware")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "Hardware")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "60_LMI_Hardware.mof")
endif (WITH-HARDWARE)

if (WITH-JOURNALD)
    set(PROVIDERS ${PROVIDERS} "journald")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "journald")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "60_LMI_Journald.mof")
endif (WITH-JOURNALD)

if (WITH-LOGICALFILE)
    set(PROVIDERS ${PROVIDERS} "logicalfile")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "Logical File")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "50_LMI_LogicalFile.mof")
endif (WITH-LOGICALFILE)

if (WITH-POWER)
    set(PROVIDERS ${PROVIDERS} "power")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "Power Management")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "60_LMI_PowerManagement.mof")
endif (WITH-POWER)

if (WITH-REALMD)
    set(PROVIDERS ${PROVIDERS} "realmd")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "Realmd")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "60_LMI_Realmd.mof")
endif (WITH-REALMD)

if (WITH-SERVICE)
    set(PROVIDERS ${PROVIDERS} "service-dbus")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "Service")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "60_LMI_Service.mof")
endif (WITH-SERVICE)

if (WITH-SOFTWARE)
    set(PROVIDERS ${PROVIDERS} "software")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "Software")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "${CMAKE_BINARY_DIR}/mof/60_LMI_Software.mof")
endif (WITH-SOFTWARE)

if (WITH-LOCALE)
    set(PROVIDERS ${PROVIDERS} "locale")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "Locale")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "60_LMI_Locale.mof")
endif (WITH-LOCALE)

if (WITH-SSSD)
    set(PROVIDERS ${PROVIDERS} "sssd")
    set(PROVIDER_CAPTIONS ${PROVIDER_CAPTIONS} "SSSD")
    set(PROVIDER_MOFS ${PROVIDER_MOFS} "60_LMI_SSSD.mof")
endif (WITH-SSSD)

list(LENGTH PROVIDERS LEN)
math(EXPR LEN '${LEN}-1')

set(DOC_DEPS "")

find_program(PLANTUML plantuml)

# For each provider, define these targets:
# PROVIDER-doc-mof-dir - creates all directories
# PROVIDER-doc-mof - create .rst from .mof
# PROVIDER-doc-pic - create .svg from .uml (if needed)
# PROVIDER-doc - create .html from .rst
foreach(I RANGE ${LEN})
    list(GET PROVIDERS ${I} PROVIDER)
    list(GET PROVIDER_CAPTIONS ${I} CAPTION)
    list(GET PROVIDER_MOFS ${I} MOF)
    add_custom_target(${PROVIDER}-doc-mof-dir
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${PROVIDER}/mof
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${PROVIDER}/pic
    )

    if(IS_ABSOLUTE ${MOF})
        set(MOF_PATH "${MOF}")
    else(IS_ABSOLUTE ${MOF})
        set(MOF_PATH "${CMAKE_SOURCE_DIR}/mof/${MOF}")
    endif(IS_ABSOLUTE ${MOF})

    add_custom_target(${PROVIDER}-doc-mof
        COMMAND $(CMAKE_SOURCE_DIR)/tools/openlmi-doc-class2rst
                -H ${CMAKE_CURRENT_SOURCE_DIR}/header.txt
                --schema /usr/share/mof/cim-current/CIM_Schema.mof
                --schema ${OPENLMI_QUALIFIERS_MOF}
                --schema ${OPENLMI_JOBS_MOF}
                --mof ${MOF_PATH}
                CIM_ComputerSystem
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROVIDER}/mof
    )
    add_dependencies(${PROVIDER}-doc-mof
        ${PROVIDER}-doc-mof-dir
    )

    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER}/pic)
        if (PLANTUML AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER}/pic/make-svg.sh)
            # We have plantuml, svgs will be regenerated
            add_custom_target(${PROVIDER}-doc-pic-gen
                    COMMENT "Regenerating pictures using ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER}/pic/make-svg.sh"
                    COMMAND ./make-svg.sh
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER}/pic
                )
        else (PLANTUML AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER}/pic/make-svg.sh)
            # We don't have plantuml, so we depend on svgs in tarball
            add_custom_target(${PROVIDER}-doc-pic-gen)
        endif (PLANTUML AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER}/pic/make-svg.sh)

        add_custom_target(${PROVIDER}-doc-pic
            # copy the pictures
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER}/pic/*.svg ${CMAKE_CURRENT_BINARY_DIR}/${PROVIDER}/pic/
        )
        add_dependencies(${PROVIDER}-doc-pic
            ${PROVIDER}-doc-mof-dir ${PROVIDER}-doc-pic-gen
        )

    else (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER}/pic)
        add_custom_target(${PROVIDER}-doc-pic)
    endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER}/pic)

    add_custom_target(${PROVIDER}-doc
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/${PROVIDER}/*.rst ${CMAKE_CURRENT_BINARY_DIR}/${PROVIDER}

        COMMAND sphinx-build
                    -b html
                    -c ${CMAKE_CURRENT_SOURCE_DIR}
                    -D "version=${OPENLMI_VERSION}" -D "release=${OPENLMI_VERSION}" -D "project=OpenLMI ${CAPTION} provider" -D "epub_title=OpenLMI ${CAPTION} provider"
                    ${CMAKE_CURRENT_BINARY_DIR}/${PROVIDER} ${CMAKE_CURRENT_BINARY_DIR}/${PROVIDER}/html
    )
    add_dependencies(${PROVIDER}-doc
        ${PROVIDER}-doc-mof ${PROVIDER}-doc-pic
    )
    set(DOC_DEPS ${DOC_DEPS} ${PROVIDER}-doc)
endforeach(I RANGE ${LEN})

# Finally, create 'doc' target to run all the previous *doc targets
add_custom_target(doc)
add_dependencies(doc ${DOC_DEPS})
