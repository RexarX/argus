#pragma once

#include <Eigen/Geometry>

#include <cmath>
#include <cstdint>

namespace argus {

using int2 = Eigen::Vector2i;
using int3 = Eigen::Vector3i;
using int4 = Eigen::Vector4i;

using uint2 = Eigen::Vector2<uint32_t>;
using uint3 = Eigen::Vector3<uint32_t>;
using uint4 = Eigen::Vector4<uint32_t>;

using float2 = Eigen::Vector2f;
using float3 = Eigen::Vector3f;
using float4 = Eigen::Vector4f;

using int2x2 = Eigen::Matrix2i;
using int2x3 = Eigen::Matrix<int32_t, 2, 3>;
using int2x4 = Eigen::Matrix<int32_t, 2, 4>;
using int3x2 = Eigen::Matrix<int32_t, 3, 2>;
using int3x3 = Eigen::Matrix3i;
using int3x4 = Eigen::Matrix<int32_t, 3, 4>;
using int4x2 = Eigen::Matrix<int32_t, 4, 2>;
using int4x3 = Eigen::Matrix<int32_t, 4, 3>;
using int4x4 = Eigen::Matrix4i;

using uint2x2 = Eigen::Matrix<uint32_t, 2, 2>;
using uint2x3 = Eigen::Matrix<uint32_t, 2, 3>;
using uint2x4 = Eigen::Matrix<uint32_t, 2, 4>;
using uint3x2 = Eigen::Matrix<uint32_t, 3, 2>;
using uint3x3 = Eigen::Matrix<uint32_t, 3, 3>;
using uint3x4 = Eigen::Matrix<uint32_t, 3, 4>;
using uint4x2 = Eigen::Matrix<uint32_t, 4, 2>;
using uint4x3 = Eigen::Matrix<uint32_t, 4, 3>;
using uint4x4 = Eigen::Matrix<uint32_t, 4, 4>;

using float2x2 = Eigen::Matrix2f;
using float2x3 = Eigen::Matrix<float, 2, 3>;
using float2x4 = Eigen::Matrix<float, 2, 4>;
using float3x2 = Eigen::Matrix<float, 3, 2>;
using float3x3 = Eigen::Matrix3f;
using float3x4 = Eigen::Matrix<float, 3, 4>;
using float4x2 = Eigen::Matrix<float, 4, 2>;
using float4x3 = Eigen::Matrix<float, 4, 3>;
using float4x4 = Eigen::Matrix4f;

/// @brief Orthonormal frame for yaw, pitch, and roll.
/// @details Columns are right (+X), up (+Y), and forward (+Z) in world
/// space. YAW rotates about world +Y, pitch rises toward +Y, and roll
/// twists about forward. YAW 0 and pitch 0 points forward along +Z. ROLL 0
/// keeps up on world +Y when forward is +Z. Angles are radians.
/// @param yaw rotation about world +Y
/// @param pitch rotation toward world +Y
/// @param roll Twist about the forward axis
/// @return Frame whose columns are right, up, and forward
/// @note PITCH near ±π/2 is singular. The frame stays orthonormal, but yaw
/// and roll trade off. Keep pitch bounds inside that open range.
[[nodiscard]] inline float3x3 rotation(float yaw, float pitch,
                                       float roll) noexcept {
    const float cosine_pitch = std::cos(pitch);
    const float3 forward{cosine_pitch * std::sin(yaw), std::sin(pitch),
                         cosine_pitch * std::cos(yaw)};

    float3 right = float3::UnitY().cross(forward);
    float length_sq = right.squaredNorm();
    if (length_sq <= 1.0e-8F) {
        right = float3::UnitZ().cross(forward);
        length_sq = right.squaredNorm();
    }
    right /= std::sqrt(length_sq);
    const float3 up0 = forward.cross(right);

    const float cosine_roll = std::cos(roll);
    const float sine_roll = std::sin(roll);
    const float3 up = up0 * cosine_roll - right * sine_roll;
    right = right * cosine_roll + up0 * sine_roll;

    float3x3 frame;
    frame.col(0) = right;
    frame.col(1) = up;
    frame.col(2) = forward;
    return frame;
}

}  // namespace argus
