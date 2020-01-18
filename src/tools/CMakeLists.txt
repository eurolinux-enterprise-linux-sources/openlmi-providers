
install(PROGRAMS openlmi-doc-class2rst openlmi-doc-class2uml DESTINATION bin)

if (WITH-DEVASSISTANT)
    install(DIRECTORY devassistant DESTINATION share USE_SOURCE_PERMISSIONS)
endif (WITH-DEVASSISTANT)
