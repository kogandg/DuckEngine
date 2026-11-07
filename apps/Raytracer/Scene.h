#pragma once
#include "Geometry.h"

#include <Duck/Core/Ray.h>

#include <optional>
#include <vector>

namespace Duck
{
    struct Scene
    {
        std::vector<Material> Materials;
        std::vector<Sphere> Spheres;

        //directional light, points TOWARD the light
        Vec3 SunDirection = glm::normalize(Vec3(0.6f, 1.0f, 0.4f));
        Vec3 SunStrength{0.7f};

        std::optional<Hit> ClosestHit(const Ray& ray, float tMin, float tMax) const
        {
            std::optional<Hit> closest;
            for (const Sphere& sphere : Spheres)
            {
                if (const auto hit = sphere.Intersect(ray, tMin, tMax))
                {
                    closest = hit;
                    tMax = hit->T;
                }
            }
            return closest;
        }

        // any hit in the interval counts
        bool Occluded(const Ray& ray, float tMin, float tMax) const
        {
            for (const Sphere& sphere : this->Spheres)
            {
                if (sphere.Intersect(ray, tMin, tMax))
                {
                    return true;
                }
            }
            return false;
        }

        Vec3 SkyColor(const Ray& ray) const
        {
            const float a = 0.5f * (ray.Direction.y + 1.0f);
            return glm::mix(Vec3(1.0f), Vec3(0.5f, 0.7f, 1.0f), a);
        }
    };
}