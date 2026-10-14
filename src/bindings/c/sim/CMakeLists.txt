set(ARGUS_C_SIM_HEADERS
    include/argus/sim.h
)

set(ARGUS_C_SIM_SOURCES
    src/sim.cpp
    src/pch.hpp
)

argus_add_c_module(
    NAME sim
    CPP_TARGET argus_sim
    HEADERS ${ARGUS_C_SIM_HEADERS}
    SOURCES ${ARGUS_C_SIM_SOURCES}
)
