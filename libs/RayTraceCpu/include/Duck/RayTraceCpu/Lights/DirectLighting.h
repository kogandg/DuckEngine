#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Lights/Lights.h>
#include <Duck/RayTraceCpu/Scene/Hit.h>
#include <Duck/RayTraceCpu/Scene/RenderScene.h>

#include <cmath>

namespace Duck::RayTraceCpu
{
    // f(wi) * Li * cos / pdf for one light sample, or 0 if the light is behind the surface or something blocks it
    template <typename TBrdf>
    Vec3 ShadowedContribution(const RenderScene& scene, const Hit& hit, const LightSample& sample, TBrdf&& brdf)
    {
        const float cosine = glm::dot(hit.Normal, sample.Direction);
        if (sample.Pdf <= 0.0f || cosine <= 0.0f)
        {
            return Vec3(0.0f); // nothing sampled, or the light is behind the surface
        }

        // stop short of the light so area lights dont shadow themselves
        Ray shadow = SpawnRay(hit, sample.Direction);
        float shadowEnd = Infinity;
        if (!std::isinf(sample.Distance))
        {
            const Vec3 target = hit.Point + sample.Direction * sample.Distance;
            const Vec3 toTarget = target - shadow.Origin;
            const float length = glm::length(toTarget);
            shadow.Direction = toTarget / length;
            shadowEnd = length - RayOffset; // stop just short of the light's own surface
        }
        if (scene.Occluded(shadow, RayEpsilon, shadowEnd))
        {
            return Vec3(0.0f);
        }
        return brdf(sample.Direction) * sample.Radiance * cosine / sample.Pdf;
    }

    // light arriving straight from the lights. brdf(wi) returns f(wo, wi) for the surface being shaded
    // every listed light (sun, point lights) is sampled: there are few, and each is one cheap shadow ray. of the area
    // lights only one is, picked at random: dividing by the chance of picking it keeps the average equal to the sum
    // over all of them, so the cost stays one shadow ray however many emissive objects there are
    template <typename TBrdf, typename TBsdfPdf>
    Vec3 DirectLighting(const RenderScene& scene, const Hit& hit, ISampler& sampler, TBrdf&& brdf, TBsdfPdf&& bsdfPdf)
    {
        Vec3 total(0.0f);
        for (const Light& light : scene.GetLights())
        {
            total += ShadowedContribution(scene, hit, SampleLight(light, hit.Point, sampler), brdf);
        }

        if (const auto choice = scene.ChooseAreaLight(sampler.Get1D()))
        {
            const LightSample sample = scene.SampleAreaLight(choice->Index, hit.Point, sampler);
            if (sample.Pdf > 0.0f)
            {
                const float weight = PowerHeuristic(sample.Pdf * choice->Probability, bsdfPdf(sample.Direction));
                total += ShadowedContribution(scene, hit, sample, brdf) * (weight / choice->Probability);
            }
        }
        return total;
    }

    // light sampling on its own (Whitted): nothing else can find the lights, so every sample has full weight
    template <typename TBrdf>
    Vec3 DirectLighting(const RenderScene& scene, const Hit& hit, ISampler& sampler, TBrdf&& brdf)
    {
        return DirectLighting(scene, hit, sampler, brdf, [](const Vec3&) { return 0.0f; });
    }
}