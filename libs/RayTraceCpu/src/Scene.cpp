#include <Duck/RayTraceCpu/Scene.h>

#include <algorithm>

namespace Duck::RayTraceCpu
{
    namespace
    {
        template <typename TShape>
        void FindClosest(const std::vector<TShape>& shapes, const Ray& ray, float tMin, float& tMax,
                         std::optional<Hit>& closest)
        {
            for (const TShape& shape : shapes)
            {
                if (const auto hit = shape.Intersect(ray, tMin, tMax))
                {
                    closest = hit;
                    tMax = hit->T; // anything further away can't be the closest now
                }
            }
        }

        template <typename TShape>
        bool AnyHit(const std::vector<TShape>& shapes, const Ray& ray, float tMin, float tMax)
        {
            return std::ranges::any_of(shapes, [&](const TShape& shape)
                                       { return shape.Intersect(ray, tMin, tMax).has_value(); });
        }
    }

    std::optional<Hit> Scene::ClosestHit(const Ray& ray, float tMin, float tMax) const
    {
        std::optional<Hit> closest;
        FindClosest(Spheres, ray, tMin, tMax, closest);
        FindClosest(Quads, ray, tMin, tMax, closest);
        return closest;
    }

    bool Scene::Occluded(const Ray& ray, float tMin, float tMax) const
    {
        return AnyHit(Spheres, ray, tMin, tMax) || AnyHit(Quads, ray, tMin, tMax);
    }
}