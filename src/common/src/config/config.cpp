#include <pch.hpp>

#include <argus/common/config/config.hpp>

#include <argus/assert.hpp>
#include <argus/common/config/argparse_policy.hpp>
#include <argus/common/config/argus.hpp>
#include <argus/common/config/lidar.hpp>
#include <argus/common/config/mirror.hpp>
#include <argus/common/config/optimizer.hpp>
#include <argus/common/config/paths.hpp>
#include <argus/common/config/policy.hpp>
#include <argus/common/config/toml_policy.hpp>
#include <argus/log/logger.hpp>

#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace argus::common {

namespace {

template <typename ConfigT, typename SerialiserT, typename CmdArgsT>
struct PolicyChecker {
  static_assert(
      SerialiserPolicy<SerialiserT, ConfigT>,
      "SerialiserT does not satisfy SerialiserPolicy<SerialiserT, ConfigT>. "
      "Ensure it provides Read(), Write(), and kFileExtension.");
  static_assert(CmdArgsPolicy<CmdArgsT, ConfigT>,
                "CmdArgsT does not satisfy CmdArgsPolicy<CmdArgsT, ConfigT>. "
                "Ensure it provides Parse(int, char**).");
};

}  // namespace

template <typename ConfigT, typename SerialiserT, typename CmdArgsT>
auto ConfigBase<ConfigT, SerialiserT, CmdArgsT>::FromFile(
    const std::filesystem::path& path) -> ConfigResult<ConfigT> {
  namespace fs = std::filesystem;

  [[maybe_unused]] PolicyChecker<ConfigT, SerialiserT, CmdArgsT> check{};

  ARGUS_ASSERT(!path.empty(), "Path is empty!");

  if (!fs::exists(path)) [[unlikely]] {
    ARGUS_ERROR("Config file '{}' does not exist!", path.string());
    return std::unexpected(ConfigError::kFileDoesNotExist);
  }

  if (path.extension() != SerialiserT::kFileExtension) [[unlikely]] {
    ARGUS_ERROR("Config file '{}' has wrong extension (expected '{}')!",
                path.string(), SerialiserT::kFileExtension);
    return std::unexpected(ConfigError::kFileError);
  }

  std::ifstream file(path);
  if (!file.is_open()) [[unlikely]] {
    ARGUS_ERROR("Failed to open config file: {}!", path.string());
    return std::unexpected(ConfigError::kFileError);
  }

  const std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
  return FromString(content);
}

template <typename ConfigT, typename SerialiserT, typename CmdArgsT>
auto ConfigBase<ConfigT, SerialiserT, CmdArgsT>::FromString(
    std::string_view string) -> ConfigResult<ConfigT> {
  ARGUS_ASSERT(!string.empty(), "String is empty!");
  return SerialiserT::Read(string);
}

template <typename ConfigT, typename SerialiserT, typename CmdArgsT>
auto ConfigBase<ConfigT, SerialiserT, CmdArgsT>::FromCmdArgs(int argc,
                                                             char* argv[])
    -> ConfigResult<ConfigT> {
  ARGUS_ASSERT(argc > 0, "argc must be > 0!");
  ARGUS_ASSERT(argv != nullptr, "argv is null!");
  return CmdArgsT::Parse(argc, argv);
}

template <typename ConfigT, typename SerialiserT, typename CmdArgsT>
auto ConfigBase<ConfigT, SerialiserT, CmdArgsT>::ToFile(
    std::filesystem::path path) const -> ConfigResult<void> {
  namespace fs = std::filesystem;

  ARGUS_ASSERT(!path.empty(), "Path is empty!");

  // If the path is a directory, append the default filename
  if (fs::is_directory(path) || !path.has_filename()) {
    path /= kDefaultConfigFilename;
  }

  if (path.extension() != SerialiserT::kFileExtension) [[unlikely]] {
    ARGUS_ERROR("Config file '{}' should have '{}' extension!", path.string(),
                SerialiserT::kFileExtension);
    return std::unexpected(ConfigError::kFileError);
  }

  // Create parent directories if they don't exist
  if (const auto parent = path.parent_path(); !parent.empty()) {
    std::error_code ec;
    if (!fs::create_directories(parent, ec) && ec) [[unlikely]] {
      ARGUS_ERROR("Failed to create directories '{}': {}!", parent.string(),
                  ec.message());
      return std::unexpected(ConfigError::kFileError);
    }
  }

  auto result = ToString();
  if (!result) [[unlikely]] {
    return std::unexpected(result.error());
  }

  std::ofstream file(path);
  if (!file.is_open()) [[unlikely]] {
    ARGUS_ERROR("Failed to open config file for writing: {}", path.string());
    return std::unexpected(ConfigError::kFileError);
  }

  file << *result;
  if (!file) [[unlikely]] {
    ARGUS_ERROR("Failed to write config to file: {}", path.string());
    return std::unexpected(ConfigError::kWriteError);
  }

  return {};
}

template <typename ConfigT, typename SerialiserT, typename CmdArgsT>
auto ConfigBase<ConfigT, SerialiserT, CmdArgsT>::ToString() const
    -> ConfigResult<std::string> {
  return SerialiserT::Write(GetDerived());
}

}  // namespace argus::common

template struct argus::common::ConfigBase<
    argus::common::LidarConfig,
    argus::common::TomlPolicy<argus::common::LidarConfig>,
    argus::common::ArgparsePolicy<argus::common::LidarConfig>>;

template struct argus::common::ConfigBase<
    argus::common::MirrorConfig,
    argus::common::TomlPolicy<argus::common::MirrorConfig>,
    argus::common::ArgparsePolicy<argus::common::MirrorConfig>>;

template struct argus::common::ConfigBase<
    argus::common::OptimizerConfig,
    argus::common::TomlPolicy<argus::common::OptimizerConfig>,
    argus::common::ArgparsePolicy<argus::common::OptimizerConfig>>;

template struct argus::common::ConfigBase<
    argus::common::PathsConfig,
    argus::common::TomlPolicy<argus::common::PathsConfig>,
    argus::common::ArgparsePolicy<argus::common::PathsConfig>>;

template struct argus::common::ConfigBase<
    argus::common::ArgusConfig,
    argus::common::TomlPolicy<argus::common::ArgusConfig>,
    argus::common::ArgparsePolicy<argus::common::ArgusConfig>>;
