# Installation configuration for argus
#
# Requires ARGUS_ENABLE_INSTALL=ON.
#
# `argus::argus` is the C++ umbrella over core, algo, sim, and common
# (each when enabled). C bindings export as argus::c / argus::c::* when
# ARGUS_BUILD_C_BINDINGS is ON. Shared builds produce one DLL; static builds
# keep separate archives and the glue is INTERFACE. Headers install under
# include/argus/.

include_guard(GLOBAL)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(_argus_export_targets argus)

if(NOT ARGUS_BUILD_SHARED)
  list(APPEND _argus_export_targets argus_core)
  if(TARGET argus_algo)
    list(APPEND _argus_export_targets argus_algo)
  endif()
  if(TARGET argus_sim)
    list(APPEND _argus_export_targets argus_sim)
  endif()
  if(TARGET argus_common)
    list(APPEND _argus_export_targets argus_common)
  endif()
  get_property(_argus_c_targets GLOBAL PROPERTY ARGUS_C_TARGETS)
  if(_argus_c_targets)
    list(APPEND _argus_export_targets ${_argus_c_targets})
  endif()
endif()

if(TARGET argus_c_argus)
  list(APPEND _argus_export_targets argus_c_argus)
endif()

if(TARGET argus_c)
  list(APPEND _argus_export_targets argus_c)
endif()

set(_argus_runtime_targets "")
set(_argus_interface_targets "")
foreach(_target IN LISTS _argus_export_targets)
  get_target_property(_type ${_target} TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY" OR _type STREQUAL "INTERFACE")
    list(APPEND _argus_interface_targets ${_target})
  else()
    list(APPEND _argus_runtime_targets ${_target})
  endif()
endforeach()

if(_argus_runtime_targets)
  install(TARGETS ${_argus_runtime_targets}
      EXPORT argusTargets
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()

if(_argus_interface_targets)
  install(TARGETS ${_argus_interface_targets}
      EXPORT argusTargets
  )
endif()

if(TARGET eigen)
  install(DIRECTORY "${ARGUS_ROOT_DIR}/third-party/eigen/Eigen"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(TARGETS eigen EXPORT argusTargets)
endif()

if(TARGET argus_common AND TARGET assimp)
  set(_argus_assimp_export_deps assimp)
  if(TARGET zlibstatic)
    list(APPEND _argus_assimp_export_deps zlibstatic)
  endif()
  install(TARGETS ${_argus_assimp_export_deps} EXPORT argusTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

install(EXPORT argusTargets
    FILE argusTargets.cmake
    NAMESPACE argus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/argus
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/argusConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

if(ARGUS_ENABLE_EMBREE_BACKEND)
  get_property(ARGUS_EMBREE_RUNTIME_NAMES GLOBAL PROPERTY ARGUS_EMBREE_RUNTIME_NAMES)
  get_property(ARGUS_EMBREE_PACKAGE_DIR GLOBAL PROPERTY ARGUS_EMBREE_PACKAGE_DIR)
else()
  set(ARGUS_EMBREE_RUNTIME_NAMES "")
  set(ARGUS_EMBREE_PACKAGE_DIR "")
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/argusConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/argusConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/argus
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/argusConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/argusConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/argus
)

if(ARGUS_ENABLE_EMBREE_BACKEND)
  argus_install_embree()
endif()
