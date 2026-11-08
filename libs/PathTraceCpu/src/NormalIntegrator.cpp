#include <Duck/PathTraceCpu/NormalIntegrator.h>

#include <Duck/PathTraceCpu/Scene.h>

namespace Duck::PathTrace
{
    Vec3 NormalIntegrator::Li(const Scene& scene, const Ray& ray, Random&) const
    {
        if (const auto hit = scene.ClosestHit(ray, 0.001f, Infinity))
        {
            return 0.5f * (hit->Normal + Vec3(1.0f));
        }
        return Vec3(0.0f);
    }
}