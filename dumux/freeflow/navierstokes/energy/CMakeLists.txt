file(GLOB DUMUX_FREEFLOW_NAVIERSTOKES_ENERGY_HEADERS *.hh *.inc)
install(FILES ${DUMUX_FREEFLOW_NAVIERSTOKES_ENERGY_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dumux/freeflow/navierstokes/energy)
