#pragma once

#include <argus/common/config/argparse_policy.hpp>
#include <argus/common/config/config.hpp>
#include <argus/common/config/lidar.hpp>
#include <argus/common/config/mirror.hpp>
#include <argus/common/config/optimizer.hpp>
#include <argus/common/config/paths.hpp>
#include <argus/common/config/toml_policy.hpp>

#include <glaze/glaze.hpp>

namespace argus::common {

/// @brief Top-level configuration that aggregates all sub-configs.
struct ArgusConfig : public ConfigBase<ArgusConfig, TomlPolicy<ArgusConfig>,
                                       ArgparsePolicy<ArgusConfig>> {
  LidarConfig lidar;
  MirrorConfig mirror;
  OptimizerConfig optimiser;
  PathsConfig paths;
};

}  // namespace argus::common

template <>
struct glz::meta<argus::common::ArgusConfig> {
  using T = argus::common::ArgusConfig;
  static constexpr auto value =
      glz::object("lidar", &T::lidar, "mirror", &T::mirror, "optimiser",
                  &T::optimiser, "paths", &T::paths);
};
