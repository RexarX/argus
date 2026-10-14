#include <pch.hpp>

#include <argus/target.hpp>

#include <argus/assert.hpp>
#include <argus/bound.hpp>
#include <argus/error.hpp>
#include <argus/lidar.hpp>
#include <argus/mirror.hpp>

#include <cstddef>
#include <span>

namespace argus {

namespace {

[[nodiscard]] constexpr bool on_lidar(Variable variable) noexcept {
    switch (variable) {
        using enum Variable;
        case X:
        case Y:
        case Z:
        case YAW:
        case PITCH:
        case ROLL:
        case SCALE:
        case RANGE:
            return true;
        case WIDTH:
        case HEIGHT:
            return false;
    }
    return false;
}

[[nodiscard]] constexpr bool on_mirror(Variable variable) noexcept {
    switch (variable) {
        using enum Variable;
        case X:
        case Y:
        case Z:
        case YAW:
        case PITCH:
        case ROLL:
        case WIDTH:
        case HEIGHT:
            return true;
        case SCALE:
        case RANGE:
            return false;
    }
    return false;
}

[[nodiscard]] constexpr bool positive_minimum(Variable variable) noexcept {
    return variable == Variable::SCALE || variable == Variable::RANGE ||
           variable == Variable::WIDTH || variable == Variable::HEIGHT;
}

[[nodiscard]] float* field(Lidar* lidar, Variable variable) noexcept {
    switch (variable) {
        using enum Variable;
        case X:
            return &lidar->position.coeffRef(0);
        case Y:
            return &lidar->position.coeffRef(1);
        case Z:
            return &lidar->position.coeffRef(2);
        case YAW:
            return &lidar->orientation.yaw;
        case PITCH:
            return &lidar->orientation.pitch;
        case ROLL:
            return &lidar->orientation.roll;
        case SCALE:
            return &lidar->scale;
        case RANGE:
            return &lidar->distance;
        case WIDTH:
        case HEIGHT:
            break;
    }
    ARGUS_ASSERT(false, "Variable '{}' is not on a lidar!",
                 to_string(variable));
    return &lidar->position.coeffRef(0);
}

[[nodiscard]] float* field(Mirror* mirror, Variable variable) noexcept {
    switch (variable) {
        using enum Variable;
        case X:
            return &mirror->position.coeffRef(0);
        case Y:
            return &mirror->position.coeffRef(1);
        case Z:
            return &mirror->position.coeffRef(2);
        case YAW:
            return &mirror->orientation.yaw;
        case PITCH:
            return &mirror->orientation.pitch;
        case ROLL:
            return &mirror->orientation.roll;
        case WIDTH:
            return &mirror->extent.width;
        case HEIGHT:
            return &mirror->extent.height;
        case SCALE:
        case RANGE:
            break;
    }
    ARGUS_ASSERT(false, "Variable '{}' is not on a mirror!",
                 to_string(variable));
    return &mirror->position.coeffRef(0);
}

[[nodiscard]] size_t subject_count(Subject subject, size_t lidars,
                                   size_t mirrors) noexcept {
    return subject == Subject::LIDAR ? lidars : mirrors;
}

}  // namespace

auto validate(std::span<const Target> targets, std::span<const Lidar> lidars,
              std::span<const Mirror> mirrors) noexcept -> Result<void> {
    for (size_t index = 0; index < targets.size(); ++index) {
        const Target& target = targets[index];
        if (to_string(target.variable) == "Unknown") [[unlikely]] {
            return fail(Error::INVALID, "Target '{}': unknown variable '{}'!",
                        index, to_string(target.variable));
        }
        if (target.bounded && target.min > target.max) [[unlikely]] {
            return fail(Error::INVALID,
                        "Target '{}': min={} is greater than max={}!", index,
                        target.min, target.max);
        }
        if (target.bounded && positive_minimum(target.variable) &&
            target.min <= 0.0F) [[unlikely]] {
            return fail(Error::INVALID,
                        "Target '{}': '{}' requires a positive minimum!", index,
                        to_string(target.variable));
        }

        const bool fits = target.subject == Subject::LIDAR
                              ? on_lidar(target.variable)
                              : on_mirror(target.variable);
        if (!fits) [[unlikely]] {
            return fail(
                Error::INVALID,
                "Target '{}': variable '{}' does not belong to subject {}!",
                index, to_string(target.variable),
                static_cast<int>(target.subject));
        }

        const size_t count =
            subject_count(target.subject, lidars.size(), mirrors.size());
        if (static_cast<size_t>(target.index) >= count) [[unlikely]] {
            return fail(Error::INVALID,
                        "Target '{}': index={} out of range (count={})!", index,
                        target.index, count);
        }

        for (size_t earlier = 0; earlier < index; ++earlier) {
            const Target& other = targets[earlier];
            if (other.subject == target.subject &&
                other.index == target.index &&
                other.variable == target.variable) [[unlikely]] {
                return fail(Error::INVALID,
                            "Target '{}' duplicates target '{}' on index {} "
                            "variable={}!",
                            index, earlier, target.index,
                            to_string(target.variable));
            }
        }
    }
    return {};
}

void copy_bounds(std::span<const Target> targets,
                 std::span<Bound> bounds) noexcept {
    ARGUS_ASSERT(bounds.size() == targets.size());
    for (size_t index = 0; index < targets.size(); ++index) {
        ARGUS_ASSERT(targets[index].bounded);
        ARGUS_ASSERT(targets[index].min <= targets[index].max);
        bounds[index].min = targets[index].min;
        bounds[index].max = targets[index].max;
    }
}

void read(std::span<const Target> targets, std::span<const Lidar> lidars,
          std::span<const Mirror> mirrors, std::span<float> params) noexcept {
    ARGUS_ASSERT(params.size() == targets.size());
    for (size_t index = 0; index < targets.size(); ++index) {
        const Target& target = targets[index];
        const size_t count =
            subject_count(target.subject, lidars.size(), mirrors.size());
        ARGUS_ASSERT(static_cast<size_t>(target.index) < count);
        if (target.subject == Subject::LIDAR) {
            ARGUS_ASSERT(on_lidar(target.variable));
            params[index] = *field(const_cast<Lidar*>(&lidars[target.index]),
                                   target.variable);
            continue;
        }
        ARGUS_ASSERT(on_mirror(target.variable));
        params[index] = *field(const_cast<Mirror*>(&mirrors[target.index]),
                               target.variable);
    }
}

void apply(std::span<const Target> targets, std::span<Lidar> lidars,
           std::span<Mirror> mirrors, std::span<const float> params) noexcept {
    ARGUS_ASSERT(params.size() == targets.size());
    for (size_t index = 0; index < targets.size(); ++index) {
        const Target& target = targets[index];
        const size_t count =
            subject_count(target.subject, lidars.size(), mirrors.size());
        ARGUS_ASSERT(static_cast<size_t>(target.index) < count);
        if (target.subject == Subject::LIDAR) {
            ARGUS_ASSERT(on_lidar(target.variable));
            *field(&lidars[target.index], target.variable) = params[index];
            continue;
        }
        ARGUS_ASSERT(on_mirror(target.variable));
        *field(&mirrors[target.index], target.variable) = params[index];
    }
}

}  // namespace argus
