#include <doctest/doctest.h>

#include <argus/utils/macro.hpp>

#include <array>
#include <string>

TEST_SUITE("utils::Macro") {
  TEST_CASE("utils::ARGUS_BIT: bit shifting macro") {
    SUBCASE("Bit 0") {
      CHECK_EQ(ARGUS_BIT(0), 1);
    }

    SUBCASE("Bit 1") {
      CHECK_EQ(ARGUS_BIT(1), 2);
    }

    SUBCASE("Bit 2") {
      CHECK_EQ(ARGUS_BIT(2), 4);
    }

    SUBCASE("Bit 3") {
      CHECK_EQ(ARGUS_BIT(3), 8);
    }

    SUBCASE("Bit 4") {
      CHECK_EQ(ARGUS_BIT(4), 16);
    }

    SUBCASE("Bit 7") {
      CHECK_EQ(ARGUS_BIT(7), 128);
    }

    SUBCASE("Bit 8") {
      CHECK_EQ(ARGUS_BIT(8), 256);
    }

    SUBCASE("Bit 15") {
      CHECK_EQ(ARGUS_BIT(15), 32768);
    }

    SUBCASE("Bit 16") {
      CHECK_EQ(ARGUS_BIT(16), 65536);
    }

    SUBCASE("Power of two relationship") {
      for (int i = 0; i < 16; ++i) {
        CHECK_EQ(ARGUS_BIT(i), 1 << i);
      }
    }

    SUBCASE("Usage in bitmask") {
      constexpr int kFlagA = ARGUS_BIT(0);
      constexpr int kFlagB = ARGUS_BIT(1);
      constexpr int kFlagC = ARGUS_BIT(2);

      int flags = kFlagA | kFlagC;

      CHECK((flags & kFlagA) != 0);
      CHECK((flags & kFlagB) == 0);
      CHECK((flags & kFlagC) != 0);
    }
  }

  TEST_CASE("utils::ARGUS_STRINGIFY: stringification macro") {
    SUBCASE("Stringify integer literal") {
      const char* str = ARGUS_STRINGIFY(42);
      CHECK_EQ(std::string(str), "42");
    }

    SUBCASE("Stringify identifier") {
      const char* str = ARGUS_STRINGIFY(hello_world);
      CHECK_EQ(std::string(str), "hello_world");
    }

    SUBCASE("Stringify expression") {
      const char* str = ARGUS_STRINGIFY(1 + 2);
      CHECK_EQ(std::string(str), "1 + 2");
    }

    SUBCASE("Stringify type") {
      const char* str = ARGUS_STRINGIFY(int);
      CHECK_EQ(std::string(str), "int");
    }

    SUBCASE("Stringify template-like syntax") {
      const char* str = ARGUS_STRINGIFY(std::vector<int>);
      CHECK_EQ(std::string(str), "std::vector<int>");
    }

    SUBCASE("Stringify with parentheses") {
      const char* str = ARGUS_STRINGIFY((a, b, c));
      CHECK_EQ(std::string(str), "(a, b, c)");
    }

    SUBCASE("Stringify macro argument") {
#define TEST_VALUE 123
      const char* str = ARGUS_STRINGIFY(TEST_VALUE);
      // ARGUS_STRINGIFY should expand the macro first via
      // ARGUS_STRINGIFY_IMPL
      CHECK_EQ(std::string(str), "123");
#undef TEST_VALUE
    }
  }

  TEST_CASE("utils::ARGUS_CONCAT: concatenation macro") {
    SUBCASE("Concatenate identifiers to form variable name") {
      // ARGUS_CONCAT joins tokens together
      int ARGUS_CONCAT(test_, var) = 42;
      CHECK_EQ(test_var, 42);
    }

    SUBCASE("Concatenate to form function name") {
      auto ARGUS_CONCAT(get_, value) = []() { return 100; };
      CHECK_EQ(get_value(), 100);
    }

    SUBCASE("Concatenate numbers") {
      constexpr int ARGUS_CONCAT(var, 1) = 10;
      constexpr int ARGUS_CONCAT(var, 2) = 20;
      constexpr int ARGUS_CONCAT(var, 3) = 30;

      CHECK_EQ(var1, 10);
      CHECK_EQ(var2, 20);
      CHECK_EQ(var3, 30);
    }

    SUBCASE("Concatenate with underscore") {
      int ARGUS_CONCAT(my, _variable) = 99;
      CHECK_EQ(my_variable, 99);
    }
  }

  TEST_CASE("utils::ARGUS_ANONYMOUS_VAR: anonymous variable generation") {
    SUBCASE("Creates unique variables on different lines") {
      // Each ARGUS_ANONYMOUS_VAR on a different line should create a unique
      // variable
      [[maybe_unused]] int ARGUS_ANONYMOUS_VAR(test_) = 1;
      [[maybe_unused]] int ARGUS_ANONYMOUS_VAR(test_) = 2;
      [[maybe_unused]] int ARGUS_ANONYMOUS_VAR(test_) = 3;

      // If they were the same name, this wouldn't compile
      CHECK(true);
    }

    SUBCASE("Variable is usable") {
      [[maybe_unused]] int ARGUS_ANONYMOUS_VAR(counter_) = 42;
      // We can use the variable by knowing the line number, but typically
      // anonymous variables are meant to be unused after initialization
      CHECK(true);
    }

    SUBCASE("Works with different prefixes") {
      [[maybe_unused]] int ARGUS_ANONYMOUS_VAR(a_) = 1;
      [[maybe_unused]] float ARGUS_ANONYMOUS_VAR(b_) = 2.0f;
      [[maybe_unused]] double ARGUS_ANONYMOUS_VAR(c_) = 3.0;

      CHECK(true);
    }

    SUBCASE("Useful for RAII guards") {
      int counter = 0;

      struct Guard {
        int& ref;
        explicit Guard(int& r) : ref(r) { ++ref; }
        ~Guard() { ++ref; }
      };

      CHECK_EQ(counter, 0);
      {
        [[maybe_unused]] Guard ARGUS_ANONYMOUS_VAR(guard_)(counter);
        CHECK_EQ(counter, 1);
      }
      CHECK_EQ(counter, 2);
    }
  }

  TEST_CASE("utils::ARGUS_BIT: constexpr usage") {
    SUBCASE("Can be used in constexpr context") {
      constexpr int bit0 = ARGUS_BIT(0);
      constexpr int bit5 = ARGUS_BIT(5);
      constexpr int bit10 = ARGUS_BIT(10);

      static_assert(bit0 == 1, "Bit 0 should be 1");
      static_assert(bit5 == 32, "Bit 5 should be 32");
      static_assert(bit10 == 1024, "Bit 10 should be 1024");

      CHECK_EQ(bit0, 1);
      CHECK_EQ(bit5, 32);
      CHECK_EQ(bit10, 1024);
    }

    SUBCASE("Can be used in template arguments") {
      std::array<int, ARGUS_BIT(3)> arr;
      CHECK_EQ(arr.size(), 8);
    }

    SUBCASE("Can be used in switch case") {
      int value = 4;
      int result = 0;

      switch (value) {
        case ARGUS_BIT(0):
          result = 1;
          break;
        case ARGUS_BIT(1):
          result = 2;
          break;
        case ARGUS_BIT(2):
          result = 3;
          break;
        default:
          result = 0;
          break;
      }

      CHECK_EQ(result, 3);
    }
  }

  TEST_CASE("utils::Macro combinations") {
    SUBCASE("STRINGIFY and CONCAT together") {
      const char* str = ARGUS_STRINGIFY(ARGUS_CONCAT(hello, _world));
      // The inner CONCAT should be expanded first
      CHECK_EQ(std::string(str), "hello_world");
    }

    SUBCASE("BIT in expressions") {
      constexpr int flags = ARGUS_BIT(0) | ARGUS_BIT(2) | ARGUS_BIT(4);
      CHECK_EQ(flags, 1 + 4 + 16);
      CHECK_EQ(flags, 21);
    }
  }

  TEST_CASE("utils::ARGUS_STRINGIFY_IMPL: direct usage") {
    SUBCASE("Stringify without macro expansion") {
      const char* str = ARGUS_STRINGIFY_IMPL(test);
      CHECK_EQ(std::string(str), "test");
    }
  }

  TEST_CASE("utils::ARGUS_CONCAT_IMPL: direct usage") {
    SUBCASE("Concatenate directly") {
      int ARGUS_CONCAT_IMPL(direct_, concat) = 999;
      CHECK_EQ(direct_concat, 999);
    }
  }

}  // TEST_SUITE
