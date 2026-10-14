set(ARGUS_C_ALGO_HEADERS
    include/argus/algo.h
)

set(ARGUS_C_ALGO_SOURCES
    src/algo.cpp
    src/pch.hpp
)

argus_add_c_module(
    NAME algo
    CPP_TARGET argus_algo
    HEADERS ${ARGUS_C_ALGO_HEADERS}
    SOURCES ${ARGUS_C_ALGO_SOURCES}
)
