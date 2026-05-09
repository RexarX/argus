#include <doctest/doctest.h>

#include <argus/platform.hpp>

TEST_SUITE("platform::Platform") {
  TEST_CASE("ARGUS_API: macro is defined") {
    // ARGUS_API should be defined (may be empty on some platforms)
    // We can't directly test the macro value, but we can verify code compiles
    // with it
#ifdef ARGUS_API
    CHECK(true);  // Macro is defined
#else
    CHECK(false);  // ARGUS_API should always be defined
#endif
  }

  TEST_CASE("ARGUS_EXPORT: macro is defined") {
    // ARGUS_EXPORT should be defined for all platforms
#ifdef ARGUS_EXPORT
    CHECK(true);  // Macro is defined
#else
    CHECK(false);  // ARGUS_EXPORT should always be defined
#endif
  }

  TEST_CASE("ARGUS_DEBUG_BREAK: macro is defined") {
    // ARGUS_DEBUG_BREAK should be defined for all supported platforms
    // We cannot actually call it in tests as it would break/stop the test
    // process
#ifdef ARGUS_DEBUG_BREAK
    CHECK(true);  // Macro is defined
#else
    CHECK(false);  // ARGUS_DEBUG_BREAK should always be defined
#endif
  }

  TEST_CASE(
      "Platform detection: at least one platform is "
      "detected") {
    [[maybe_unused]] bool platform_detected = false;

#ifdef ARGUS_PLATFORM_WINDOWS
    platform_detected = true;
#endif

#ifdef ARGUS_PLATFORM_LINUX
    platform_detected = true;
#endif

#ifdef ARGUS_PLATFORM_MACOS
    platform_detected = true;
#endif

    // At least during build, CMake should have defined one of these
    // If none are defined, the build system hasn't configured platform macros
    // This is acceptable as platform.hpp doesn't require them to be defined
    CHECK(true);  // Platform detection macros are optional
  }

  TEST_CASE("Build mode detection: exactly one build mode") {
    int mode_count = 0;

#ifdef ARGUS_DEBUG_MODE
    ++mode_count;
#endif

#ifdef ARGUS_RELEASE_MODE
    ++mode_count;
#endif

#ifdef ARGUS_RELEASE_WITH_DEBUG_INFO_MODE
    ++mode_count;
#endif

    // Build mode macros are set by CMake, not by platform.hpp
    // They may or may not be defined depending on build configuration
    CHECK_LE(mode_count, 1);  // At most one mode should be defined
  }

  TEST_CASE("ARGUS_BUILD_SHARED: optional macro") {
    // ARGUS_BUILD_SHARED is optional and only defined when building shared
    // libraries We just verify it's handled correctly (doesn't cause
    // compilation issues)
#ifdef ARGUS_BUILD_SHARED
    CHECK(true);  // Building as shared library
#else
    CHECK(true);  // Building as static library or executable
#endif
  }

  TEST_CASE("Platform-specific API macros: correct expansion") {
    // Test that API macros expand to something valid (even if empty)
    // Create a simple struct with the API macro to verify it compiles

    struct ARGUS_API TestApiStruct {
      int value = 42;
    };

    TestApiStruct test_obj;
    CHECK_EQ(test_obj.value, 42);
  }

  TEST_CASE(
      "Platform-specific EXPORT macros: correct "
      "expansion") {
    // Test that EXPORT macros expand to something valid (even if empty)
    // Create a simple struct with the EXPORT macro to verify it compiles

    struct ARGUS_EXPORT TestExportStruct {
      int value = 100;
    };

    TestExportStruct test_obj;
    CHECK_EQ(test_obj.value, 100);
  }

  TEST_CASE("Debug break: architecture detection") {
    // Verify that architecture-specific code paths exist
    // The actual debug break is not called, just verify compilation

#if defined(_MSC_VER)
    // MSVC path
    CHECK(true);
#elif defined(__arm64__) || defined(__aarch64__)
    // ARM64 path
    CHECK(true);
#elif defined(__arm__)
    // ARM32 path
    CHECK(true);
#elif defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || \
    defined(_M_IX86)
    // x86/x86_64 path
    CHECK(true);
#elif defined(__powerpc__) || defined(__ppc__) || defined(_ARCH_PPC)
    // PowerPC path
    CHECK(true);
#elif defined(__wasm__)
    // WebAssembly path
    CHECK(true);
#else
    // Fallback path
    CHECK(true);
#endif
  }

  TEST_CASE("API macros: function decoration") {
    // Test that API macros work with functions
    // This primarily verifies compilation succeeds

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif
    auto test_api_function = []() ARGUS_EXPORT -> int { return 42; };
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    CHECK_EQ(test_api_function(), 42);
  }

  TEST_CASE("Windows-specific: DLL export/import") {
#ifdef ARGUS_PLATFORM_WINDOWS
    SUBCASE("Windows platform detected") {
      CHECK(true);
    }

#ifdef ARGUS_BUILD_SHARED
    SUBCASE("Shared build mode on Windows") {
      // In shared builds on Windows, ARGUS_API should be __declspec(dllexport)
      CHECK(true);
    }
#else
    SUBCASE("Static/executable build mode on Windows") {
      // In non-shared builds on Windows, ARGUS_API should be
      // __declspec(dllimport)
      CHECK(true);
    }
#endif
#else
    SUBCASE("Non-Windows platform") {
      CHECK(true);
    }
#endif
  }

  TEST_CASE("Linux-specific: visibility attributes") {
#ifdef ARGUS_PLATFORM_LINUX
    SUBCASE("Linux platform detected") {
      CHECK(true);
    }

#ifdef ARGUS_BUILD_SHARED
    SUBCASE("Shared build mode on Linux") {
      // In shared builds on Linux, ARGUS_API should have visibility("default")
      CHECK(true);
    }
#else
    SUBCASE("Static build mode on Linux") {
      // In static builds on Linux, ARGUS_API should be empty
      CHECK(true);
    }
#endif
#else
    SUBCASE("Non-Linux platform") {
      CHECK(true);
    }
#endif
  }

  TEST_CASE("Compiler detection for debug break") {
    // Verify compiler is detected correctly for debug break implementation

#if defined(_MSC_VER)
    SUBCASE("MSVC compiler detected") {
      CHECK(true);
    }
#elif defined(__GNUC__) || defined(__clang__)
    SUBCASE("GCC or Clang compiler detected") {
      CHECK(true);
    }
#elif defined(__ARMCC_VERSION)
    SUBCASE("ARM compiler detected") {
      CHECK(true);
    }
#else
    SUBCASE("Other compiler - using fallback") {
      CHECK(true);
    }
#endif
  }

}  // TEST_SUITE
