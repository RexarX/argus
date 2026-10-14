#pragma once

#include <argus/memory/temporary_storage.hpp>

#include <cstddef>
#include <format>
#include <memory_resource>
#include <string>
#include <utility>

namespace argus::utils {

/// @brief Appends a formatted string to `out`.
/// @tparam Traits Character traits of `out`
/// @tparam Allocator Allocator of `out`
/// @tparam Args Format argument types
/// @param out String to append to
/// @param fmt Format string
/// @param args Format arguments
/// @return Iterator past the appended text
template <typename Traits, typename Allocator, typename... Args>
inline auto format_to(std::basic_string<char, Traits, Allocator>& out,
                      std::format_string<Args...> fmt, Args&&... args) noexcept
    -> typename std::basic_string<char, Traits, Allocator>::iterator {
    using difference_type =
        typename std::basic_string<char, Traits, Allocator>::difference_type;

    const size_t old_size = out.size();
    const size_t added = std::formatted_size(fmt, std::forward<Args>(args)...);
    out.resize(old_size + added);
    if (added == 0) [[unlikely]] {
        return out.begin() + static_cast<difference_type>(old_size);
    }
    return std::format_to(out.begin() + static_cast<difference_type>(old_size),
                          fmt, std::forward<Args>(args)...);
}

/// @brief Formats into a `std::pmr::string` allocated from `resource`.
/// @tparam Args Format argument types
/// @param resource Allocator for the result
/// @param fmt Format string
/// @param args Format arguments
/// @return Formatted string
template <typename... Args>
inline auto format_with(std::pmr::memory_resource* resource,
                        std::format_string<Args...> fmt,
                        Args&&... args) noexcept -> std::pmr::string {
    std::pmr::string result{resource};
    format_to(result, fmt, std::forward<Args>(args)...);
    return result;
}

/// @brief Format a string into a temporary buffer allocated from the calling
/// thread's `temporary storage`.
/// @warning The returned string is only valid until the next call to
/// `reset_temporary_storage()` on this thread.
/// @tparam Args Argument types for the format string
/// @param fmt The format string
/// @param args The arguments to format into the string
/// @return A formatted string, allocated from the calling thread's
/// `temporary storage`
template <typename... Args>
inline auto temp_format(std::format_string<Args...> fmt, Args&&... args)
    -> std::pmr::string {
    auto resource = mem::get_temporary_resource();
    return format_with(&resource, fmt, std::forward<Args>(args)...);
}

/// @brief Format a wide string into a temporary buffer allocated from the
/// calling thread's `temporary storage`.
/// @warning The returned string is only valid until the next call to
/// `reset_temporary_storage()` on this thread.
/// @tparam Args Argument types for the format string
/// @param fmt The format string
/// @param args The arguments to format into the string
/// @return A formatted wide string, allocated from the calling thread's
/// `temporary storage`
template <typename... Args>
inline auto temp_format(std::wformat_string<Args...> fmt, Args&&... args)
    -> std::pmr::wstring {
    auto resource = mem::get_temporary_resource();
    return format_with(&resource, fmt, std::forward<Args>(args)...);
}

}  // namespace argus::utils
