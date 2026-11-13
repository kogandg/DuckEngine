#include <Duck/RayTraceCpu/NormalIntegrator.h>

#include <Duck/RayTraceCpu/Scene.h>
#include <Duck/RayTraceCpu/Shading.h>

namespace Duck::RayTraceCpu
{
    Vec3 NormalIntegrator::Li(const RenderScene& scene, const Ray& ray, ISampler&) const
    {
        if (const auto hit = scene.ClosestHit(ray, RayEpsilon, Infinity))
        {
            // map the normal components from [-1, 1] to [0, 1] so they can be shown as a color
            return 0.5f * (hit->Normal + Vec3(1.0f));
        }
        return Vec3(0.0f);
    }
}