#include <pch.hpp>

#include <argus/log.h>

#include <argus/assert.hpp>
#include <argus/log.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <memory_resource>
#include <string_view>

namespace {

std::atomic<ArgusLogCallback> g_c_callback{nullptr};
std::atomic<void*> g_c_user{nullptr};

void c_thunk(argus::log::Level level, std::string_view message) noexcept {
    const ArgusLogCallback callback =
        g_c_callback.load(std::memory_order_relaxed);
    if (callback == nullptr) {
        return;
    }
    void* user = g_c_user.load(std::memory_order_relaxed);

    const auto c_level = static_cast<ArgusLogLevel>(level);
    if (message.empty()) {
        callback(c_level, "", user);
        return;
    }

    std::array<char, argus::log::INLINE_BUFFER_SIZE> stack = {};
    if (message.size() + 1 <= stack.size()) {
        std::memcpy(stack.data(), message.data(), message.size());
        stack[message.size()] = '\0';
        callback(c_level, stack.data(), user);
        return;
    }

    std::pmr::string owned(message);
    callback(c_level, owned.c_str(), user);
}

void v_log(argus::log::Level level, const char* fmt, va_list args) noexcept {
    ARGUS_ASSERT(fmt != nullptr);

    std::array<char, argus::log::INLINE_BUFFER_SIZE> stack = {};
    va_list args_copy;
    va_copy(args_copy, args);
    const int written =
        std::vsnprintf(stack.data(), stack.size(), fmt, args_copy);
    va_end(args_copy);
    if (written < 0) {
        argus::log::log(level, fmt);
        return;
    }
    if (static_cast<size_t>(written) < stack.size()) {
        argus::log::log(level, stack.data());
        return;
    }

    std::pmr::string owned(static_cast<size_t>(written), '\0');
    std::vsnprintf(owned.data(), owned.size() + 1, fmt, args);
    argus::log::log(level, owned);
}

}  // namespace

extern "C" {

void argus_log_set_custom(ArgusLogCallback callback, void* user) noexcept {
    g_c_user.store(callback != nullptr ? user : nullptr,
                   std::memory_order_relaxed);
    g_c_callback.store(callback, std::memory_order_relaxed);
    argus::log::set_custom_logger(callback != nullptr ? &c_thunk : nullptr);
}

void argus_log_reset_custom(void) noexcept {
    argus_log_set_custom(nullptr, nullptr);
}

bool argus_log_has_custom(void) noexcept {
    return argus::log::get_custom_logger() == &c_thunk;
}

void argus_log(ArgusLogLevel level, const char* message) noexcept {
    ARGUS_ASSERT(message != nullptr);
    argus::log::log(static_cast<argus::log::Level>(level), message);
}

void argus_log_trace(const char* message) noexcept {
    ARGUS_ASSERT(message != nullptr);
    argus::log::trace(message);
}

void argus_log_debug(const char* message) noexcept {
    ARGUS_ASSERT(message != nullptr);
    argus::log::debug(message);
}

void argus_log_info(const char* message) noexcept {
    ARGUS_ASSERT(message != nullptr);
    argus::log::info(message);
}

void argus_log_warn(const char* message) noexcept {
    ARGUS_ASSERT(message != nullptr);
    argus::log::warn(message);
}

void argus_log_error(const char* message) noexcept {
    ARGUS_ASSERT(message != nullptr);
    argus::log::error(message);
}

void argus_log_critical(const char* message) noexcept {
    ARGUS_ASSERT(message != nullptr);
    argus::log::critical(message);
}

void argus_logf(ArgusLogLevel level, const char* fmt, ...) noexcept {
    va_list args;
    va_start(args, fmt);
    v_log(static_cast<argus::log::Level>(level), fmt, args);
    va_end(args);
}

void argus_log_tracef([[maybe_unused]] const char* fmt, ...) noexcept {
#ifndef NDEBUG
    va_list args;
    va_start(args, fmt);
    v_log(argus::log::Level::TRACE, fmt, args);
    va_end(args);
#endif
}

void argus_log_debugf([[maybe_unused]] const char* fmt, ...) noexcept {
#ifndef NDEBUG
    va_list args;
    va_start(args, fmt);
    v_log(argus::log::Level::DEBUG, fmt, args);
    va_end(args);
#endif
}

void argus_log_infof(const char* fmt, ...) noexcept {
    va_list args;
    va_start(args, fmt);
    v_log(argus::log::Level::INFO, fmt, args);
    va_end(args);
}

void argus_log_warnf(const char* fmt, ...) noexcept {
    va_list args;
    va_start(args, fmt);
    v_log(argus::log::Level::WARN, fmt, args);
    va_end(args);
}

void argus_log_errorf(const char* fmt, ...) noexcept {
    va_list args;
    va_start(args, fmt);
    v_log(argus::log::Level::ERROR, fmt, args);
    va_end(args);
}

void argus_log_criticalf(const char* fmt, ...) noexcept {
    va_list args;
    va_start(args, fmt);
    v_log(argus::log::Level::CRITICAL, fmt, args);
    va_end(args);
}
}
