#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Core/Ray.h>
#include <Duck/PathTraceCpu/Geometry.h>
#include <Duck/PathTraceCpu/Lights.h>
#include <Duck/PathTraceCpu/Scene.h>

namespace Duck::PathTrace
{
    // rays leaving a surface ignore hits closer than this, so they don't hit the surface they start on
    inline constexpr float RayEpsilon = 0.001f;

    // a ray leaving the surface at hit. the one place to add an offset along the normal later
    inline Ray SpawnRay(const Hit& hit, const Vec3& direction) { return {hit.Point, direction}; }

    inline Vec3 LambertBrdf(const Vec3& albedo) { return albedo * InvPi; }

    // light arriving straight from the lights, one sample per light: sum of f(wi) * Li * cos / pdf.
    // brdf(wi) returns f(wo, wi) for the surface being shaded, so Blinn-Phong etc. plug in later
    template <typename TBrdf>
    Vec3 DirectLighting(const Scene& scene, const Hit& hit, Random& random, TBrdf&& brdf)
    {
        Vec3 total(0.0f);
        for (const Light& light : scene.Lights)
        {
            const LightSample sample = SampleLight(light, hit.Point, random);
            const float cosine = glm::dot(hit.Normal, sample.Direction);
            if (sample.Pdf <= 0.0f || cosine <= 0.0f)
            {
                continue; // nothing sampled, or the light is behind the surface
            }
            if (scene.Occluded(SpawnRay(hit, sample.Direction), RayEpsilon, sample.Distance - RayEpsilon))
            {
                continue;
            }
            total += brdf(sample.Direction) * sample.Radiance * cosine / sample.Pdf;
        }
        return total;
    }
}