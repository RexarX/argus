# Main dependency configuration for Argus
#
# This file provides:
# - Initial setup and status messages
# - Test dependencies (loaded here since tests are built)
# - Summary printing at the end of configuration
#
# Strategy: System packages -> CPM fallback

include_guard(GLOBAL)

# Load dependency management helpers
include(DependencyFinder)
include(DownloadUsingCPM)

# Print configuration header
message(STATUS "")
message(STATUS "========== Argus Dependency Configuration ==========")
message(STATUS "  → Dependencies loaded on-demand")
message(STATUS "  → System packages checked FIRST")
message(STATUS "  → CPM downloads for missing dependencies")
message(STATUS "Allow CPM downloads: ${ARGUS_DOWNLOAD_PACKAGES}")
message(STATUS "Check package versions: ${ARGUS_CHECK_PACKAGE_VERSIONS}")
message(STATUS "=====================================================")
message(STATUS "")

if(BUILD_TESTING)
  message(STATUS "Finding Test Dependencies...")
  message(STATUS "")

  argus_require_dependency(doctest)

  message(STATUS "")
endif()

function(argus_print_dependency_summary)
  argus_print_dependencies()
  if(CPM_PACKAGES)
    argus_print_cpm_packages()
  endif()
  message(STATUS "")
endfunction()
