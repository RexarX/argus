#include <doctest/doctest.h>

#include <argus/compiler.hpp>

TEST_SUITE("compiler::Compiler") {
  TEST_CASE("ARGUS_EXPECT_TRUE: branch prediction hint") {
    SUBCASE("True value") {
      int value = 1;
      if (ARGUS_EXPECT_TRUE(value == 1)) {
        CHECK(true);
      } else {
        CHECK(false);
      }
    }

    SUBCASE("False value") {
      int value = 0;
      if (ARGUS_EXPECT_TRUE(value == 1)) {
        CHECK(false);
      } else {
        CHECK(true);
      }
    }

    SUBCASE("Boolean true") {
      CHECK(ARGUS_EXPECT_TRUE(true));
    }

    SUBCASE("Boolean false") {
      CHECK_FALSE(ARGUS_EXPECT_TRUE(false));
    }

    SUBCASE("Expression evaluation") {
      int a = 5;
      int b = 5;
      CHECK(ARGUS_EXPECT_TRUE(a == b));
    }

    SUBCASE("Complex expression") {
      int x = 10;
      CHECK(ARGUS_EXPECT_TRUE(x > 5 && x < 20));
    }
  }

  TEST_CASE("ARGUS_EXPECT_FALSE: branch prediction hint") {
    SUBCASE("False value") {
      int value = 0;
      if (ARGUS_EXPECT_FALSE(value == 1)) {
        CHECK(false);
      } else {
        CHECK(true);
      }
    }

    SUBCASE("True value") {
      int value = 1;
      if (ARGUS_EXPECT_FALSE(value == 1)) {
        CHECK(true);
      } else {
        CHECK(false);
      }
    }

    SUBCASE("Boolean false") {
      CHECK_FALSE(ARGUS_EXPECT_FALSE(false));
    }

    SUBCASE("Boolean true") {
      CHECK(ARGUS_EXPECT_FALSE(true));
    }

    SUBCASE("Expression evaluation") {
      int a = 5;
      int b = 10;
      CHECK_FALSE(ARGUS_EXPECT_FALSE(a == b));
    }

    SUBCASE("Complex expression") {
      int x = 100;
      CHECK_FALSE(ARGUS_EXPECT_FALSE(x > 5 && x < 20));
    }
  }

  TEST_CASE("ARGUS_FORCE_INLINE: macro definition") {
    // We can't directly test if a function is inlined, but we can verify
    // the macro is defined and doesn't cause compilation errors
    SUBCASE("Macro is defined") {
#ifdef ARGUS_FORCE_INLINE
      CHECK(true);
#else
      CHECK(false);
#endif
    }
  }

  TEST_CASE("ARGUS_NO_INLINE: macro definition") {
    SUBCASE("Macro is defined") {
#ifdef ARGUS_NO_INLINE
      CHECK(true);
#else
      CHECK(false);
#endif
    }
  }

#ifdef ARGUS_MOVEONLY_FUNCTION_AVAILABLE
  TEST_CASE(
      "ARGUS_MOVEONLY_FUNCTION_AVAILABLE: feature "
      "detection") {
    SUBCASE("Move-only function is available") {
      CHECK(true);
    }
  }
#endif

#ifdef ARGUS_CONTAINERS_RANGES_AVAILABLE
  TEST_CASE(
      "ARGUS_CONTAINERS_RANGES_AVAILABLE: feature "
      "detection") {
    SUBCASE("Container ranges are available") {
      CHECK(true);
    }
  }
#endif

#ifdef ARGUS_STL_FLAT_MAP_AVAILABLE
  TEST_CASE("ARGUS_STL_FLAT_MAP_AVAILABLE: feature detection") {
    SUBCASE("STL flat_map is available") {
      CHECK(true);
    }
  }
#endif

  TEST_CASE("Compiler feature macros: consistency") {
    SUBCASE("All macros expand to valid expressions") {
      // Verify macros can be used in conditional expressions
      const bool expect_true_result = ARGUS_EXPECT_TRUE(1);
      const bool expect_false_result = ARGUS_EXPECT_FALSE(0);

      CHECK(expect_true_result);
      CHECK_FALSE(expect_false_result);
    }

    SUBCASE("Macros work with pointer types") {
      int value = 42;
      int* ptr = &value;
      int* null_ptr = nullptr;

      CHECK(ARGUS_EXPECT_TRUE(ptr != nullptr));
      // ARGUS_EXPECT_FALSE returns the boolean value as-is, just with a branch
      // hint So (null_ptr == nullptr) is true, and the macro returns true
      CHECK(ARGUS_EXPECT_FALSE(null_ptr == nullptr));
      CHECK_FALSE(ARGUS_EXPECT_FALSE(ptr == nullptr));
    }

    SUBCASE("Macros work with function return values") {
      auto always_true = []() { return true; };
      auto always_false = []() { return false; };

      CHECK(ARGUS_EXPECT_TRUE(always_true()));
      CHECK_FALSE(ARGUS_EXPECT_FALSE(always_false()));
    }
  }

  TEST_CASE("Branch prediction macros: control flow") {
    SUBCASE("ARGUS_EXPECT_TRUE in if-else chain") {
      int counter = 0;
      const bool likely_true = true;

      if (ARGUS_EXPECT_TRUE(likely_true)) {
        counter = 1;
      } else {
        counter = 2;
      }

      CHECK_EQ(counter, 1);
    }

    SUBCASE("ARGUS_EXPECT_FALSE in if-else chain") {
      int counter = 0;
      const bool likely_false = false;

      if (ARGUS_EXPECT_FALSE(likely_false)) {
        counter = 1;
      } else {
        counter = 2;
      }

      CHECK_EQ(counter, 2);
    }

    SUBCASE("Nested branch predictions") {
      int result = 0;
      const int x = 10;
      const int y = 20;

      if (ARGUS_EXPECT_TRUE(x > 0)) {
        if (ARGUS_EXPECT_TRUE(y > 0)) {
          if (ARGUS_EXPECT_FALSE(x > y)) {
            result = 1;
          } else {
            result = 2;
          }
        }
      }

      CHECK_EQ(result, 2);
    }

    SUBCASE("Branch prediction with logical operators") {
      const bool a = true;
      const bool b = true;
      const bool c = false;

      CHECK(ARGUS_EXPECT_TRUE(a && b));
      CHECK_FALSE(ARGUS_EXPECT_TRUE(a && c));
      CHECK(ARGUS_EXPECT_TRUE(a || c));
      CHECK_FALSE(ARGUS_EXPECT_FALSE(c));
    }

    SUBCASE("Branch prediction in ternary operator") {
      const int value = ARGUS_EXPECT_TRUE(5 > 3) ? 100 : 200;
      CHECK_EQ(value, 100);

      const int value2 = ARGUS_EXPECT_FALSE(5 < 3) ? 100 : 200;
      CHECK_EQ(value2, 200);
    }
  }

  TEST_CASE("Inline macros: function attributes") {
    // Define test functions to verify macros compile correctly
    struct TestFunctions {
      ARGUS_FORCE_INLINE static int ForceInlinedFunction(int x) {
        return x * 2;
      }

      ARGUS_NO_INLINE static int NoInlineFunction(int x) { return x * 3; }
    };

    SUBCASE("Force inlined function works correctly") {
      CHECK_EQ(TestFunctions::ForceInlinedFunction(5), 10);
      CHECK_EQ(TestFunctions::ForceInlinedFunction(0), 0);
      CHECK_EQ(TestFunctions::ForceInlinedFunction(-3), -6);
    }

    SUBCASE("No inline function works correctly") {
      CHECK_EQ(TestFunctions::NoInlineFunction(5), 15);
      CHECK_EQ(TestFunctions::NoInlineFunction(0), 0);
      CHECK_EQ(TestFunctions::NoInlineFunction(-3), -9);
    }
  }

  TEST_CASE("Compiler detection: platform-specific behavior") {
#if defined(__GNUC__) || defined(__clang__)
    SUBCASE("GCC/Clang detected") {
      // On GCC/Clang, ARGUS_EXPECT_* should use __builtin_expect
      CHECK(true);
    }
#elif defined(_MSC_VER)
    SUBCASE("MSVC detected") {
      // On MSVC, ARGUS_EXPECT_* should be identity macros
      CHECK(true);
    }
#else
    SUBCASE("Unknown compiler") {
      // On unknown compilers, macros should still work as identity
      CHECK(true);
    }
#endif
  }

}  // TEST_SUITE
