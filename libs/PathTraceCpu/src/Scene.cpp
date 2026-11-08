#include <Duck/PathTraceCpu/Scene.h>

namespace Duck::PathTrace
{
    std::optional<Hit> Scene::ClosestHit(const Ray& ray, float tMin, float tMax) const
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

    bool Scene::Occluded(const Ray& ray, float tMin, float tMax) const
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
}