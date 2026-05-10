#pragma once

#include <argus/common/config/argparse_policy.hpp>
#include <argus/common/config/config.hpp>
#include <argus/common/config/toml_policy.hpp>

#include <cstdint>
#include <optional>

#include <glaze/glaze.hpp>

namespace argus::common {

/// @brief Configuration for the optimizer.
struct OptimizerConfig
    : public ConfigBase<OptimizerConfig, TomlPolicy<OptimizerConfig>,
                        ArgparsePolicy<OptimizerConfig>> {
  static constexpr uint32_t kDefaultMaxIterations = 1000;
  static constexpr float kDefaultConvergenceThreshold = 1e-4F;
  static constexpr uint32_t kDefaultPopulationSize = 64;
  static constexpr uint32_t kDefaultSeed = 0;

  /// Maximum number of iterations before early exit
  std::optional<uint32_t> max_iterations;
  /// Convergence threshold - stop when improvement drops below this ratio
  std::optional<float> convergence_threshold;
  /// Number of parallel candidate positions evaluated per iteration
  std::optional<uint32_t> population_size;
  /// RNG seed (0 = non-deterministic)
  std::optional<uint32_t> seed;
};

}  // namespace argus::common

template <>
struct glz::meta<argus::common::OptimizerConfig> {
  using T = argus::common::OptimizerConfig;
  static constexpr auto value =
      glz::object("max_iterations", &T::max_iterations, "convergence_threshold",
                  &T::convergence_threshold, "population_size",
                  &T::population_size, "seed", &T::seed);
};
