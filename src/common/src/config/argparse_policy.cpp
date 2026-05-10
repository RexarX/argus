#include <pch.hpp>

#include <argus/common/config/argparse_policy.hpp>

#include <argus/common/config/argus.hpp>
#include <argus/common/config/config.hpp>
#include <argus/common/config/lidar.hpp>
#include <argus/common/config/mirror.hpp>
#include <argus/common/config/optimizer.hpp>
#include <argus/common/config/paths.hpp>
#include <argus/log/logger.hpp>

#include <argparse/argparse.hpp>

#include <cstdint>
#include <string>

namespace argus::common {

namespace {

auto MakeParser(int argc, char* argv[]) -> argparse::ArgumentParser {
  std::string program_name = (argc > 0 && argv[0]) ? argv[0] : "argus";
  return argparse::ArgumentParser(std::move(program_name));
}

auto TryParse(argparse::ArgumentParser& parser, int argc, char* argv[])
    -> ConfigResult<void> {
  try {
    parser.parse_args(argc, argv);
  } catch (const std::exception& e) {
    ARGUS_ERROR("Failed to parse command-line arguments: {}!", e.what());
    return std::unexpected(ConfigError::kParseError);
  }
  return {};
}

}  // namespace

template <>
auto ArgparsePolicy<LidarConfig>::Parse(int argc, char* argv[])
    -> ConfigResult<LidarConfig> {
  auto root = MakeParser(argc, argv);
  argparse::ArgumentParser lidar_cmd("lidar");

  lidar_cmd.add_description("Lidar configuration options");
  lidar_cmd.add_argument("--fov-horizontal")
      .help("Horizontal field of view in degrees [1, 360]")
      .scan<'f', float>();
  lidar_cmd.add_argument("--fov-vertical")
      .help("Vertical field of view in degrees [1, 180]")
      .scan<'f', float>();
  lidar_cmd.add_argument("--range")
      .help("Maximum sensor range in metres")
      .scan<'f', float>();
  lidar_cmd.add_argument("--angular-resolution")
      .help("Angular step size in degrees")
      .scan<'f', float>();
  lidar_cmd.add_argument("--scan-pattern")
      .help("Scanning pattern: raster | spiral | random | sawtooth");

  root.add_subparser(lidar_cmd);

  if (auto result = TryParse(root, argc, argv); !result) {
    return std::unexpected(result.error());
  }

  LidarConfig cfg{};

  if (!root.is_subcommand_used("lidar")) {
    return cfg;
  }

  if (auto value = lidar_cmd.present<float>("--fov-horizontal")) {
    cfg.fov_horizontal = *value;
  }

  if (auto value = lidar_cmd.present<float>("--fov-vertical")) {
    cfg.fov_vertical = *value;
  }

  if (auto value = lidar_cmd.present<float>("--range")) {
    cfg.range = *value;
  }

  if (auto value = lidar_cmd.present<float>("--angular-resolution")) {
    cfg.angular_resolution = *value;
  }

  if (auto value = lidar_cmd.present<std::string>("--scan-pattern")) {
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

template <>
auto ArgparsePolicy<MirrorConfig>::Parse(int argc, char* argv[])
    -> ConfigResult<MirrorConfig> {
  auto root = MakeParser(argc, argv);
  argparse::ArgumentParser mirror_cmd("mirror");

  mirror_cmd.add_description("Mirror configuration options");
  mirror_cmd.add_argument("--width")
      .help("Mirror width in metres")
      .scan<'f', float>();
  mirror_cmd.add_argument("--height")
      .help("Mirror height in metres")
      .scan<'f', float>();
  mirror_cmd.add_argument("--tilt-min")
      .help("Minimum tilt angle in degrees")
      .scan<'f', float>();
  mirror_cmd.add_argument("--tilt-max")
      .help("Maximum tilt angle in degrees")
      .scan<'f', float>();
  mirror_cmd.add_argument("--pan-min")
      .help("Minimum pan angle in degrees")
      .scan<'f', float>();
  mirror_cmd.add_argument("--pan-max")
      .help("Maximum pan angle in degrees")
      .scan<'f', float>();

  root.add_subparser(mirror_cmd);

  if (auto result = TryParse(root, argc, argv); !result) [[unlikely]] {
    return std::unexpected(result.error());
  }

  MirrorConfig cfg{};

  if (!root.is_subcommand_used("mirror")) {
    return cfg;
  }

  if (auto value = mirror_cmd.present<float>("--mirror-width")) {
    cfg.width = *value;
  }

  if (auto value = mirror_cmd.present<float>("--mirror-height")) {
    cfg.height = *value;
  }

  if (auto value = mirror_cmd.present<float>("--mirror-tilt-min")) {
    cfg.tilt_min = *value;
  }

  if (auto value = mirror_cmd.present<float>("--mirror-tilt-max")) {
    cfg.tilt_max = *value;
  }

  if (auto value = mirror_cmd.present<float>("--mirror-pan-min")) {
    cfg.pan_min = *value;
  }

  if (auto value = mirror_cmd.present<float>("--mirror-pan-max")) {
    cfg.pan_max = *value;
  }

  return cfg;
}

template <>
auto ArgparsePolicy<OptimizerConfig>::Parse(int argc, char* argv[])
    -> ConfigResult<OptimizerConfig> {
  auto root = MakeParser(argc, argv);
  argparse::ArgumentParser optimizer_cmd("optimizer");

  optimizer_cmd.add_description("Optimizer configuration options");
  optimizer_cmd.add_argument("--max-iterations")
      .help("Maximum number of optimiser iterations")
      .scan<'u', uint32_t>();
  optimizer_cmd.add_argument("--convergence-threshold")
      .help("Stop when improvement ratio drops below this value")
      .scan<'f', float>();
  optimizer_cmd.add_argument("--population-size")
      .help("Candidate positions evaluated per iteration")
      .scan<'u', uint32_t>();
  optimizer_cmd.add_argument("--seed")
      .help("RNG seed (0 = non-deterministic)")
      .scan<'u', uint32_t>();

  root.add_subparser(optimizer_cmd);

  if (auto result = TryParse(root, argc, argv); !result) [[unlikely]] {
    return std::unexpected(result.error());
  }

  OptimizerConfig cfg{};

  if (!root.is_subcommand_used("optimizer")) {
    return cfg;
  }

  if (auto value = optimizer_cmd.present<uint32_t>("--max-iterations")) {
    cfg.max_iterations = *value;
  }

  if (auto value = optimizer_cmd.present<float>("--convergence-threshold")) {
    cfg.convergence_threshold = *value;
  }

  if (auto value = optimizer_cmd.present<uint32_t>("--population-size")) {
    cfg.population_size = *value;
  }

  if (auto value = optimizer_cmd.present<uint32_t>("--seed")) {
    cfg.seed = *value;
  }

  return cfg;
}

template <>
auto ArgparsePolicy<PathsConfig>::Parse(int argc, char* argv[])
    -> ConfigResult<PathsConfig> {
  auto root = MakeParser(argc, argv);
  argparse::ArgumentParser paths_cmd("paths");

  paths_cmd.add_description("Paths configuration options");
  paths_cmd.add_argument("--scene-file")
      .help("Path to the scene file (OBJ, GLTF, etc.) relative to the config");
  paths_cmd.add_argument("--output-directory")
      .help("Directory where result files are written");

  root.add_subparser(paths_cmd);

  if (auto result = TryParse(root, argc, argv); !result) [[unlikely]] {
    return std::unexpected(result.error());
  }

  PathsConfig cfg{};

  if (!root.is_subcommand_used("paths")) {
    return cfg;
  }

  if (auto value = paths_cmd.present<std::string>("--scene-file")) {
    cfg.scene_file = std::move(*value);
  }

  if (auto value = paths_cmd.present<std::string>("--output-directory")) {
    cfg.output_directory = std::move(*value);
  }

  return cfg;
}

template struct ArgparsePolicy<LidarConfig>;
template struct ArgparsePolicy<MirrorConfig>;
template struct ArgparsePolicy<OptimizerConfig>;
template struct ArgparsePolicy<PathsConfig>;
template struct ArgparsePolicy<ArgusConfig>;

}  // namespace argus::common
