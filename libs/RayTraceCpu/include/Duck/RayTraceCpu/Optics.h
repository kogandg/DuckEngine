#pragma once

#include <Duck/Core/Math.h>

#include <cmath>
#include <optional>
#include <algorithm>

namespace Duck::PathTrace
{
    // mirror d about n. d points toward the surface, the result points away from it
    inline Vec3 Reflect(const Vec3& d, const Vec3& n) { return d - 2.0f * glm::dot(d, n) * n; }

    // bends unit d through a surface whose unit normal n faces d's side. eta = etaFrom / etaTo
    // returns nullopt on total internal reflection
    inline std::optional<Vec3> Refract(const Vec3& d, const Vec3& n, float eta)
    {
        const float cosIncident = -glm::dot(d, n);
        const float k = 1.0f - eta * eta * (1.0f - cosIncident * cosIncident);
        if (k < 0.0f)
        {
            return std::nullopt;
        }
        return eta * d + (eta * cosIncident - std::sqrt(k)) * n;
    }

    // Schlick's approximation. cosTheta must be measured on the side with the lower index
    inline float SchlickFresnel(float cosTheta, float etaFrom, float etaTo)
    {
        float r0 = (etaFrom - etaTo) / (etaFrom + etaTo);
        r0 *= r0;
        const float m = 1.0f - cosTheta;
        return r0 + (1.0f - r0) * (m * m * m * m * m);
    }

    // reflectance at normal incidence of a dielectric with index ior seen from air
    inline float F0FromIor(float ior)
    {
        const float r = (ior - 1.0f) / (ior + 1.0f);
        return r * r;
    }

    // Schlick from the reflectance at normal incidence (F0). colored for metals
    inline float SchlickReflectance(float f0, float cosTheta)
    {
        const float m = 1.0f - std::clamp(cosTheta, 0.0f, 1.0f);
        return f0 + (1.0f - f0) * (m * m * m * m * m);
    }

    inline Vec3 SchlickReflectance(const Vec3& f0, float cosTheta)
    {
        const float m = 1.0f - std::clamp(cosTheta, 0.0f, 1.0f);
        return f0 + (Vec3(1.0f) - f0) * (m * m * m * m * m);
    }
}