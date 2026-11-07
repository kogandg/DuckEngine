#pragma once

#include <Duck/Core/Ray.h>

#include <cmath>
#include <optional>

namespace Duck
{
    struct Hit
    {
        Vec3 Point{0.0f};
        Vec3 Normal{0.0f};
        float T = 0.0f;
        bool FrontFace = true; // false if hit from the inside of a surface
    };

    struct Sphere
    {
        Vec3 Center{0.0f};
        float Radius = 1.0f;

        // only hits with tMin <= t <= tMax count
        std::optional<Hit> Intersect(const Ray& ray, float tMin, float tMax) const
        {
            const Vec3 oc = ray.Origin - Center;
            const float halfB = glm::dot(oc, ray.Direction);
            const float c = glm::dot(oc, oc) - Radius * Radius;

            const float discriminant = halfB * halfB - c;
            if (discriminant < 0.0f)
            {
                return std::nullopt;
            }

            const float root = std::sqrt(discriminant);
            float t = -halfB - root;
            if (t < tMin || t > tMax)
            {
                t = -halfB + root;
                if (t < tMin || t > tMax)
                {
                    return std::nullopt;
                }
            }

            Hit hit;
            hit.T = t;
            hit.Point = ray.At(t);
            const Vec3 outward = (hit.Point - Center) / Radius;
            hit.FrontFace = glm::dot(ray.Direction, outward) < 0.0f;
            hit.Normal = hit.FrontFace ? outward : -outward;
            return hit;
        }
    };
}