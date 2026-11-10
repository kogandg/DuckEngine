#pragma once

#include <glm/glm.hpp>

#include <numbers>
#include <algorithm>

namespace Duck
{
    using std::int32_t;
    using std::int64_t;
    using std::size_t;
    using std::uint32_t;
    using std::uint64_t;
    using std::uint8_t;

    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;

    using IVec2 = glm::ivec2;
    using IVec3 = glm::ivec3;
    using IVec4 = glm::ivec4;

    using UVec2 = glm::uvec2;
    using UVec3 = glm::uvec3;
    using UVec4 = glm::uvec4;

    using Mat3 = glm::mat3;
    using Mat4 = glm::mat4;
    using Quat = glm::quat;

    inline constexpr float Pi = std::numbers::pi_v<float>;
    inline constexpr float TwoPi = 2.0f * Pi;
    inline constexpr float HalfPi = 0.5f * Pi;
    inline constexpr float InvPi = 1.0f / Pi;
    inline constexpr float Epsilon = 1e-6f;
    inline constexpr float Infinity = std::numeric_limits<float>::infinity();

    constexpr float Radians(float degrees) { return degrees * (Pi / 180.0f); }
    constexpr float Degrees(float radians) { return radians * (180.0f / Pi); }

    inline float MaxComponent(const Vec3& v) { return std::max({v.x, v.y, v.z}); }
}