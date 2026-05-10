#pragma once

#include <argus/common/config/config.hpp>
#include <argus/common/config/policy.hpp>

#include <string>
#include <string_view>

namespace argus::common {

/**
 * @brief TOML serialiser policy for ConfigT.
 * @tparam ConfigT The configuration type
 */
template <typename ConfigT>
struct TomlPolicy {
  static constexpr std::string_view kFileExtension = ".toml";

  /**
   * @brief Deserialise TOML text into a ConfigT.
   * @param data The TOML text to deserialise
   * @return The deserialised ConfigT, or an error if deserialisation failed
   */
  [[nodiscard]] static auto Read(std::string_view data)
      -> ConfigResult<ConfigT>;

  /**
   * @brief Serialise a ConfigT to a TOML string.
   * @param cfg The ConfigT to serialise
   * @return The serialised TOML string, or an error if serialisation failed
   */
  [[nodiscard]] static auto Write(const ConfigT& cfg)
      -> ConfigResult<std::string>;
};

}  // namespace argus::common
