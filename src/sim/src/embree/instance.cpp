#include <pch.hpp>

#include <argus/sim/embree/instance.hpp>
#include <argus/sim/embree/scene.hpp>

#include <argus/assert.hpp>
#include <argus/error.hpp>
#include <argus/types.hpp>
#include <dispatch.hpp>

#include <embree4/rtcore.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <new>
#include <span>

namespace argus::sim::embree {

namespace {

void log_embree_error(void* /*user*/, RTCError code,
                      const char* message) noexcept {
    log::error("Embree RTC error {}: {}", static_cast<int>(code),
               message != nullptr ? message : "");
}

void release_geometry(RTCGeometry geometry, RTCScene rtc_scene) noexcept {
    if (geometry != nullptr) {
        rtcReleaseGeometry(geometry);
    }
    if (rtc_scene != nullptr) {
        rtcReleaseScene(rtc_scene);
    }
}

[[nodiscard]] bool indices_fit(const Mesh_View& mesh) noexcept {
    const size_t vertices = mesh.vertices.size();
    if (vertices > UINT32_MAX) {
        return false;
    }
    if (mesh.triangles.size() > UINT32_MAX) {
        return false;
    }
    const auto limit = static_cast<uint32_t>(vertices);
    for (const uint3& triangle : mesh.triangles) {
        if (triangle[0] >= limit || triangle[1] >= limit ||
            triangle[2] >= limit) {
            return false;
        }
    }
    return true;
}

auto commit(RTCDeviceTy* rtc, const Mesh_View& mesh) noexcept
    -> Result<RTCScene> {
    if (!indices_fit(mesh)) [[unlikely]] {
        return fail(Error::INVALID,
                    "mesh triangle references a vertex index out of range");
    }

    RTCScene rtc_scene = rtcNewScene(rtc);
    if (rtc_scene == nullptr) [[unlikely]] {
        return fail(Error::OUT_OF_MEMORY, "rtcNewScene failed");
    }
    rtcSetSceneBuildQuality(rtc_scene, RTC_BUILD_QUALITY_HIGH);

    const size_t triangles = mesh.triangles.size();
    if (triangles > 0) [[likely]] {
        RTCGeometry geometry = rtcNewGeometry(rtc, RTC_GEOMETRY_TYPE_TRIANGLE);
        if (geometry == nullptr) [[unlikely]] {
            release_geometry(nullptr, rtc_scene);
            return fail(Error::OUT_OF_MEMORY, "rtcNewGeometry failed");
        }
        rtcSetGeometryBuildQuality(geometry, RTC_BUILD_QUALITY_HIGH);

        const size_t vertices = mesh.vertices.size();
        auto* vertex_dst = static_cast<float*>(rtcSetNewGeometryBuffer(
            geometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
            3 * sizeof(float), vertices));
        auto* index_dst = static_cast<uint32_t*>(rtcSetNewGeometryBuffer(
            geometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
            3 * sizeof(uint32_t), triangles));
        if (vertex_dst == nullptr || index_dst == nullptr) [[unlikely]] {
            release_geometry(geometry, rtc_scene);
            return fail(Error::OUT_OF_MEMORY, "rtcSetNewGeometryBuffer failed");
        }

        float* vertex = vertex_dst;
        for (const float3& source : mesh.vertices) {
            vertex[0] = source[0];
            vertex[1] = source[1];
            vertex[2] = source[2];
            vertex += 3;
        }

        uint32_t* index = index_dst;
        for (const uint3& triangle : mesh.triangles) {
            index[0] = triangle[0];
            index[1] = triangle[1];
            index[2] = triangle[2];
            index += 3;
        }

        rtcCommitGeometry(geometry);
        if (rtcGetDeviceError(rtc) != RTC_ERROR_NONE) [[unlikely]] {
            release_geometry(geometry, rtc_scene);
            return fail(Error::INVALID, "rtcCommitGeometry failed");
        }

        rtcAttachGeometry(rtc_scene, geometry);
        rtcReleaseGeometry(geometry);
        if (rtcGetDeviceError(rtc) != RTC_ERROR_NONE) [[unlikely]] {
            release_geometry(nullptr, rtc_scene);
            return fail(Error::INVALID, "rtcAttachGeometry failed");
        }
    }

    rtcCommitScene(rtc_scene);
    if (rtcGetDeviceError(rtc) != RTC_ERROR_NONE) [[unlikely]] {
        release_geometry(nullptr, rtc_scene);
        return fail(Error::INVALID, "rtcCommitScene failed");
    }
    return rtc_scene;
}

auto create_instance_thunk() noexcept -> Result<void*>;
void destroy_thunk(void* instance) noexcept;
auto create_scene_thunk(void* instance, const Mesh_View& mesh) noexcept
    -> Result<void*>;
void destroy_scene_thunk(void* scene) noexcept;
auto trace_thunk(void* scene, std::span<const Ray> rays,
                 std::span<Hit> hits) noexcept -> Result<void>;

}  // namespace

extern const Backend_VTable TABLE{
    .backend = Backend::EMBREE,
    .create_instance = &create_instance_thunk,
    .destroy_instance = &destroy_thunk,
    .create_scene = &create_scene_thunk,
    .create_user_scene = nullptr,
    .destroy_scene = &destroy_scene_thunk,
    .trace = &trace_thunk,
};

auto create_instance() noexcept -> Result<Instance*> {
    RTCDeviceTy* rtc = rtcNewDevice(nullptr);
    if (rtc == nullptr) [[unlikely]] {
        return fail(Error::OUT_OF_MEMORY, "rtcNewDevice failed");
    }
    rtcSetDeviceErrorFunction(rtc, log_embree_error, nullptr);

    auto* native = new (std::nothrow) Instance;
    if (native == nullptr) [[unlikely]] {
        rtcReleaseDevice(rtc);
        return fail(Error::OUT_OF_MEMORY, "Embree instance allocation failed");
    }

    native->table = &TABLE;
    native->rtc = rtc;
    return native;
}

void destroy(Instance* instance) noexcept {
    if (instance == nullptr) [[unlikely]] {
        return;
    }

    if (instance->rtc != nullptr) [[likely]] {
        rtcReleaseDevice(instance->rtc);
        instance->rtc = nullptr;
    }
    delete instance;
}

auto create_scene(Instance* instance, const Mesh_View& mesh) noexcept
    -> Result<Scene*> {
    ARGUS_ASSERT(instance != nullptr);
    ARGUS_ASSERT(instance->rtc != nullptr);

    auto committed = commit(instance->rtc, mesh);
    if (!committed) [[unlikely]] {
        return std::unexpected(committed.error());
    }

    auto* scene = new (std::nothrow) Scene;
    if (scene == nullptr) [[unlikely]] {
        rtcReleaseScene(*committed);
        return fail(Error::OUT_OF_MEMORY, "Embree scene allocation failed");
    }

    scene->table = &TABLE;
    scene->rtc_scene = *committed;
    return scene;
}

void destroy(Scene* scene) noexcept {
    if (scene == nullptr) [[unlikely]] {
        return;
    }

    if (scene->rtc_scene != nullptr) [[likely]] {
        rtcReleaseScene(scene->rtc_scene);
        scene->rtc_scene = nullptr;
    }
    delete scene;
}

auto trace(Scene* scene, std::span<const Ray> rays,
           std::span<Hit> hits) noexcept -> Result<void> {
    ARGUS_ASSERT(scene != nullptr);
    ARGUS_ASSERT(scene->rtc_scene != nullptr);
    ARGUS_ASSERT(rays.size() == hits.size());

    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    args.flags = RTC_RAY_QUERY_FLAG_COHERENT;

    for (size_t index = 0; index < rays.size(); ++index) {
        Hit& hit = hits[index];
        hit = {};

        const Ray& ray = rays[index];
        if (ray.max_distance <= 0.0F) {
            continue;
        }

        RTCRayHit query{};
        query.ray.org_x = ray.origin[0];
        query.ray.org_y = ray.origin[1];
        query.ray.org_z = ray.origin[2];
        query.ray.tnear = 0.0F;
        query.ray.dir_x = ray.direction[0];
        query.ray.dir_y = ray.direction[1];
        query.ray.dir_z = ray.direction[2];
        query.ray.tfar = ray.max_distance;
        query.ray.mask = 0xFFFFFFFFU;
        query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rtcIntersect1(scene->rtc_scene, &query, &args);
        if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
            continue;
        }

        hit.hit = true;
        hit.distance = query.ray.tfar;
        hit.position = ray.origin + ray.direction * query.ray.tfar;
        const float3 normal{query.hit.Ng_x, query.hit.Ng_y, query.hit.Ng_z};
        hit.normal = normal.normalized();
        hit.primitive = query.hit.primID;
    }
    return {};
}

namespace {

auto create_instance_thunk() noexcept -> Result<void*> {
    auto created = create_instance();
    if (!created) [[unlikely]] {
        return std::unexpected(created.error());
    }
    return static_cast<void*>(*created);
}

void destroy_thunk(void* instance) noexcept {
    destroy(static_cast<Instance*>(instance));
}

auto create_scene_thunk(void* instance, const Mesh_View& mesh) noexcept
    -> Result<void*> {
    auto built = create_scene(static_cast<Instance*>(instance), mesh);
    if (!built) [[unlikely]] {
        return std::unexpected(built.error());
    }
    return static_cast<void*>(*built);
}

void destroy_scene_thunk(void* scene) noexcept {
    destroy(static_cast<Scene*>(scene));
}

auto trace_thunk(void* scene, std::span<const Ray> rays,
                 std::span<Hit> hits) noexcept -> Result<void> {
    return trace(static_cast<Scene*>(scene), rays, hits);
}

}  // namespace

}  // namespace argus::sim::embree
