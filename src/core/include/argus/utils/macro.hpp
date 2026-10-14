#pragma once

#define ARGUS_STRINGIFY_IMPL(x) #x
#define ARGUS_STRINGIFY(x) ARGUS_STRINGIFY_IMPL(x)

#define ARGUS_CONCAT_IMPL(a, b) a##b
#define ARGUS_CONCAT(a, b) ARGUS_CONCAT_IMPL(a, b)

#define ARGUS_ANONYMOUS_VAR(prefix) ARGUS_CONCAT(prefix, __LINE__)
