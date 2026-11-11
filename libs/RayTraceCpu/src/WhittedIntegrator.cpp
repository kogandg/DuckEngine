#include <Duck/RayTraceCpu/WhittedIntegrator.h>

#include <Duck/Core/Ray.h>
#include <Duck/RayTraceCpu/Bsdf.h>
#include <Duck/RayTraceCpu/Materials.h>
#include <Duck/RayTraceCpu/Optics.h>
#include <Duck/RayTraceCpu/Scene.h>
#include <Duck/RayTraceCpu/Shading.h>

namespace Duck::RayTraceCpu
{
    namespace
    {
        constexpr float MinWeight = 0.001f; // below this a branch cant visibly change the pixel

        float MirrorFade(float roughness) { return (1.0f - roughness) * (1.0f - roughness); }
    }

    Vec3 WhittedIntegrator::Li(const RenderScene& scene, const Ray& ray, Random& random) const
    {
        return Trace(scene, ray, random, 0, 1.0f);
    }

    Vec3 WhittedIntegrator::Trace(const RenderScene& scene, const Ray& ray, Random& random, int depth, float weight) const
    {
        if (depth > maxDepth || weight < MinWeight)
        {
            return Vec3(0.0f);
        }

        const auto hit = scene.ClosestHit(ray, RayEpsilon, Infinity);
        if (!hit)
        {
            return scene.GetEnironment().SkyColor(ray.Direction);
        }

        const Material& material = scene.GetMaterial(hit->MaterialId);

        const Vec3 transmittance = SegmentTransmittance(material, *hit);
        const float carried = weight * MaxComponent(transmittance); // what this branch can still contribute

        const Vec3 wo = -ray.Direction;
        const Bsdf bsdf = GetBsdf(material, *hit, wo);

        Vec3 radiance = EmissionAt(material, *hit);
        if (bsdf.HasNonDeltaLobes())
        {
            radiance += (bsdf.DiffuseReflectance() * scene.GetEnironment().Ambient(hit->Normal)) +
                        DirectLighting(scene, *hit, random, [&](const Vec3& wi) { return bsdf.Evaluate(wo, wi); });
        }

        // perfect reflection and refraction rays
        bsdf.ForEachSpecularPath(wo,
                                 [&](const SpecularPath& path)
                                 {
                                     const Vec3 pathWeight = path.Weight * MirrorFade(path.Roughness);
                                     const float strength = MaxComponent(pathWeight);
                                     if (strength > 0.0f)
                                     {
                                         radiance += pathWeight * Trace(scene, SpawnRay(*hit, path.Direction), random,
                                                                        depth + 1, carried * strength);
                                     }
                                 });

        return transmittance * radiance;
    }
}