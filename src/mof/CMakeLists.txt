
file(GLOB MOFS *.mof)

foreach(MOF ${MOFS})
    install(FILES ${MOF} DESTINATION share/openlmi-providers/)
endforeach(MOF ${MOFS})
