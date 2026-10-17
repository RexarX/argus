#include <pch.hpp>

#include <argus/coverage.hpp>

#include <argus/assert.hpp>
#include <argus/error.hpp>
#include <argus/lidar.hpp>
#include <argus/math.hpp>
#include <argus/memory/temporary_storage.hpp>
#include <argus/target.hpp>
#include <argus/types.hpp>
#include <argus/workspace.hpp>

#ifdef ARGUS_ENABLE_SIM
#include <argus/sim/scan.hpp>
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <span>
#include <vector>

namespace argus {

namespace {

constexpr float VOXEL_BIAS = 1.0e-3F;
constexpr size_t GROUP_LIMIT = 256;

struct Weighted {
    uint32_t id = 0;
    float area = 0.0F;
};

[[nodiscard]] bool grid_fits(const Grid& grid) noexcept {
    const uint64_t dx = grid.dims[0];
    const uint64_t dy = grid.dims[1];
    const uint64_t dz = grid.dims[2];
    if (dy != 0 && dx > UINT64_MAX / dy) {
        return false;
    }

    const uint64_t xy = dx * dy;
    if (dz != 0 && xy > UINT64_MAX / dz) {
        return false;
    }

    return xy * dz <= UINT32_MAX;
}

[[nodiscard]] size_t pattern_rays(std::span<const Lidar> lidars) noexcept {
    size_t rays = 0;
    for (const Lidar& lidar : lidars) {
        const size_t count = lidar.pattern.directions.size();
        ARGUS_ASSERT(rays <= SIZE_MAX - count);
        rays += count;
    }
    return rays;
}

[[nodiscard]] bool accepted(const Coverage& coverage, const Hit& hit) noexcept {
    if (!hit.hit) {
        return false;
    }
    if (coverage.accept == nullptr) {
        return true;
    }
    return coverage.accept(hit, coverage.accept_user);
}

[[nodiscard]] size_t group_words(size_t group_count) noexcept {
    if (group_count == 0) {
        return 0;
    }
    return (group_count + 63) / 64;
}

void run_combine(const Eval& eval, const Coverage& coverage,
                 std::span<float> group_coverage) noexcept {
    const Combine combine =
        coverage.combine != nullptr ? coverage.combine : combine_union;
    combine(eval, coverage, eval.scratch, group_coverage);
}

}  // namespace

auto voxel_id(const Grid& grid, float3 point, float3 normal) noexcept
    -> std::optional<uint32_t> {
    ARGUS_ASSERT(grid.voxel > 0.0F);

    const float length_sq = normal.squaredNorm();
    if (length_sq > 1.0e-12F) {
        point -= normal * ((grid.voxel * VOXEL_BIAS) / std::sqrt(length_sq));
    }

    const float x = (point[0] - grid.min[0]) / grid.voxel;
    const float y = (point[1] - grid.min[1]) / grid.voxel;
    const float z = (point[2] - grid.min[2]) / grid.voxel;
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
        return std::nullopt;
    }

    if (x < 0.0F || y < 0.0F || z < 0.0F) {
        return std::nullopt;
    }

    if (x >= static_cast<float>(grid.dims[0]) ||
        y >= static_cast<float>(grid.dims[1]) ||
        z >= static_cast<float>(grid.dims[2])) {
        return std::nullopt;
    }

    const auto ix = static_cast<uint32_t>(x);
    const auto iy = static_cast<uint32_t>(y);
    const auto iz = static_cast<uint32_t>(z);
    return (iz * grid.dims[1] + iy) * grid.dims[0] + ix;
}

auto surface_voxels(const Context& context, const Mesh_View& mesh,
                    const Grid& grid) noexcept -> Result<Surface> {
    ARGUS_ASSERT(context.allocator != nullptr,
                 "surface_voxels needs an allocator!");
    if (!(grid.voxel > 0.0F) || !grid_fits(grid)) [[unlikely]] {
        return fail(Error::INVALID,
                    "Coverage grid has invalid voxel size or too many voxels!");
    }

    if (grid.dims[0] == 0 || grid.dims[1] == 0 || grid.dims[2] == 0) {
        return create_surface_with(context.allocator);
    }

    const size_t vertex_count = mesh.vertices.size();
    const float step = grid.voxel * 0.5F;
    auto temp = mem::get_temporary_resource();
    std::pmr::vector<uint32_t> local{&temp};
    std::pmr::vector<Weighted> samples{&temp};
    for (const uint3& triangle : mesh.triangles) {
        if (triangle[0] >= vertex_count || triangle[1] >= vertex_count ||
            triangle[2] >= vertex_count) [[unlikely]] {
            return fail(
                Error::INVALID,
                "Mesh triangle references a vertex index out of range!");
        }

        const float3& v0 = mesh.vertices[triangle[0]];
        const float3 edge1 = mesh.vertices[triangle[1]] - v0;
        const float3 edge2 = mesh.vertices[triangle[2]] - v0;
        const float length1 = edge1.norm();
        const float length2 = edge2.norm();
        if (!std::isfinite(length1) || !std::isfinite(length2)) [[unlikely]] {
            return fail(Error::INVALID,
                        "Mesh triangle has non-finite edge length!");
        }

        const float3 cross = edge1.cross(edge2);
        const float cross_length = cross.norm();
        if (!std::isfinite(cross_length)) [[unlikely]] {
            return fail(Error::INVALID,
                        "Mesh triangle has a non-finite normal!");
        }
        const float area = 0.5F * cross_length;

        const float count1 = std::ceil(length1 / step);
        const float count2 = std::ceil(length2 / step);
        if (count1 > 1.0e6F || count2 > 1.0e6F) [[unlikely]] {
            return fail(
                Error::INVALID,
                "Mesh triangle is too large for the coverage grid step!");
        }
        const int steps1 = count1 < 1.0F ? 1 : static_cast<int>(count1);
        const int steps2 = count2 < 1.0F ? 1 : static_cast<int>(count2);

        local.clear();
        for (int i = 0; i <= steps1; ++i) {
            const float a = static_cast<float>(i) / static_cast<float>(steps1);
            for (int j = 0; j <= steps2; ++j) {
                const float b =
                    static_cast<float>(j) / static_cast<float>(steps2);
                if (a + b > 1.0F) {
                    continue;
                }
                const float3 point = v0 + edge1 * a + edge2 * b;
                if (const auto id = voxel_id(grid, point, cross)) {
                    local.push_back(*id);
                }
            }
        }

        if (local.empty()) {
            continue;
        }

        const float weight = area / static_cast<float>(local.size());
        for (const uint32_t id : local) {
            samples.push_back({.id = id, .area = weight});
        }
    }

    std::ranges::sort(samples, [](const Weighted& lhs, const Weighted& rhs) {
        return lhs.id < rhs.id;
    });

    Surface surface = create_surface_with(context.allocator);
    for (const Weighted& sample : samples) {
        if (!surface.voxels.empty() && surface.voxels.back() == sample.id) {
            surface.area.back() += sample.area;
            continue;
        }
        surface.voxels.push_back(sample.id);
        surface.area.push_back(sample.area);
    }
    return surface;
}

void combine_union(const Eval& eval, const Coverage& coverage,
                   std::span<uint64_t> masks,
                   std::span<float> group_coverage) noexcept {
    const size_t group_count = coverage.group_weights.size();
    const size_t target_count = coverage.targets.size();
    ARGUS_ASSERT(group_count <= GROUP_LIMIT);
    ARGUS_ASSERT(coverage.groups.size() == eval.lidars.size());
    ARGUS_ASSERT(group_coverage.size() == group_count);
    ARGUS_ASSERT(masks.size() == scratch_words(target_count, group_count));
    ARGUS_ASSERT(eval.hits.size() == pattern_rays(eval.lidars));

    const size_t words = group_words(group_count);
    std::ranges::fill(masks, 0);
    std::ranges::fill(group_coverage, 0.0F);
    if (target_count == 0 || group_count == 0) {
        return;
    }

    uint32_t counts[GROUP_LIMIT] = {};
    size_t cursor = 0;
    for (size_t lidar_ordinal = 0; lidar_ordinal < eval.lidars.size();
         ++lidar_ordinal) {
        const Lidar& lidar = eval.lidars[lidar_ordinal];
        const uint8_t group = coverage.groups[lidar_ordinal];
        ARGUS_ASSERT(static_cast<size_t>(group) < group_count);
        const uint64_t bit = uint64_t{1} << (group % 64U);
        const size_t word = static_cast<size_t>(group) / 64;
        const size_t count = lidar.pattern.directions.size();
        for (size_t sample = 0; sample < count; ++sample) {
            const Hit& hit = eval.hits[cursor++];
            if (!accepted(coverage, hit)) {
                continue;
            }
            const auto id = voxel_id(coverage.grid, hit.position, hit.normal);
            if (!id) {
                continue;
            }
            const auto found = std::ranges::lower_bound(coverage.targets, *id);
            if (found == coverage.targets.end() || *found != *id) {
                continue;
            }
            const auto target =
                static_cast<size_t>(found - coverage.targets.begin());
            uint64_t& mask = masks[(target * words) + word];
            if ((mask & bit) != 0U) {
                continue;
            }
            mask |= bit;
            counts[group] += 1U;
        }
    }

    ARGUS_ASSERT(cursor == eval.hits.size());

    const float scale = 1.0F / static_cast<float>(target_count);
    for (size_t group = 0; group < group_count; ++group) {
        group_coverage[group] = static_cast<float>(counts[group]) * scale;
    }
}

float score(std::span<const Term> terms, const Eval& eval) noexcept {
    float score = 0.0F;
    for (const Term& term : terms) {
        ARGUS_ASSERT(term.metric != nullptr);
        score += term.weight * term.metric(eval, term.user);
    }
    return score;
}

float coverage_metric(const Eval& eval, void* user) noexcept {
    ARGUS_ASSERT(user != nullptr);
    const auto& coverage = *static_cast<const Coverage*>(user);
    const size_t group_count = coverage.group_weights.size();
    ARGUS_ASSERT(group_count <= GROUP_LIMIT);

    float storage[GROUP_LIMIT] = {};
    const std::span<float> fraction{storage, group_count};
    run_combine(eval, coverage, fraction);

    float score = 0.0F;
    for (size_t group = 0; group < group_count; ++group) {
        score += coverage.group_weights[group] * fraction[group];
    }
    return score;
}

float blind_area_metric(const Eval& eval, void* user) noexcept {
    ARGUS_ASSERT(user != nullptr);
    const auto& coverage = *static_cast<const Coverage*>(user);
    ARGUS_ASSERT(coverage.area.size() == coverage.targets.size());
    const size_t group_count = coverage.group_weights.size();
    ARGUS_ASSERT(group_count <= GROUP_LIMIT);

    float storage[GROUP_LIMIT] = {};
    run_combine(eval, coverage, {storage, group_count});

    const size_t words = group_words(group_count);
    float blind = 0.0F;
    for (size_t target = 0; target < coverage.targets.size(); ++target) {
        bool covered = false;
        for (size_t word = 0; word < words; ++word) {
            if (eval.scratch[(target * words) + word] != 0U) {
                covered = true;
                break;
            }
        }
        if (!covered) {
            blind += coverage.area[target];
        }
    }
    return blind;
}

float hit_density_cv_metric(const Eval& eval, void* user) noexcept {
    ARGUS_ASSERT(user != nullptr);
    const auto& coverage = *static_cast<const Coverage*>(user);
    ARGUS_ASSERT(eval.hits.size() == pattern_rays(eval.lidars));
    const size_t target_count = coverage.targets.size();
    if (target_count == 0) {
        return 0.0F;
    }

    auto temp = mem::get_temporary_resource();
    std::pmr::vector<uint32_t> counts{target_count, &temp};
    size_t cursor = 0;
    for (const Lidar& lidar : eval.lidars) {
        const size_t count = lidar.pattern.directions.size();
        for (size_t sample = 0; sample < count; ++sample) {
            const Hit& hit = eval.hits[cursor++];
            if (!accepted(coverage, hit)) {
                continue;
            }
            const auto id = voxel_id(coverage.grid, hit.position, hit.normal);
            if (!id) {
                continue;
            }
            const auto found = std::ranges::lower_bound(coverage.targets, *id);
            if (found == coverage.targets.end() || *found != *id) {
                continue;
            }
            const auto target =
                static_cast<size_t>(found - coverage.targets.begin());
            counts[target] += 1U;
        }
    }
    ARGUS_ASSERT(cursor == eval.hits.size());

    double sum = 0.0;
    size_t covered = 0;
    for (const uint32_t count : counts) {
        if (count == 0U) {
            continue;
        }
        sum += static_cast<double>(count);
        ++covered;
    }

    if (covered == 0) {
        return 0.0F;
    }

    const double mean = sum / static_cast<double>(covered);
    double variance = 0.0;
    for (const uint32_t count : counts) {
        if (count == 0U) {
            continue;
        }
        const double delta = static_cast<double>(count) - mean;
        variance += delta * delta;
    }
    variance /= static_cast<double>(covered);
    if (mean == 0.0) {
        return 0.0F;
    }
    return static_cast<float>(std::sqrt(variance) / mean);
}

float evaluate(std::span<const float> params, Workspace* workspace) noexcept {
    ARGUS_ASSERT(workspace != nullptr);
    ARGUS_ASSERT(workspace->problem != nullptr);

    const Problem& problem = *workspace->problem;
    apply(problem.targets, workspace->lidars, workspace->mirrors, params);

#ifdef ARGUS_ENABLE_SIM
    const auto cast =
        sim::cast(problem.scene, workspace->lidars, workspace->mirrors,
                  problem.max_bounces, workspace->hits);
    ARGUS_ASSERT(cast);
    if (!cast) [[unlikely]] {
        return 0.0F;
    }
#endif

    Eval eval;
    eval.lidars = workspace->lidars;
    eval.hits = workspace->hits;
    eval.scratch = workspace->scratch;
    return score(problem.terms, eval);
}

}  // namespace argus
