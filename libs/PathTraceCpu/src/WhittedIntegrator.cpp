#include <Duck/PathTraceCpu/WhittedIntegrator.h>

#include <Duck/Core/Ray.h>
#include <Duck/PathTraceCpu/Optics.h>
#include <Duck/PathTraceCpu/Scene.h>
#include <Duck/PathTraceCpu/Shading.h>

#include <algorithm>
#include <cmath>

namespace Duck::PathTrace
{
    namespace
    {
        constexpr float MinWeight = 0.001f; // below this a branch cant visibly change the pixel

        // lambert: BRDF = albedo / pi, so reflected radiance = albedo / pi * irradiance
        Vec3 ShadeDiffuse(const Scene& scene, const Hit& hit, const Vec3& albedo, Random& random)
        {
            // ambient is radiance from all around, uniform radiance L over hemisphere gives E = pi * L
            // and albedo / pi * pi * L = albedo * L, so no pi needed
            const Vec3 ambient = albedo * scene.Ambient(hit.Normal);

            const Vec3 brdf = LambertBrdf(albedo);

            return ambient + DirectLighting(scene, hit, random, [&](const Vec3&) { return brdf; });
        }
    }

    Vec3 WhittedIntegrator::Li(const Scene& scene, const Ray& ray, Random& random) const
    {
        return Trace(scene, ray, random, 0, 1.0f);
    }

    Vec3 WhittedIntegrator::Trace(const Scene& scene, const Ray& ray, Random& random, int depth, float weight) const
    {
        if (depth > maxDepth || weight < MinWeight)
        {
            return Vec3(0.0f);
        }

        const auto hit = scene.ClosestHit(ray, RayEpsilon, Infinity);
        if (!hit)
        {
            return scene.SkyColor(ray);
        }

        const Material& material = scene.Materials[hit->MaterialId];
        switch (material.Type)
        {
        case MaterialType::Diffuse:
            return ShadeDiffuse(scene, *hit, material.AlbedoAt(hit->Point), random);

        case MaterialType::Mirror:
        {
            const Vec3 reflected = Reflect(ray.Direction, hit->Normal);
            const float tint = std::max({material.Albedo.x, material.Albedo.y, material.Albedo.z});
            return material.Albedo * Trace(scene, SpawnRay(*hit, reflected), random, depth + 1, weight * tint);
        }

        case MaterialType::Dielectric:
            return TraceDielectric(scene, ray, *hit, material, random, depth, weight);
        }

        return Vec3(0.0f);
    }

    Vec3 WhittedIntegrator::TraceDielectric(const Scene& scene, const Ray& ray, const Hit& hit,
                                            const Material& material, Random& random, int depth, float weight) const
    {
        // hit.normal always faces the incoming ray, frontface says either entering or leaving
        const float etaFrom = hit.FrontFace ? 1.0f : material.Ior;
        const float etaTo = hit.FrontFace ? material.Ior : 1.0f;

        const Vec3 reflected = Reflect(ray.Direction, hit.Normal);
        const auto refracted = Refract(ray.Direction, hit.Normal, etaFrom / etaTo);
        if (!refracted)
        {
            // total internal reflection, it all reflects
            return Trace(scene, SpawnRay(hit, reflected), random, depth + 1, weight);
        }

        // schlick has angle on lower index (air) side, incoming when entering, refracted when leaving
        const float cosine = hit.FrontFace ? -glm::dot(ray.Direction, hit.Normal) : -glm::dot(*refracted, hit.Normal);
        const float reflectance = SchlickFresnel(cosine, etaFrom, etaTo);

        const Vec3 reflectedLight = Trace(scene, SpawnRay(hit, reflected), random, depth + 1, weight * reflectance);
        const Vec3 refractedLight =
            Trace(scene, SpawnRay(hit, *refracted), random, depth + 1, weight * (1.0f - reflectance));

        return (reflectance * reflectedLight) + ((1.0f - reflectance) * refractedLight);
    }
}