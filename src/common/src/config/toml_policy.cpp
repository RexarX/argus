#include <pch.hpp>

#include <argus/common/config/toml_policy.hpp>

#include <argus/common/config/argus.hpp>
#include <argus/common/config/config.hpp>
#include <argus/common/config/lidar.hpp>
#include <argus/common/config/mirror.hpp>
#include <argus/common/config/optimizer.hpp>
#include <argus/common/config/paths.hpp>
#include <argus/log/logger.hpp>

#include <glaze/glaze.hpp>
#include <glaze/toml.hpp>

#include <expected>
#include <string>
#include <string_view>

template <class T>
struct glz::from<glz::TOML, std::optional<T>> {
  template <auto Opts, class It>
  static void op(auto&& value, is_context auto&& ctx, It&& it, auto end) {
    if (!value.has_value()) {
      value.emplace();
    }
    from<glz::TOML, T>::template op<Opts>(*value, ctx, it, end);
  }
};

namespace argus::common {

template <typename ConfigT>
auto TomlPolicy<ConfigT>::Read(std::string_view data) -> ConfigResult<ConfigT> {
  ConfigT config{};
  const auto error = glz::read_toml(config, data);
  if (error) [[unlikely]] {
    const auto message = glz::format_error(error, data);
    ARGUS_ERROR("Failed to parse config from TOML string: {}!", message);
    return std::unexpected(ConfigError::kParseError);
  }
  return config;
}

template <typename ConfigT>
auto TomlPolicy<ConfigT>::Write(const ConfigT& cfg)
    -> ConfigResult<std::string> {
  std::string out;
  const auto error = glz::write_toml(cfg, out);
  if (error) [[unlikely]] {
    const auto message = glz::format_error(error, out);
    ARGUS_ERROR("Failed to serialise config to TOML string: {}!", message);
    return std::unexpected(ConfigError::kWriteError);
  }
  return out;
}

template struct TomlPolicy<LidarConfig>;
template struct TomlPolicy<MirrorConfig>;
template struct TomlPolicy<OptimizerConfig>;
template struct TomlPolicy<PathsConfig>;
template struct TomlPolicy<ArgusConfig>;

}  // namespace argus::common
