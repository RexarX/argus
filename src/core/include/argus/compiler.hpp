#pragma once

#include <argus/compiler.h>

#ifdef __clang__
#define ARGUS_ALWAYS_INLINE [[clang::always_inline]]
#elif defined(__GNUC__)
#define ARGUS_ALWAYS_INLINE [[gnu::always_inline]]
#elif defined(_MSC_VER)
#define ARGUS_ALWAYS_INLINE [[msvc::forceinline]]
#else
#define ARGUS_ALWAYS_INLINE
#endif

#ifdef _MSC_VER
#define ARGUS_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define ARGUS_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
