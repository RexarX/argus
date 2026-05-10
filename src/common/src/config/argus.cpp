#include <pch.hpp>

#include <argus/common/config/argus.hpp>

#include <argus/common/config/argparse_policy.hpp>
#include <argus/common/config/config.hpp>
#include <argus/common/config/toml_policy.hpp>
#include <argus/log/logger.hpp>

#include <argparse/argparse.hpp>

#include <cstdint>
#include <string>

namespace argus::common {

namespace {

void AddLidarArgs(argparse::ArgumentParser& parser) {
  parser.add_description("Lidar configuration options");
  parser.add_argument("--fov-horizontal")
      .help("Horizontal field of view in degrees [1, 360]")
      .scan<'f', float>();
  parser.add_argument("--fov-vertical")
      .help("Vertical field of view in degrees [1, 180]")
      .scan<'f', float>();
  parser.add_argument("--range")
      .help("Maximum sensor range in metres")
      .scan<'f', float>();
  parser.add_argument("--angular-resolution")
      .help("Angular step size in degrees")
      .scan<'f', float>();
  parser.add_argument("--scan-pattern")
      .help("Scanning pattern: raster | spiral | random | sawtooth");
}

[[nodiscard]] auto ApplyLidarArgs(const argparse::ArgumentParser& parser)
    -> ConfigResult<LidarConfig> {
  LidarConfig cfg{};

  if (auto value = parser.present<float>("--fov-horizontal")) {
    cfg.fov_horizontal = *value;
  }

  if (auto value = parser.present<float>("--fov-vertical")) {
    cfg.fov_vertical = *value;
  }

  if (auto value = parser.present<float>("--range")) {
    cfg.range = *value;
  }

  if (auto value = parser.present<float>("--angular-resolution")) {
    cfg.angular_resolution = *value;
  }

  if (auto value = parser.present<std::string>("--scan-pattern")) {
    if (*value == "raster") {
      cfg.scan_pattern = ScanPattern::kRaster;
    } else if (*value == "spiral") {
      cfg.scan_pattern = ScanPattern::kSpiral;
    } else if (*value == "random") {
      cfg.scan_pattern = ScanPattern::kRandom;
    } else if (*value == "sawtooth") {
      cfg.scan_pattern = ScanPattern::kSawtooth;
    } else {
      ARGUS_ERROR("Unknown scan pattern: '{}'!", *value);
      return std::unexpected(ConfigError::kParseError);
    }
  }
  return cfg;
}

void AddMirrorArgs(argparse::ArgumentParser& parser) {
  parser.add_description("Mirror configuration options");
  parser.add_argument("--width")
      .help("Mirror width in metres")
      .scan<'f', float>();
  parser.add_argument("--height")
      .help("Mirror height in metres")
      .scan<'f', float>();
  parser.add_argument("--tilt-min")
      .help("Minimum tilt angle in degrees")
      .scan<'f', float>();
  parser.add_argument("--tilt-max")
      .help("Maximum tilt angle in degrees")
      .scan<'f', float>();
  parser.add_argument("--pan-min")
      .help("Minimum pan angle in degrees")
      .scan<'f', float>();
  parser.add_argument("--pan-max")
      .help("Maximum pan angle in degrees")
      .scan<'f', float>();
}

[[nodiscard]] auto ApplyMirrorArgs(const argparse::ArgumentParser& parser)
    -> ConfigResult<MirrorConfig> {
  MirrorConfig cfg{};

  if (auto value = parser.present<float>("--width")) {
    cfg.width = *value;
  }

  if (auto value = parser.present<float>("--height")) {
    cfg.height = *value;
  }

  if (auto value = parser.present<float>("--tilt-min")) {
    cfg.tilt_min = *value;
  }

  if (auto value = parser.present<float>("--tilt-max")) {
    cfg.tilt_max = *value;
  }

  if (auto value = parser.present<float>("--pan-min")) {
    cfg.pan_min = *value;
  }

  if (auto value = parser.present<float>("--pan-max")) {
    cfg.pan_max = *value;
  }

  return cfg;
}

void AddOptimizerArgs(argparse::ArgumentParser& parser) {
  parser.add_description("Optimizer configuration options");
  parser.add_argument("--max-iterations")
      .help("Maximum number of optimiser iterations")
      .scan<'u', uint32_t>();
  parser.add_argument("--convergence-threshold")
      .help("Stop when improvement ratio drops below this")
      .scan<'f', float>();
  parser.add_argument("--population-size")
      .help("Candidate positions evaluated per iteration")
      .scan<'u', uint32_t>();
  parser.add_argument("--seed")
      .help("RNG seed (0 = non-deterministic)")
      .scan<'u', uint32_t>();
}

[[nodiscard]] auto ApplyOptimizerArgs(const argparse::ArgumentParser& parser)
    -> ConfigResult<OptimizerConfig> {
  OptimizerConfig cfg{};

  if (auto value = parser.present<uint32_t>("--max-iterations")) {
    cfg.max_iterations = *value;
  }

  if (auto value = parser.present<float>("--convergence-threshold")) {
    cfg.convergence_threshold = *value;
  }

  if (auto value = parser.present<uint32_t>("--population-size")) {
    cfg.population_size = *value;
  }

  if (auto value = parser.present<uint32_t>("--seed")) {
    cfg.seed = *value;
  }

  return cfg;
}

void AddPathsArgs(argparse::ArgumentParser& parser) {
  parser.add_description("Paths configuration options");
  parser.add_argument("--scene-file")
      .help("Path to the scene file (OBJ, GLTF, …) relative to the config");
  parser.add_argument("--output-directory")
      .help("Directory where result files are written");
}

[[nodiscard]] auto ApplyPathsArgs(const argparse::ArgumentParser& parser)
    -> ConfigResult<PathsConfig> {
  PathsConfig cfg{};

  if (auto value = parser.present<std::string>("--scene-file")) {
    cfg.scene_file = std::move(*value);
  }

  if (auto value = parser.present<std::string>("--output-directory")) {
    cfg.output_directory = std::move(*value);
  }

  return cfg;
}

}  // namespace

template <>
auto ArgparsePolicy<ArgusConfig>::Parse(int argc, char* argv[])
    -> ConfigResult<ArgusConfig> {
  std::string program = (argc > 0 && argv[0]) ? argv[0] : "argus";

  // Top-level parser
  argparse::ArgumentParser root(std::move(program));

  // One subparser per sub-config - all optional
  argparse::ArgumentParser lidar_cmd("lidar");
  argparse::ArgumentParser mirror_cmd("mirror");
  argparse::ArgumentParser optimiser_cmd("optimiser");
  argparse::ArgumentParser paths_cmd("paths");

  AddLidarArgs(lidar_cmd);
  AddMirrorArgs(mirror_cmd);
  AddOptimizerArgs(optimiser_cmd);
  AddPathsArgs(paths_cmd);

  root.add_subparser(lidar_cmd);
  root.add_subparser(mirror_cmd);
  root.add_subparser(optimiser_cmd);
  root.add_subparser(paths_cmd);

  try {
    root.parse_args(argc, argv);
  } catch (const std::exception& e) {
    ARGUS_ERROR("Failed to parse command-line arguments: {}!", e.what());
    return std::unexpected(ConfigError::kParseError);
  }

  ArgusConfig cfg{};

  // Apply each subcommand only when it was actually present in argv
  if (root.is_subcommand_used("lidar")) {
    auto result = ApplyLidarArgs(lidar_cmd);
    if (!result) [[unlikely]] {
      return std::unexpected(result.error());
    }
    cfg.lidar = std::move(*result);
  }

  if (root.is_subcommand_used("mirror")) {
    auto result = ApplyMirrorArgs(mirror_cmd);
    if (!result) [[unlikely]] {
      return std::unexpected(result.error());
    }
    cfg.mirror = std::move(*result);
  }

  if (root.is_subcommand_used("optimiser")) {
    auto result = ApplyOptimizerArgs(optimiser_cmd);
    if (!result) [[unlikely]] {
      return std::unexpected(result.error());
    }
    cfg.optimiser = std::move(*result);
  }

  if (root.is_subcommand_used("paths")) {
    auto result = ApplyPathsArgs(paths_cmd);
    if (!result) [[unlikely]] {
      return std::unexpected(result.error());
    }
    cfg.paths = std::move(*result);
  }

  return cfg;
}

template struct ArgparsePolicy<ArgusConfig>;

}  // namespace argus::common
