#include <pch.hpp>

#include <argus/instance.hpp>

#include <argus/assert.hpp>
#include <argus/backend.hpp>
#include <argus/error.hpp>
#include <argus/scene.hpp>
#include <dispatch.hpp>

#include <new>
#include <span>

namespace argus::sim {

namespace callback {

extern const Backend_VTable TABLE;

}

namespace {

struct Callback_Instance {
    const Backend_VTable* table = nullptr;
};

struct Scene {
    const Backend_VTable* table = nullptr;
    User_Trace::Function trace = nullptr;
    void* user = nullptr;
};

auto create_callback_instance() noexcept -> Result<void*>;
void destroy_callback_instance(void* instance) noexcept;

auto create_user_scene(void* instance, const User_Trace& trace) noexcept
    -> Result<void*>;

void destroy_scene(void* scene) noexcept;
auto trace_callback(void* scene, std::span<const Ray> rays,
                    std::span<Hit> hits) noexcept -> Result<void>;

auto create_callback_instance() noexcept -> Result<void*> {
    auto* created = new (std::nothrow) Callback_Instance;
    if (created == nullptr) [[unlikely]] {
        return fail(Error::OUT_OF_MEMORY,
                    "Callback backend instance allocation failed!");
    }
    created->table = &callback::TABLE;
    return created;
}

void destroy_callback_instance(void* instance) noexcept {
    delete static_cast<Callback_Instance*>(instance);
}

auto create_user_scene(void* instance, const User_Trace& trace) noexcept
    -> Result<void*> {
    ARGUS_ASSERT(instance != nullptr);
    ARGUS_ASSERT(trace.trace != nullptr);

    auto* scene = new (std::nothrow) Scene;
    if (scene == nullptr) [[unlikely]] {
        return fail(Error::OUT_OF_MEMORY,
                    "Callback backend scene allocation failed!");
    }
    scene->table = &callback::TABLE;
    scene->trace = trace.trace;
    scene->user = trace.user;
    return scene;
}

void destroy_scene(void* scene) noexcept {
    delete static_cast<Scene*>(scene);
}

auto trace_callback(void* scene, std::span<const Ray> rays,
                    std::span<Hit> hits) noexcept -> Result<void> {
    auto* callback = static_cast<Scene*>(scene);
    ARGUS_ASSERT(callback != nullptr);
    ARGUS_ASSERT(callback->trace != nullptr);
    return callback->trace(callback->user, rays, hits);
}

}  // namespace

namespace callback {

extern const Backend_VTable TABLE{
    .backend = Backend::CALLBACK,
    .create_instance = &create_callback_instance,
    .destroy_instance = &destroy_callback_instance,
    .create_scene = nullptr,
    .create_user_scene = &create_user_scene,
    .destroy_scene = &destroy_scene,
    .trace = &trace_callback,
};

}  // namespace callback

}  // namespace argus::sim
