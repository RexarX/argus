#include <pch.hpp>

#include <argus/scan.hpp>

#include <argus/assert.hpp>
#include <argus/error.hpp>
#include <argus/lidar.hpp>
#include <argus/math.hpp>
#include <argus/memory/temporary_storage.hpp>
#include <argus/mirror.hpp>
#include <argus/scene.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <vector>

namespace argus::sim {

namespace {

constexpr size_t CHUNK = 256;
constexpr float MIRROR_T_MIN = 1.0e-4F;
constexpr float ORIGIN_OFFSET = 1.0e-3F;

struct Frame {
    float3 position = float3::Zero();
    float3 tangent = float3::Zero();
    float3 bitangent = float3::Zero();
    float3 normal = float3::Zero();
    float half_width = 0.0F;
    float half_height = 0.0F;
    uint16_t mirror = 0;
};

struct Mirror_Hit {
    float t = 0.0F;
    uint16_t index = 0;
    bool back = false;
};

struct Shot {
    Ray ray;
    float traveled = 0.0F;
    uint16_t skip = 0;
    uint16_t mirror = 0;
    uint8_t bounces = 0;
    bool has_skip = false;
    bool open = false;
};

[[nodiscard]] float facing(float3 direction, float3 normal) noexcept {
    const float length_sq = normal.squaredNorm();
    if (length_sq <= 1.0e-12F) {
        return 0.0F;
    }
    normal /= std::sqrt(length_sq);
    if (normal.dot(direction) > 0.0F) {
        normal = -normal;
    }
    return (-direction).dot(normal);
}

[[nodiscard]] bool nearest_mirror(const Ray& ray, std::span<const Frame> frames,
                                  uint16_t skip, bool has_skip, float limit,
                                  Mirror_Hit* hit) noexcept {
    bool found = false;
    float best = limit;
    for (size_t index = 0; index < frames.size(); ++index) {
        if (has_skip && index == skip) {
            continue;
        }
        const Frame& frame = frames[index];
        if (frame.half_width <= 0.0F || frame.half_height <= 0.0F) {
            continue;
        }

        const float denom = ray.direction.dot(frame.normal);
        if (std::abs(denom) <= 1.0e-8F) {
            continue;
        }
        const float t = (frame.position - ray.origin).dot(frame.normal) / denom;
        if (t <= MIRROR_T_MIN || t >= best) {
            continue;
        }

        const float3 point = ray.origin + ray.direction * t;
        const float3 relative = point - frame.position;
        const float along = relative.dot(frame.tangent);
        const float across = relative.dot(frame.bitangent);
        if (std::abs(along) > frame.half_width ||
            std::abs(across) > frame.half_height) {
            continue;
        }

        best = t;
        hit->t = t;
        hit->index = static_cast<uint16_t>(index);
        hit->back = denom > 0.0F;
        found = true;
    }
    return found;
}

[[nodiscard]] bool reflect(Shot* shot, const Frame& frame, float t,
                           uint16_t index) noexcept {
    float3 direction =
        shot->ray.direction -
        2.0F * shot->ray.direction.dot(frame.normal) * frame.normal;
    const float length_sq = direction.squaredNorm();
    if (length_sq <= 1.0e-12F) {
        return false;
    }
    direction /= std::sqrt(length_sq);
    if (t + ORIGIN_OFFSET >= shot->ray.max_distance) {
        return false;
    }

    const float3 point = shot->ray.origin + shot->ray.direction * t;
    shot->ray.origin = point + direction * ORIGIN_OFFSET;
    shot->ray.direction = direction;
    shot->ray.max_distance -= t + ORIGIN_OFFSET;
    shot->traveled += t + ORIGIN_OFFSET;
    shot->bounces = static_cast<uint8_t>(shot->bounces + 1U);
    shot->skip = index;
    shot->mirror = frame.mirror;
    shot->has_skip = true;
    return true;
}

void write_scene(Hit* dst, const Hit& scene_hit, const Shot& shot) noexcept {
    if (!scene_hit.hit) {
        *dst = {};
        return;
    }
    *dst = scene_hit;
    dst->distance = shot.traveled + scene_hit.distance;
    dst->bounces = shot.bounces;
    dst->mirror = shot.bounces == 0 ? uint16_t{0} : shot.mirror;
    dst->facing = facing(shot.ray.direction, scene_hit.normal);
}

[[nodiscard]] auto cast_lidar(Scene scene, const Lidar& lidar,
                              std::span<const Frame> frames,
                              uint8_t max_bounces, std::span<Hit> hits) noexcept
    -> Result<void> {
    const size_t count = lidar.pattern.directions.size();
    ARGUS_ASSERT(hits.size() == count);
    if (count == 0) {
        return {};
    }

    const float3x3 pose = rotation(
        lidar.orientation.yaw, lidar.orientation.pitch, lidar.orientation.roll);
    for (size_t base = 0; base < count; base += CHUNK) {
        const size_t batch = std::min(CHUNK, count - base);
        Shot shots[CHUNK] = {};
        for (size_t index = 0; index < batch; ++index) {
            const float3& sample = lidar.pattern.directions[base + index];
            ARGUS_ASSERT(std::abs(sample.squaredNorm() - 1.0F) <= 1.0e-4F);
            Shot& shot = shots[index];
            shot.ray.origin = lidar.position;
            shot.ray.direction = pose * scale_angle(sample, lidar.scale);
            shot.ray.max_distance = lidar.distance;
            shot.traveled = 0.0F;
            shot.skip = 0;
            shot.mirror = 0;
            shot.bounces = 0;
            shot.has_skip = false;
            shot.open = true;
        }

        while (true) {
            Ray rays[CHUNK] = {};
            Hit traced[CHUNK] = {};
            size_t slot[CHUNK] = {};
            size_t active = 0;
            for (size_t index = 0; index < batch; ++index) {
                if (!shots[index].open) {
                    continue;
                }
                slot[active] = index;
                rays[active] = shots[index].ray;
                ++active;
            }
            if (active == 0) {
                break;
            }

            const auto result = trace(scene, std::span{rays, active},
                                      std::span{traced, active});
            if (!result) [[unlikely]] {
                return fail(result.error(),
                            "Scene trace failed during mirror bounce!");
            }

            bool pending = false;
            for (size_t index = 0; index < active; ++index) {
                Shot& shot = shots[slot[index]];
                Hit& dst = hits[base + slot[index]];
                const Hit& scene_hit = traced[index];
                const float limit =
                    scene_hit.hit ? scene_hit.distance : shot.ray.max_distance;
                Mirror_Hit mirror;
                const bool blocked = nearest_mirror(
                    shot.ray, frames, shot.skip, shot.has_skip, limit, &mirror);
                if (!blocked) {
                    write_scene(&dst, scene_hit, shot);
                    shot.open = false;
                    continue;
                }
                if (mirror.back || shot.bounces >= max_bounces ||
                    !reflect(&shot, frames[mirror.index], mirror.t,
                             mirror.index)) {
                    dst = {};
                    shot.open = false;
                    continue;
                }
                pending = true;
            }
            if (!pending) {
                break;
            }
        }
    }
    return {};
}

}  // namespace

size_t get_ray_count(std::span<const Lidar> lidars) noexcept {
    size_t rays = 0;
    for (const Lidar& lidar : lidars) {
        const size_t count = lidar.pattern.directions.size();
        ARGUS_ASSERT(rays <= SIZE_MAX - count);
        rays += count;
    }
    return rays;
}

auto cast(Scene scene, std::span<const Lidar> lidars,
          std::span<const Mirror> mirrors, uint8_t max_bounces,
          std::span<Hit> hits) noexcept -> Result<void> {
    ARGUS_ASSERT(scene.ptr != nullptr);
    ARGUS_ASSERT(hits.size() == get_ray_count(lidars));
    ARGUS_ASSERT(mirrors.size() <= UINT16_MAX);

    auto temp = mem::get_temporary_resource();
    std::pmr::vector<Frame> frames{&temp};
    frames.reserve(mirrors.size());
    for (size_t index = 0; index < mirrors.size(); ++index) {
        const Mirror& mirror = mirrors[index];
        const float3x3 pose =
            rotation(mirror.orientation.yaw, mirror.orientation.pitch,
                     mirror.orientation.roll);
        Frame frame;
        frame.position = mirror.position;
        frame.tangent = pose.col(0);
        frame.bitangent = pose.col(1);
        frame.normal = pose.col(2);
        frame.half_width = mirror.extent.width * 0.5F;
        frame.half_height = mirror.extent.height * 0.5F;
        frame.mirror = static_cast<uint16_t>(index);
        frames.push_back(frame);
    }

    size_t offset = 0;
    for (const Lidar& lidar : lidars) {
        const size_t count = lidar.pattern.directions.size();
        const auto cast = cast_lidar(scene, lidar, frames, max_bounces,
                                     hits.subspan(offset, count));
        if (!cast) [[unlikely]] {
            return fail(cast.error(), "Lidar ray cast failed!");
        }
        offset += count;
    }
    return {};
}

}  // namespace argus::sim
