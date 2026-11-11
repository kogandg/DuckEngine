#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Core/Ray.h>
#include <Duck/PathTraceCpu/Geometry.h>
#include <Duck/PathTraceCpu/Lights.h>
#include <Duck/PathTraceCpu/Scene.h>

namespace Duck::PathTrace
{
    // rays leaving a surface ignore hits closer than this
    inline constexpr float RayEpsilon = 1e-6f;

    // how far SpawnRay pushes an origin off it surface
    inline constexpr float RayOffset = 1e-4f;

    // a ray leaving the surface at hit. the one place to add an offset along the normal later
    inline Ray SpawnRay(const Hit& hit, const Vec3& direction)
    {
        const float side = glm::dot(direction, hit.Normal) > 0.0f ? 1.0f : -1.0f;
        
        return {hit.Point + side * RayOffset * hit.Normal, direction};
    }

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

    // light coming back along the ray from hit. if the ray hit this surface from the inside, it crossed the
    // objects interior to get there and was partly absorbed on the way. (no nested media)
    inline Vec3 SegmentTransmittance(const Material& material, const Hit& hit)
    {
        return hit.FrontFace ? Vec3(1.0f) : InteriorTransmittance(material, hit.T);
    }
}