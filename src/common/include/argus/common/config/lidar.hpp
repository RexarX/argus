#pragma once

#include <argus/common/config/argparse_policy.hpp>
#include <argus/common/config/config.hpp>
#include <argus/common/config/toml_policy.hpp>

#include <cstdint>
#include <optional>

#include <glaze/glaze.hpp>

namespace argus::common {

enum class ScanPattern : uint8_t {
  kRaster,    ///< Row-by-row horizontal sweep
  kSpiral,    ///< Archimedean spiral from center out
  kRandom,    ///< Uniform random sampling
  kSawtooth,  ///< Sawtooth (flyback) vertical + horizontal
};

/**
 * @brief Converts a `ScanPattern` to a human-readable string
 * @param pattern The pattern to convert
 * @return A human-readable string representation of the pattern
 */
static constexpr auto ScanPatternToString(ScanPattern pattern) noexcept
    -> std::string_view {
  switch (pattern) {
    case ScanPattern::kRaster:
      return "Raster";
    case ScanPattern::kSpiral:
      return "Spiral";
    case ScanPattern::kRandom:
      return "Random";
    case ScanPattern::kSawtooth:
      return "Sawtooth";
    default:
      return "Unknown pattern";
  }
}

/// @brief Configuration for a lidar sensor.
struct LidarConfig : public ConfigBase<LidarConfig, TomlPolicy<LidarConfig>,
                                       ArgparsePolicy<LidarConfig>> {
  static constexpr float kDefaultFovHorizontal = 360.0F;
  static constexpr float kDefaultFovVertical = 30.0F;
  static constexpr float kDefaultRange = 100.0F;
  static constexpr float kDefaultAngularResolution = 0.1F;
  static constexpr ScanPattern kDefaultScanPattern = ScanPattern::kRaster;

  /// Horizontal field of view in degrees [1, 360]
  std::optional<float> fov_horizontal;
  /// Vertical field of view in degrees [1, 180]
  std::optional<float> fov_vertical;
  /// Maximum range in metres
  std::optional<float> range;
  /// Angular resolution (step) in degrees
  std::optional<float> angular_resolution;
  /// Scanning pattern used for ray generation
  std::optional<ScanPattern> scan_pattern;
};

}  // namespace argus::common

template <>
struct glz::meta<argus::common::ScanPattern> {
  using enum argus::common::ScanPattern;
  static constexpr auto value =
      glz::enumerate("raster", kRaster, "spiral", kSpiral, "random", kRandom,
                     "sawtooth", kSawtooth);
};

template <>
struct glz::meta<argus::common::LidarConfig> {
  using T = argus::common::LidarConfig;
  static constexpr auto value =
      glz::object("fov_horizontal", &T::fov_horizontal, "fov_vertical",
                  &T::fov_vertical, "range", &T::range, "angular_resolution",
                  &T::angular_resolution, "scan_pattern", &T::scan_pattern);
};
