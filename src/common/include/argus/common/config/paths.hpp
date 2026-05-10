#pragma once

#include <argus/common/config/argparse_policy.hpp>
#include <argus/common/config/config.hpp>
#include <argus/common/config/toml_policy.hpp>

#include <optional>
#include <string>
#include <string_view>

#include <glaze/glaze.hpp>

namespace argus::common {

/// @brief Configuration for the paths.
struct PathsConfig : public ConfigBase<PathsConfig, TomlPolicy<PathsConfig>,
                                       ArgparsePolicy<PathsConfig>> {
  static constexpr std::string_view kDefaultOutputDir = "output";

  /// Path to the scene file (OBJ, GLTF, etc.) relative to the config file
  std::optional<std::string> scene_file;
  /// Directory where result files are written
  std::optional<std::string> output_directory;
};

}  // namespace argus::common

template <>
struct glz::meta<argus::common::PathsConfig> {
  using T = argus::common::PathsConfig;
  static constexpr auto value = glz::object(
      "scene_file", &T::scene_file, "output_directory", &T::output_directory);
};
