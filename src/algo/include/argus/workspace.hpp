#pragma once

#include <argus/coverage.hpp>
#include <argus/lidar.hpp>
#include <argus/mirror.hpp>
#include <argus/target.hpp>
#include <argus/types.hpp>

#ifdef ARGUS_ENABLE_SIM
#include <argus/sim/scene.hpp>
#endif

#include <cstdint>
#include <span>

namespace argus {

/// @brief Shared, read-only inputs of one optimization problem.
/// @details Every thread reads the same `Problem`. Pattern spans stored in
/// the lidars may be shared. `scene` is the world when the simulation
/// module is built. Fields with no `targets` entry stay at the value on the
/// lidar or mirror. `terms` is the in-memory score configuration.
struct Problem {
    std::span<const Target> targets;
    std::span<const Term> terms;
#ifdef ARGUS_ENABLE_SIM
    sim::Scene scene;
#endif
    uint8_t max_bounces = 1;
};

/// @brief Per-thread scratch for one objective evaluation.
/// @details The caller owns every span. Parallel evaluation uses one
/// `Workspace` per thread, each with its own lidar, mirror, hit, and scratch
/// storage. Pattern spans inside the lidars may alias a shared buffer.
/// `user` is untouched by the optimizer. `scratch` must be `scratch_words`
/// long for `combine_union`.
struct Workspace {
    const Problem* problem = nullptr;
    std::span<Lidar> lidars;
    std::span<Mirror> mirrors;
    std::span<Hit> hits;
    std::span<uint64_t> scratch;
    void* user = nullptr;
};

}  // namespace argus
