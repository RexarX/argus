#pragma once

#define ARGUS_BIT(x) (1U << (x))

// Stringify macros
#define ARGUS_STRINGIFY_IMPL(x) #x
#define ARGUS_STRINGIFY(x) ARGUS_STRINGIFY_IMPL(x)

// Concatenation macros
#define ARGUS_CONCAT_IMPL(a, b) a##b
#define ARGUS_CONCAT(a, b) ARGUS_CONCAT_IMPL(a, b)

// Anonymous variable generation
#define ARGUS_ANONYMOUS_VAR(prefix) ARGUS_CONCAT(prefix, __LINE__)
