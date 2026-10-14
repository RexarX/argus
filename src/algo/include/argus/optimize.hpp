#pragma once

#include <argus/error.hpp>
#include <argus/platform.hpp>

#include <span>

namespace argus {

struct Workspace;
struct Bound;

/// @brief Scores one candidate. Higher is better.
/// @details The callback owns the metric. `evaluate` applies `params`, calls
/// `cast`, and reduces the hits with `score`. `workspace->user` carries
/// anything a custom callback needs.
/// @param params Candidate, one value per bound
/// @param workspace Per-thread sensors, hit buffers, and scene
/// @return Objective value
using Objective = float (*)(std::span<const float> params,
                            Workspace* workspace) noexcept;

/// @brief Searches `bounds` for the parameter vector that raises `objective`.
/// @details The search does not read lidars, mirrors, or hits. `params`
/// enters as the initial guess and, on success, holds the best vector.
/// Pass one `Workspace` per thread that may evaluate a candidate.
/// @param bounds Inclusive range of each dimension
/// @param objective score callback
/// @param workspaces Per-thread scratch. At least one
/// @param params Initial guess on input, best vector on success
/// @return Best objective value, or `Error::UNSUPPORTED` until a search is
/// implemented
/// @warning Asserts in next cases:
/// - If `objective` is null.
/// - If `workspaces` is empty.
/// - If `params` and `bounds` differ in size.
/// - If a bound has `min > max`.
[[nodiscard]] ARGUS_API auto optimize(std::span<const Bound> bounds,
                                      Objective objective,
                                      std::span<Workspace> workspaces,
                                      std::span<float> params) noexcept
    -> Result<float>;

}  // namespace argus
