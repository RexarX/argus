#pragma once

#include <argus/common/config/argparse_policy.hpp>
#include <argus/common/config/config.hpp>
#include <argus/common/config/toml_policy.hpp>

#include <optional>

#include <glaze/glaze.hpp>

namespace argus::common {

/// @brief Configuration for the mirror.
struct MirrorConfig : public ConfigBase<MirrorConfig, TomlPolicy<MirrorConfig>,
                                        ArgparsePolicy<MirrorConfig>> {
  static constexpr float kDefaultWidth = 0.05F;
  static constexpr float kDefaultHeight = 0.05F;
  static constexpr float kDefaultTiltMin = -45.0F;
  static constexpr float kDefaultTiltMax = 45.0F;
  static constexpr float kDefaultPanMin = -180.0F;
  static constexpr float kDefaultPanMax = 180.0F;

  /// Mirror width in metres
  std::optional<float> width;
  /// Mirror height in metres
  std::optional<float> height;
  /// Minimum allowed tilt angle in degrees (elevation from horizontal)
  std::optional<float> tilt_min;
  /// Maximum allowed tilt angle in degrees
  std::optional<float> tilt_max;
  /// Minimum pan angle (azimuth) in degrees
  std::optional<float> pan_min;
  /// Maximum pan angle (azimuth) in degrees
  std::optional<float> pan_max;
};

}  // namespace argus::common

template <>
struct glz::meta<argus::common::MirrorConfig> {
  using T = argus::common::MirrorConfig;
  static constexpr auto value = glz::object(
      "width", &T::width, "height", &T::height, "tilt_min", &T::tilt_min,
      "tilt_max", &T::tilt_max, "pan_min", &T::pan_min, "pan_max", &T::pan_max);
};
