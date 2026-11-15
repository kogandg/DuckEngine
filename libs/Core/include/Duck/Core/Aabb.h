#pragma once

#include <Duck/Core/Assert.h>
#include <Duck/Core/Math.h>
#include <Duck/Core/Ray.h>

#include <algorithm>

namespace Duck
{
    // a ray prepared for traversing boxes: everything worth computing once per ray rather than once per box
    // 1 / direction: a 0 component becomes +-infinity, which handles rays parallel to a slab without a special case
    struct TraversalRay
    {
        explicit TraversalRay(const Ray& ray) : Origin(ray.Origin), InvDirection(1.0f / ray.Direction) {}

        Vec3 Origin;
        Vec3 InvDirection;
    };

    struct Aabb
    {
        Vec3 Min{Infinity};
        Vec3 Max{-Infinity};

        void Grow(const Vec3& point)
        {
            Min = glm::min(Min, point);
            Max = glm::max(Max, point);
        }
        void Grow(const Aabb& other)
        {
            Min = glm::min(Min, other.Min);
            Max = glm::max(Max, other.Max);
        }
        bool IsEmpty() const { return glm::any(glm::greaterThan(Min, Max)); }

        float SurfaceArea() const
        {
            DUCK_ASSERT(!IsEmpty(), "Empty box has no surface area");
            const Vec3 size = Max - Min;
            return 2.0f * (size.x * size.y + size.y * size.z + size.z * size.x);
        }

        bool Contains(const Vec3& point, float tolerance = 0.0f) const
        {
            return glm::all(glm::greaterThanEqual(point, Min - tolerance)) &&
                   glm::all(glm::lessThanEqual(point, Max + tolerance));
        }

        // slab test. the t where the ray enters the box (tMin if it starts inside), or Infinity on a miss
        float EntryDistance(const TraversalRay& ray, float tMin, float tMax) const
        {
            DUCK_ASSERT(!IsEmpty(), "Empty box: its infinite corners would make every ray hit it");

            const Vec3 t0 = (Min - ray.Origin) * ray.InvDirection;
            const Vec3 t1 = (Max - ray.Origin) * ray.InvDirection;
            const Vec3 tNear = glm::min(t0, t1);
            const Vec3 tFar = glm::max(t0, t1);

            const float enter = std::max(std::max(tNear.x, tNear.y), std::max(tNear.z, tMin));
            const float exit = std::min(std::min(tFar.x, tFar.y), std::min(tFar.z, tMax));
            return enter <= exit ? enter : Infinity;
        }
    };
}