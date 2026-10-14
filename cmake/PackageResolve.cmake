# Shared helpers for checking dependency versions.

include_guard(GLOBAL)

# VERSION meets MIN (unknown VERSION is not acceptable).
function(_argus_version_meets VERSION MIN OUT_VAR)
  if(NOT VERSION OR NOT MIN)
    set(${OUT_VAR} FALSE PARENT_SCOPE)
    return()
  endif()
  if(VERSION VERSION_LESS MIN)
    set(${OUT_VAR} FALSE PARENT_SCOPE)
  else()
    set(${OUT_VAR} TRUE PARENT_SCOPE)
  endif()
endfunction()
