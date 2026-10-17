#pragma once

#include <argus/assert.hpp>
#include <argus/context.hpp>
#include <argus/error.hpp>
#include <argus/lidar.hpp>
#include <argus/math.hpp>
#include <argus/platform.hpp>
#include <argus/types.hpp>

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <span>
#include <vector>

namespace argus {

struct Workspace;

/// @brief Uniform voxel grid over a region of interest.
/// @details Voxel `(x, y, z)` covers `[min + voxel * index, min + voxel *
/// (index + 1))`. `dims` is the count along each axis.
struct Grid {
    float3 min = float3::Zero();
    float voxel = 1.0F;
    uint3 dims = uint3::Zero();
};

/// @brief Surface voxels and the area of scene surface inside each one.
/// @details `voxels` is sorted and unique. `area[i]` is the surface area, in
/// square meters, attributed to `voxels[i]`.
struct Surface {
    std::pmr::vector<uint32_t> voxels{std::pmr::get_default_resource()};
    std::pmr::vector<float> area{std::pmr::get_default_resource()};
};

/// @brief Empty surface whose containers use `allocator`.
/// @param allocator Resource for `voxels` and `area`
/// @return Surface with no voxels
/// @warning Asserts if `allocator` is null.
[[nodiscard]] inline Surface create_surface_with(
    std::pmr::memory_resource* allocator) noexcept {
    ARGUS_ASSERT(allocator != nullptr);
    return {
        .voxels = std::pmr::vector<uint32_t>{allocator},
        .area = std::pmr::vector<float>{allocator},
    };
}

/// @brief Minimum `Hit::facing` accepted by `accept_facing`.
/// @details `0` accepts every hit with a usable normal. `facing` is a cosine,
/// so a 70 degree incidence limit is `cos(70)`.
struct Facing_Limit {
    float min_facing = 0.0F;
};

/// @brief One evaluation's inputs.
/// @details Built-in combiners read `scratch` as mask words. A metric may
/// ignore it. `surface` is available for a caller-written metric; the built-in
/// metrics read their `Coverage` instead.
struct Eval {
    std::span<const Lidar> lidars;
    std::span<const Hit> hits;
    const Surface* surface = nullptr;
    std::span<uint64_t> scratch;
};

/// @brief Decides whether one hit may cover a voxel.
/// @param hit Hit to test. Misses are not passed in
/// @param user Metric settings, often a `Facing_Limit`
/// @return `true` when the hit counts
using Accept = bool (*)(const Hit& hit, void* user) noexcept;

/// @brief Accepts a hit whose `facing` is at least `Facing_Limit::min_facing`.
/// @param hit Hit to test
/// @param user Pointer to a `Facing_Limit`
/// @return `true` when `hit.facing` meets the limit
/// @warning Asserts if `user` is null.
[[nodiscard]] inline bool accept_facing(const Hit& hit, void* user) noexcept {
    ARGUS_ASSERT(user != nullptr);
    const auto* limit = static_cast<const Facing_Limit*>(user);
    return hit.facing >= limit->min_facing;
}

struct Coverage;

/// @brief Merges accepted hits into one fraction per group.
/// @details Writes `group_coverage[g]` as the fraction of `targets` credited
/// to group `g`, before `Coverage::group_weights`. Fills `masks` with one bit
/// per group per target. `masks` is `scratch_words` long.
/// @param eval Lidars, hits, and scratch
/// @param coverage Grid, targets, groups, and the accept hook
/// @param masks Output bitsets, one word-row per target
/// @param group_coverage Output fractions, one per group weight
using Combine = void (*)(const Eval& eval, const Coverage& coverage,
                         std::span<uint64_t> masks,
                         std::span<float> group_coverage) noexcept;

/// @brief Settings for the built-in coverage metrics.
/// @details `groups[i]` is the group of lidar `i` in encounter order, and must
/// be less than `group_weights.size()`. A group's coverage is whatever
/// `combine` writes. `combine_union` is the union of that group's hits, so two
/// lidars in one group that see the same voxel add nothing. `accept == nullptr`
/// counts every hit. `combine == nullptr` uses `combine_union`.
struct Coverage {
    Grid grid;
    std::span<const uint32_t> targets;
    std::span<const float> area;
    std::span<const uint8_t> groups;
    std::span<const float> group_weights;
    Accept accept = nullptr;
    void* accept_user = nullptr;
    Combine combine = nullptr;
    void* combine_user = nullptr;
};

/// @brief Words of `scratch` that `combine_union` needs.
/// @param target_count Number of surface voxels
/// @param group_count Number of group weights
/// @return `target_count * ceil(group_count / 64)`, or `0` when either count
/// is `0`
/// @warning Asserts if the product overflows.
[[nodiscard]] constexpr size_t scratch_words(size_t target_count,
                                             size_t group_count) noexcept {
    if (target_count == 0 || group_count == 0) {
        return 0;
    }
    const size_t words = (group_count + 63) / 64;
    ARGUS_ASSERT(target_count <= SIZE_MAX / words);
    return target_count * words;
}

/// @brief Voxel that contains `point`, biased off a grid plane.
/// @details Shifts `point` by `grid.voxel * 1e-3` against `normal` before the
/// index is taken, so a sample that lands on a voxel face stays with the
/// solid side of that face. A degenerate `normal` is not shifted. The result
/// is empty when the shifted point is outside the grid or not finite.
/// @param grid Region of interest
/// @param point Sample position
/// @param normal Winding normal at `point`. Zero skips the shift
/// @return Voxel id, or empty when the point is outside
/// @warning Asserts if `grid.voxel` is not positive.
[[nodiscard]] ARGUS_API auto voxel_id(const Grid& grid, float3 point,
                                      float3 normal) noexcept
    -> std::optional<uint32_t>;

/// @brief Voxel ids and area of surface inside `grid`.
/// @details Samples each triangle at half-voxel spacing. Each triangle's area
/// is split evenly across its samples that land in the grid. The voxels are
/// then sorted and the area for one voxel is summed. The result is empty when
/// `dims` is zero.
/// @param context Allocator for the returned vectors
/// @param mesh Scene triangles. Indices must land in `vertices`
/// @param grid Region of interest
/// @return Sorted voxels and parallel areas, or `Error::INVALID` when `voxel`
/// is not positive, the grid does not fit in `uint32_t`, or a triangle index
/// is out of range
/// @warning Asserts if `context.allocator` is null.
[[nodiscard]] ARGUS_API auto surface_voxels(const Context& context,
                                            const Mesh_View& mesh,
                                            const Grid& grid) noexcept
    -> Result<Surface>;

/// @brief Union combine. Overlap inside a group counts once.
/// @param eval Lidars and hits to read
/// @param coverage Grid, targets, groups, and the accept hook
/// @param masks Output bitsets. One bit per group, `scratch_words` long
/// @param group_coverage Unweighted fraction of `targets` per group
/// @warning Asserts in next cases:
/// - If `group_weights` has more than 256 entries.
/// - If `groups` and `eval.lidars` differ in size.
/// - If a group index is not less than `group_weights.size()`.
/// - If `masks` is not `scratch_words` long.
/// - If `group_coverage` and `group_weights` differ in size.
/// - If `hits` and the concatenated lidar patterns differ in size.
ARGUS_API void combine_union(const Eval& eval, const Coverage& coverage,
                             std::span<uint64_t> masks,
                             std::span<float> group_coverage) noexcept;

/// @brief One weighted metric in a `score` sum.
/// @details `metric` returns a scalar. `weight` scales it. A negative weight
/// minimizes. `user` is the metric's settings and is often a `Coverage`.
struct Term {
    using Function = float (*)(const Eval& eval, void* user) noexcept;

    Function metric = nullptr;
    void* user = nullptr;
    float weight = 1.0F;
};

/// @brief Weighted sum of metric outputs. Higher is better.
/// @param terms Metrics to evaluate. Empty scores `0`
/// @param eval Lidars, hits, and scratch shared by every term
/// @return Sum of `weight * metric(eval, user)`
/// @warning Asserts if a term's `metric` is null.
[[nodiscard]] ARGUS_API float score(std::span<const Term> terms,
                                    const Eval& eval) noexcept;

/// @brief Grouped surface fraction, then scaled by `group_weights`.
/// @details `user` is a `Coverage`. Uses `coverage.combine`, or `combine_union`
/// when that is null.
/// @param eval Lidars, hits, and scratch
/// @param user Pointer to a `Coverage`
/// @return Weighted sum of per-group fractions
/// @warning Asserts if `user` is null.
[[nodiscard]] ARGUS_API float coverage_metric(const Eval& eval,
                                              void* user) noexcept;

/// @brief Surface area of target voxels no group covered.
/// @details Runs the same combine as `coverage_metric`, then sums
/// `Coverage::area` where every group bit is clear.
/// @param eval Lidars, hits, and scratch
/// @param user Pointer to a `Coverage`
/// @return Unseen area, in square meters
/// @warning Asserts in next cases:
/// - If `user` is null.
/// - If `area` and `targets` differ in size.
[[nodiscard]] ARGUS_API float blind_area_metric(const Eval& eval,
                                                void* user) noexcept;

/// @brief Coefficient of variation of hit counts on covered voxels.
/// @details Counts accepted hits per target voxel. Voxels with no hits are
/// left out. Returns `0` when nothing was hit.
/// @param eval Lidars and hits
/// @param user Pointer to a `Coverage`
/// @return Population standard deviation divided by the mean
/// @warning Asserts if `user` is null.
[[nodiscard]] ARGUS_API float hit_density_cv_metric(const Eval& eval,
                                                    void* user) noexcept;

/// @brief Applies `params`, casts, and returns `score`.
/// @details The callback for `optimize`. `workspace->problem` holds the
/// targets, the term list, and the scene.
/// @param params Candidate, one value per target
/// @param workspace Per-thread lidars, mirrors, hit buffer, and scratch
/// @return Weighted metric sum. `0` when the cast fails
/// @warning Asserts in next cases:
/// - If `workspace` or `workspace->problem` is null.
/// - If the cast fails.
[[nodiscard]] ARGUS_API float evaluate(std::span<const float> params,
                                       Workspace* workspace) noexcept;

}  // namespace argus
