#include <pch.hpp>

#include <argus/optimize.hpp>

#include <argus/assert.hpp>
#include <argus/bound.hpp>
#include <argus/error.hpp>
#include <argus/workspace.hpp>

#include <span>

namespace argus {

auto optimize(std::span<const Bound> bounds, Objective objective,
              std::span<Workspace> workspaces, std::span<float> params) noexcept
    -> Result<float> {
    ARGUS_ASSERT(objective != nullptr);
    ARGUS_ASSERT(!workspaces.empty());
    ARGUS_ASSERT(params.size() == bounds.size());

    for (const Bound& bound : bounds) {
        ARGUS_ASSERT(bound.min <= bound.max);
    }
    return fail(Error::UNSUPPORTED, "optimize is not implemented yet!");
}

}  // namespace argus
