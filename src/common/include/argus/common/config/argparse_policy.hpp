#pragma once

#include <argus/common/config/config.hpp>
#include <argus/common/config/policy.hpp>

namespace argus::common {

/**
 * @brief Parse command-line arguments into a `ConfigT` using argparse.
 * @tparam ConfigT The configuration type to parse
 */
template <typename ConfigT>
struct ArgparsePolicy {
  /**
   * @brief Parse command-line arguments into a `ConfigT` using argparse.
   * @param argc The number of command-line arguments
   * @param argv The command-line arguments
   * @return `ConfigResult` containing the parsed configuration or an error
   */
  [[nodiscard]] static auto Parse(int argc, char* argv[])
      -> ConfigResult<ConfigT>;
};

}  // namespace argus::common
