#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Core/Random.h>

#include <cmath>

namespace Duck
{

    // uniform point on the unit disk
    inline Vec2 ConcentricDisk(float u1, float u2)
    {
        const float x = 2.0f * u1 - 1.0f;
        const float y = 2.0f * u2 - 1.0f;
        if (x == 0.0f && y == 0.0f)
        {
            return {0.0f, 0.0f};
        }

        float radius;
        float theta;
        if (std::abs(x) > std::abs(y))
        {
            radius = x;
            theta = (Pi / 4.0f) * (y / x);
        }
        else
        {
            radius = y;
            theta = HalfPi - (Pi / 4.0f) * (x / y);
        }
        return {radius * std::cos(theta), radius * std::sin(theta)};
    }

    // direction in a local frame where +Z is the normal, with pdf = cos(theta) / pi
    inline Vec3 CosineHemisphereLocal(float u1, float u2)
    {
        const Vec2 d = ConcentricDisk(u1, u2);
        return {d.x, d.y, std::sqrt(std::max(0.0f, 1.0f - d.x * d.x - d.y * d.y))};
    }

    // builds two vectors perpendicular to a unit vector n
    inline void OrthonormalBasis(const Vec3& n, Vec3& tangent, Vec3& bitangent)
    {
        const float sign = std::copysign(1.0f, n.z);
        const float a = -1.0f / (sign + n.z);
        const float b = n.x * n.y * a;
        tangent = {1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x};
        bitangent = {b, sign + n.y * n.y * a, -n.y};
    }

    // cosine-weighted direction around a world-space unit normal
    inline Vec3 SampleCosineHemisphere(const Vec3& normal, Random& random)
    {
        Vec3 tangent;
        Vec3 bitangent;
        OrthonormalBasis(normal, tangent, bitangent);
        const Vec3 local = CosineHemisphereLocal(random.NextFloat(), random.NextFloat());
        return glm::normalize(local.x * tangent + local.y * bitangent + local.z * normal);
    }

} 