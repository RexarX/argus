set(ARGUS_C_CORE_HEADERS
    include/argus/assert.h
    include/argus/core.h
    include/argus/log.h
    include/argus/version.h
)

set(ARGUS_C_CORE_SOURCES
    src/assert.cpp
    src/log.cpp
    src/pch.hpp
    src/version.cpp
)

argus_add_c_module(
    NAME core
    CPP_TARGET argus::core
    HEADERS ${ARGUS_C_CORE_HEADERS}
    SOURCES ${ARGUS_C_CORE_SOURCES}
)

target_include_directories(argus_c_core
    PUBLIC
        $<BUILD_INTERFACE:${ARGUS_CORE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${ARGUS_CORE_GENERATED_INCLUDE_DIR}>
)

target_compile_definitions(argus_c_core PUBLIC
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ARGUS_ENABLE_ASSERT>
)
