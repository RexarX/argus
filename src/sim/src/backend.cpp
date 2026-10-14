#include <pch.hpp>

#include <argus/backend.hpp>

#include <argus/assert.hpp>
#include <argus/error.hpp>
#include <dispatch.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

namespace argus::sim {

namespace {

struct Backend_List {
    std::array<Backend, BACKEND_COUNT> items = {};
    size_t count = 0;
};

[[nodiscard]] const Backend_List& compiled() noexcept {
    static const Backend_List list = [] {
        Backend_List out;
        using Backend_Underlying = std::underlying_type_t<Backend>;
        for (Backend_Underlying i = 0; i < BACKEND_COUNT; ++i) {
            const auto backend = static_cast<Backend>(i);
            if (backend_table(backend) != nullptr) {
                out.items[out.count++] = backend;
            }
        }
        return out;
    }();
    return list;
}

}  // namespace

namespace callback {

extern const Backend_VTable TABLE;

}

#ifdef ARGUS_ENABLE_EMBREE_BACKEND
namespace embree {

extern const Backend_VTable TABLE;

}
#endif

const Backend_VTable* backend_table(Backend backend) noexcept {
    switch (backend) {
        using enum Backend;
        case EMBREE:
#ifdef ARGUS_ENABLE_EMBREE_BACKEND
            return &embree::TABLE;
#else
            return nullptr;
#endif
        case CALLBACK:
            return &callback::TABLE;
        case COUNT:
            break;
    }
    return nullptr;
}

auto compiled_backends() noexcept -> std::span<const Backend> {
    const Backend_List& list = compiled();
    return {list.items.data(), list.count};
}

auto available_backends() noexcept -> std::span<const Backend> {
    return compiled_backends();
}

bool available(Backend backend) noexcept {
    return std::ranges::contains(available_backends(), backend);
}

auto preferred_backend() noexcept -> Result<Backend> {
    const auto available = available_backends();
    if (available.empty()) [[unlikely]] {
        return fail(Error::UNSUPPORTED,
                    "No simulation backends are compiled in!");
    }
    return available.front();
}

}  // namespace argus::sim
