#include <Duck/PathTraceCpu/WhittedIntegrator.h>

#include <Duck/Core/Ray.h>
#include <Duck/PathTraceCpu/Scene.h>

namespace Duck::PathTrace
{
    Vec3 WhittedIntegrator::Li(const Scene& scene, const Ray& ray, Random& random) const
    {
        const auto hit = scene.ClosestHit(ray, 0.001f, Infinity);
        if (!hit)
        {
            return scene.SkyColor(ray);
        }

        const Vec3 albedo = scene.Materials[hit->MaterialId].Albedo;

        const Vec3 ambient = 0.3f * scene.SkyColor({hit->Point, hit->Normal});

        Vec3 direct(0.0f);
        const float cosine = glm::dot(hit->Normal, scene.SunDirection);
        if (cosine > 0.0f && !scene.Occluded({hit->Point, scene.SunDirection}, 0.001f, Infinity))
        {
            direct = scene.SunStrength * cosine;
        }

        return albedo * (ambient + direct);
    }
}