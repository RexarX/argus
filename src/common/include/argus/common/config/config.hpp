#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

namespace argus::common {

inline constexpr std::string_view kDefaultConfigFilename = "argus.toml";

/// @brief Errors that can occur when parsing a configuration file
enum class ConfigError : uint8_t {
  kFileDoesNotExist,
  kFileError,
  kParseError,
  kWriteError,
};

/**
 * @brief Converts a `ConfigError` to a human-readable string
 * @param error The error to convert
 * @return A human-readable string representation of the error
 */
static constexpr auto ConfigErrorToString(ConfigError error) noexcept
    -> std::string_view {
  switch (error) {
    case ConfigError::kFileDoesNotExist:
      return "File does not exist";
    case ConfigError::kFileError:
      return "File error";
    case ConfigError::kParseError:
      return "Parse error";
    case ConfigError::kWriteError:
      return "Write error";
    default:
      return "Unknown error";
  }
}

template <typename T>
using ConfigResult = std::expected<T, ConfigError>;

/**
 * @brief CRTP base for all configuration types.
 * @note Neither `SerialiserT` nor `CmdArgsT` are constrained with concepts here
 * because the forward declarations of the default policies are enough at
 * this point - the full concept check happens in `config.cpp` where both
 * policy headers are included.
 * @tparam ConfigT The derived config struct.
 * @tparam SerialiserT A type satisfying `SerialiserPolicy<ConfigT>` - owns the
 * encode/decode logic for a particular file format (default:
 * `TomlPolicy<ConfigT>`)
 * @tparam CmdArgsT A type satisfying `CmdArgsPolicy<ConfigT>` - owns CLI
 * argument registration and parsing (default: `ArgparsePolicy<ConfigT>`)
 */
template <typename ConfigT, typename SerialiserT, typename CmdArgsT>
struct ConfigBase {
public:
  /**
   * @brief Loads a configuration from a file
   * @warning Triggers assertion if path is empty.
   * @param filename Path to the configuration file
   * @return A `ConfigT` if successful, or a `ConfigError` if unsuccessful
   */
  [[nodiscard]] static auto FromFile(const std::filesystem::path& path)
      -> ConfigResult<ConfigT>;

  /**
   * @brief Loads a configuration from a string
   * @warning Triggers assertion if string is empty.
   * @param string The configuration string
   * @return A `ConfigT` if successful, or a `ConfigError` if unsuccessful
   */
  [[nodiscard]] static auto FromString(std::string_view string)
      -> ConfigResult<ConfigT>;

  /**
   * @brief Loads a configuration from command-line arguments.
   * @param argc Argument count
   * @param argv Argument values
   * @return A `ConfigT` if successful, or a `ConfigError` otherwise
   */
  [[nodiscard]] static auto FromCmdArgs(int argc, char* argv[])
      -> ConfigResult<ConfigT>;

  /**
   * @brief Saves the configuration to a file
   * @warning Triggers assertion if path is empty.
   * @param path Path to the configuration file
   * @return `void` if successful, or a `ConfigError` if unsuccessful
   */
  [[nodiscard]] auto ToFile(std::filesystem::path path) const
      -> ConfigResult<void>;

  /**
   * @brief Converts the configuration to a string
   * @return The configuration as a string, or a `ConfigError` if unsuccessful
   */
  [[nodiscard]] auto ToString() const -> ConfigResult<std::string>;

private:
  [[nodiscard]] constexpr ConfigT& GetDerived() noexcept {
    return static_cast<ConfigT&>(*this);
  }

  [[nodiscard]] constexpr const ConfigT& GetDerived() const noexcept {
    return static_cast<const ConfigT&>(*this);
  }
};

}  // namespace argus::common
