#pragma once

#include <Duck/Core/Ray.h>

#include <cmath>
#include <cstdint>
#include <optional>

namespace Duck::PathTrace
{
    struct Hit
    {
        Vec3 Point{0.0f};
        Vec3 Normal{0.0f};
        float T = 0.0f;
        bool FrontFace = true; // false if hit from the inside of a surface
        int MaterialId = 0;
    };

    enum class MaterialType
    {
        Diffuse,    // labert + sun + ambient
        Mirror,     // perfect reflection, tinted by albedo
        Dielectric, // glass and water, fresnel weighted reflection + refraction
    };

    struct Material
    {
        MaterialType Type = MaterialType::Diffuse;
        Vec3 Albedo{0.5f}; // diffuse color or mirror tint
        float Ior = 1.5f;  // index of refraction, dielectric only
    };

    struct Sphere
    {
        Vec3 Center{0.0f};
        float Radius = 1.0f;
        int MaterialId = 0;

        // only hits with tMin <= t <= tMax count
        std::optional<Hit> Intersect(const Ray& ray, float tMin, float tMax) const
        {
            const Vec3 oc = ray.Origin - Center;
            const float b = glm::dot(oc, ray.Direction);
            const Vec3 perpendicular = oc - b * ray.Direction; // closest approach offset
            const float discriminant = Radius * Radius - glm::dot(perpendicular, perpendicular);
            if (discriminant < 0.0f)
            {
                return std::nullopt;
            }

            const float root = std::sqrt(discriminant);
            float t = -b - root;
            if (t < tMin || t > tMax)
            {
                t = -b + root;
                if (t < tMin || t > tMax)
                {
                    return std::nullopt;
                }
            }

            Hit hit;
            hit.T = t;
            hit.MaterialId = MaterialId;
            hit.Point = ray.At(t);
            const Vec3 outward = (hit.Point - Center) / Radius;
            hit.FrontFace = glm::dot(ray.Direction, outward) < 0.0f;
            hit.Normal = hit.FrontFace ? outward : -outward;
            return hit;
        }
    };
}