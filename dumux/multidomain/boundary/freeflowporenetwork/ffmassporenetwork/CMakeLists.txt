file(GLOB DUMUX_MULTIDOMAIN_BOUNDARY_FREEFLOWPORENETWORK_FFMASSPORENETWORK_HEADERS *.hh *.inc)
install(FILES ${DUMUX_MULTIDOMAIN_BOUNDARY_FREEFLOWPORENETWORK_FFMASSPORENETWORK_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dumux/multidomain/boundary/freeflowporenetwork/ffmassporenetwork)
