#include <pch.hpp>

#include <argus/assert.hpp>

#include <argus/log.hpp>
#include <argus/utils/format.hpp>

#include <atomic>
#include <cstddef>
#include <memory_resource>
#include <source_location>
#include <string_view>

namespace argus {

namespace {

std::atomic<Assertion_Handler> g_custom_assertion_handler{nullptr};

[[nodiscard]] constexpr std::string_view file_name(
    std::string_view path) noexcept {
    const size_t last_slash = path.find_last_of("/\\");
    return (last_slash != std::string_view::npos) ? path.substr(last_slash + 1)
                                                  : path;
}

void default_assertion_handler(std::string_view condition,
                               std::string_view message,
                               const std::source_location& loc) noexcept {
    const std::pmr::string formatted =
        format_assertion_message(condition, message, loc);
    log::critical(std::string_view{formatted});
}

}  // namespace

std::pmr::string format_assertion_message(
    std::string_view condition, std::string_view message,
    const std::source_location& loc,
    std::pmr::memory_resource* allocator) noexcept {
    ARGUS_ASSERT(allocator != nullptr,
                 "format_assertion_message needs an allocator");
    std::pmr::string result{allocator};
    result.reserve(256);

    if (!message.empty()) {
        utils::format_to(result, "Assertion failed: {} | {}", condition,
                         message);
    } else {
        utils::format_to(result, "Assertion failed: {}", condition);
    }

    const std::string_view filename = file_name(loc.file_name());
    utils::format_to(result, " [{}:{}]", filename, loc.line());
    return result;
}

void handle_assertion(std::string_view condition, std::string_view message,
                      const std::source_location& loc) noexcept {
    if (Assertion_Handler custom =
            g_custom_assertion_handler.load(std::memory_order_relaxed)) {
        custom(condition, message, loc);
        return;
    }
    default_assertion_handler(condition, message, loc);
}

void set_assertion_handler(Assertion_Handler handler) noexcept {
    g_custom_assertion_handler.store(handler, std::memory_order_relaxed);
}

Assertion_Handler get_assertion_handler() noexcept {
    return g_custom_assertion_handler.load(std::memory_order_relaxed);
}

}  // namespace argus
