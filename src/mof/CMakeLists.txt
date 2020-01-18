
install(FILES 05_LMI_Qualifiers.mof 30_LMI_Jobs.mof DESTINATION share/openlmi-providers/)

if (WITH-FAN)
    install(FILES 60_LMI_Fan.mof DESTINATION share/openlmi-providers/)
endif (WITH-FAN)

if (WITH-POWER)
    install(FILES 60_LMI_PowerManagement.mof DESTINATION share/openlmi-providers/)
endif (WITH-POWER)

if (WITH-SERVICE)
    install(FILES 60_LMI_Service.mof DESTINATION share/openlmi-providers/)
endif (WITH-SERVICE)

if (WITH-SERVICE-LEGACY)
    install(FILES 60_LMI_Service-legacy.mof DESTINATION share/openlmi-providers
            RENAME 60_LMI_Service.mof)
endif (WITH-SERVICE-LEGACY)

if (WITH-ACCOUNT)
    install(FILES 60_LMI_Account.mof DESTINATION share/openlmi-providers/)
endif (WITH-ACCOUNT)

if (WITH-HARDWARE)
    install(FILES 60_LMI_Hardware.mof DESTINATION share/openlmi-providers/)
endif (WITH-HARDWARE)

if (WITH-LOGICALFILE)
    install(FILES 50_LMI_LogicalFile.mof DESTINATION share/openlmi-providers/)
endif (WITH-LOGICALFILE)

if (WITH-REALMD)
    install(FILES 60_LMI_Realmd.mof DESTINATION share/openlmi-providers/)
endif (WITH-REALMD)

if (WITH-PCP)
    install(FILES 60_LMI_PCP.mof DESTINATION share/openlmi-providers/)
endif (WITH-PCP)

if (WITH-SOFTWARE)
    set(SW_DBUS_MOF "")
    if (WITH-SOFTWARE-DBUS)
        file(READ 60_LMI_Software-dbus.cmake SW_DBUS_MOF)
    else (WITH-SOFTWARE-DBUS)
        install(FILES LMI_Software.reg DESTINATION share/openlmi-providers/)
    endif (WITH-SOFTWARE-DBUS)
    configure_file(60_LMI_Software.mof.in ${CMAKE_BINARY_DIR}/mof/60_LMI_Software.mof)
    install(FILES "${CMAKE_BINARY_DIR}/mof/60_LMI_Software.mof"
                  60_LMI_Software_MethodParameters.mof
                  70_LMI_SoftwareIndicationFilters.mof
            DESTINATION share/openlmi-providers/)
endif (WITH-SOFTWARE)

if (WITH-JOURNALD)
    install(FILES 60_LMI_Journald.mof
                  70_LMI_JournaldIndicationFilters.mof
            DESTINATION share/openlmi-providers/)
endif (WITH-JOURNALD)

if (WITH-LOCALE)
    install(FILES 60_LMI_Locale.mof DESTINATION share/openlmi-providers/)
endif (WITH-LOCALE)

if (WITH-SSSD)
    install(FILES 60_LMI_SSSD.mof DESTINATION share/openlmi-providers/)
endif (WITH-SSSD)
