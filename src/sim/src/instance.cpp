#include <pch.hpp>

#include <argus/instance.hpp>

#include <argus/assert.hpp>
#include <argus/backend.hpp>
#include <argus/error.hpp>
#include <dispatch.hpp>

#include <expected>

namespace argus::sim {

Backend backend_of(Instance instance) noexcept {
    ARGUS_ASSERT(instance.ptr != nullptr);
    const Backend_VTable* table = table_of(instance);
    ARGUS_ASSERT(table != nullptr);
    return table->backend;
}

auto create_instance() noexcept -> Result<Instance> {
    const auto preferred = preferred_backend();
    if (!preferred) [[unlikely]] {
        return std::unexpected(preferred.error());
    }
    return create_instance(*preferred);
}

auto create_instance(Backend backend) noexcept -> Result<Instance> {
    if (!available(backend)) [[unlikely]] {
        return fail(Error::UNSUPPORTED, "Backend '{}' is not available!",
                    to_string(backend));
    }

    const Backend_VTable* table = backend_table(backend);
    if (table == nullptr || table->create_instance == nullptr) [[unlikely]] {
        return fail(Error::UNSUPPORTED, "Backend '{}' cannot create instances!",
                    to_string(backend));
    }

    auto created = table->create_instance();
    if (!created) [[unlikely]] {
        return std::unexpected(created.error());
    }
    ARGUS_ASSERT(*created != nullptr);
    return Instance{.ptr = *created};
}

void destroy(Instance instance) noexcept {
    if (!instance) [[unlikely]] {
        return;
    }

    const Backend_VTable* table = table_of(instance);
    ARGUS_ASSERT(table != nullptr && table->destroy_instance != nullptr,
                 "Unknown instance backend: {}!", to_string(table->backend));
    table->destroy_instance(instance.ptr);
}

}  // namespace argus::sim
