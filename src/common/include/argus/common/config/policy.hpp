#pragma once

#include <argus/common/config/config.hpp>

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

namespace argus::common {

/**
 * @brief Concept for serialiser policies.
 * @tparam P The policy type that needs to provide
 * `static auto Read(std::string_view data) -> ConfigResult<ConfigT>`
 * `static auto Write(const ConfigT& cfg) -> ConfigResult<std::string>`
 * `static constexpr std::string_view kFileExtension`
 * @tparam ConfigT The configuration type
 */
template <typename P, typename ConfigT>
concept SerialiserPolicy = std::is_object_v<std::remove_cvref_t<P>> &&
                           std::is_empty_v<std::remove_cvref_t<P>> &&
                           requires(std::string_view sv, const ConfigT& cfg) {
                             {
                               std::remove_cvref_t<P>::Read(sv)
                             } -> std::same_as<ConfigResult<ConfigT>>;
                             {
                               std::remove_cvref_t<P>::Write(cfg)
                             } -> std::same_as<ConfigResult<std::string>>;
                             {
                               std::remove_cvref_t<P>::kFileExtension
                             } -> std::convertible_to<std::string_view>;
                           };

/**
 * @brief Concept for command line argument policies.
 * @tparam P The policy type that needs to provide
 * `static auto Parse(int argc, char* argv[]) -> ConfigResult<ConfigT>`
 * @tparam ConfigT The configuration type
 */
template <typename P, typename ConfigT>
concept CmdArgsPolicy = std::is_object_v<std::remove_cvref_t<P>> &&
                        std::is_empty_v<std::remove_cvref_t<P>> &&
                        requires(int argc, char* argv[]) {
                          {
                            std::remove_cvref_t<P>::Parse(argc, argv)
                          } -> std::same_as<ConfigResult<ConfigT>>;
                        };

/**
 * @brief Get the file extension of a serialiser policy.
 * @tparam P The serialiser policy type
 * @tparam ConfigT The configuration type
 * @return The file extension as a string view
 */
template <typename P, typename ConfigT>
  requires SerialiserPolicy<P, ConfigT>
[[nodiscard]] consteval std::string_view GetFileExtensionOf() noexcept {
  return P::kFileExtension;
}

}  // namespace argus::common
