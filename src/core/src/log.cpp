#include <pch.hpp>

#include <argus/log.hpp>

#include <atomic>
#include <cstdio>
#include <string_view>

#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
#include <print>
#define ARGUS_HAS_STD_PRINT 1
#else
#define ARGUS_HAS_STD_PRINT 0
#endif

namespace argus::log {

namespace {

std::atomic<Callback> g_custom_logger{nullptr};

void default_log(Level level, std::string_view message) noexcept {
    std::FILE* stream = level >= Level::WARN ? stderr : stdout;
    const std::string_view level_str = to_string(level);

#if ARGUS_HAS_STD_PRINT
    std::println(stream, "[{}] {}", level_str, message);
#else
    std::fprintf(stream, "[%.*s] %.*s\n", static_cast<int>(level_str.size()),
                 level_str.data(), static_cast<int>(message.size()),
                 message.data());
#endif
}

}  // namespace

void set_custom_logger(Callback callback) noexcept {
    g_custom_logger.store(callback, std::memory_order_relaxed);
}

void reset_custom_logger() noexcept {
    g_custom_logger.store(nullptr, std::memory_order_relaxed);
}

bool has_custom_logger() noexcept {
    return g_custom_logger.load(std::memory_order_relaxed) != nullptr;
}

Callback get_custom_logger() noexcept {
    return g_custom_logger.load(std::memory_order_relaxed);
}

void log(Level level, std::string_view message) noexcept {
    if (Callback custom = g_custom_logger.load(std::memory_order_relaxed)) {
        custom(level, message);
        return;
    }
    default_log(level, message);
}

}  // namespace argus::log
