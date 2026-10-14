#include <pch.hpp>

#include <argus/assert.h>

#include <argus/assert.hpp>
#include <argus/log.hpp>
#include <argus/utils/format.hpp>

#include <array>
#include <atomic>
#include <cstring>
#include <memory_resource>
#include <source_location>
#include <string_view>

namespace {

constexpr size_t CSTRING_BUFFER_SIZE = 256;

std::atomic<ArgusAssertionHandler> g_c_assertion_handler{nullptr};

[[nodiscard]] constexpr std::string_view file_name(
    std::string_view path) noexcept {
    const size_t last_slash = path.find_last_of("/\\");
    return (last_slash != std::string_view::npos) ? path.substr(last_slash + 1)
                                                  : path;
}

[[nodiscard]] const char* c_string(std::string_view text,
                                   std::array<char, CSTRING_BUFFER_SIZE>* stack,
                                   std::pmr::string* owned) noexcept {
    if (text.empty()) {
        return "";
    }
    if (text.size() + 1 <= stack->size()) {
        std::memcpy(stack->data(), text.data(), text.size());
        (*stack)[text.size()] = '\0';
        return stack->data();
    }
    owned->assign(text);
    return owned->c_str();
}

void cpp_to_c_assertion_thunk(std::string_view condition,
                              std::string_view message,
                              const std::source_location& loc) noexcept {
    const ArgusAssertionHandler handler =
        g_c_assertion_handler.load(std::memory_order_relaxed);
    if (handler == nullptr) {
        return;
    }

    std::array<char, CSTRING_BUFFER_SIZE> condition_stack = {};
    std::array<char, CSTRING_BUFFER_SIZE> message_stack = {};
    std::pmr::string condition_owned;
    std::pmr::string message_owned;
    const char* file = loc.file_name() != nullptr ? loc.file_name() : "";
    handler(c_string(condition, &condition_stack, &condition_owned),
            c_string(message, &message_stack, &message_owned), file,
            static_cast<int>(loc.line()));
}

}  // namespace

extern "C" {

void argus_handle_assertion(const char* condition, const char* message,
                            const char* file, int line) noexcept {
    const char* cond = condition != nullptr ? condition : "";
    const char* msg = message != nullptr ? message : "";
    const char* path = file != nullptr ? file : "";

    if (argus::get_assertion_handler() == &cpp_to_c_assertion_thunk) {
        const ArgusAssertionHandler handler =
            g_c_assertion_handler.load(std::memory_order_relaxed);
        if (handler != nullptr) {
            handler(cond, msg, path, line);
        }
        return;
    }

    std::pmr::string formatted;
    formatted.reserve(256);
    if (msg[0] != '\0') {
        argus::utils::format_to(formatted, "Assertion failed: {} | {}", cond,
                                msg);
    } else {
        argus::utils::format_to(formatted, "Assertion failed: {}", cond);
    }
    argus::utils::format_to(formatted, " [{}:{}]", file_name(path), line);
    argus::log::critical(formatted);
}

void argus_set_assertion_handler(ArgusAssertionHandler handler) noexcept {
    g_c_assertion_handler.store(handler, std::memory_order_relaxed);
    argus::set_assertion_handler(handler != nullptr ? &cpp_to_c_assertion_thunk
                                                    : nullptr);
}

ArgusAssertionHandler argus_get_assertion_handler(void) noexcept {
    if (argus::get_assertion_handler() != &cpp_to_c_assertion_thunk) {
        return nullptr;
    }
    return g_c_assertion_handler.load(std::memory_order_relaxed);
}
}
