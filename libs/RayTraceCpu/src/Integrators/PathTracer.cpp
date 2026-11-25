#include <Duck/RayTraceCpu/Integrators/PathTracer.h>

#include <Duck/Core/Ray.h>
#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Lights/DirectLighting.h>
#include <Duck/RayTraceCpu/Materials/Bsdf.h>
#include <Duck/RayTraceCpu/Materials/Materials.h>
#include <Duck/RayTraceCpu/Scene/Scene.h>

#include <cmath>
#include <algorithm>

namespace Duck::RayTraceCpu
{
    namespace
    {
        constexpr int RouletteStartBounce = 3;

        struct PrevVertex
        {
            bool LightSampled = false; // light sampling also covered this direction there
            float BsdfPdf = 0.0f;      // density the BSDF picked the direction with
            Vec3 Point{0.0f};          // where the bounce happened
        };

        // emission seen at hit, weighted against light sampling if the previous vertex also did NEE
        Spectrum WeightedEmission(const RenderScene& scene, const Material& material, const Hit& hit,
                                  const PrevVertex& prev)
        {
            const Spectrum emitted = EmissionAt(material, hit);
            if (MaxComponent(emitted) <= 0.0f || !prev.LightSampled)
            {
                return emitted;
            }
            const int light = scene.AreaLightAt(hit);
            return light < 0 ? emitted
                             : emitted * PowerHeuristic(prev.BsdfPdf, scene.AreaLightPdf(light, hit, prev.Point));
        }

        // true if the path continues. on survival throughput is scaled up so the average stays the same
        bool SurviveRoulette(Spectrum& throughput, float etaScale, ISampler& sampler)
        {
            const float survive = std::min(MaxComponent(throughput) * etaScale, 1.0f);
            if (sampler.Get1D() >= survive)
            {
                return false;
            }
            throughput /= survive;
            return true;
        }
    }

    Spectrum PathTracer::Li(const RenderScene& scene, const Ray& cameraRay, ISampler& sampler) const
    {
        Ray ray = cameraRay;
        Spectrum radiance(0.0f);
        Spectrum throughput(1.0f);
        float etaScale = 1.0f;

        PrevVertex prev;

        for (int depth = 0; ; depth++)
        {
            // finish paths of length depth: whatever this ray sees
            const auto hit = scene.ClosestHit(ray, RayEpsilon, Infinity);
            if (!hit)
            {
                radiance += throughput * scene.GetEnvironment().SkyColor(ray.Direction);
                break;
            }
            const Material& material = scene.GetMaterial(hit->MaterialId);
            throughput *= SegmentTransmittance(material, *hit);
            radiance += throughput * WeightedEmission(scene, material, *hit, prev);

            // builds paths of length depth + 1
            if (depth == maxDepth)
            {
                break;
            }

            const Vec3 wo = -ray.Direction;
            const Bsdf bsdf = GetBsdf(material, *hit, sampler);
            const bool sampleLights = nee && IsNonSpecular(bsdf.Flags());

            if (sampleLights)
            {
                radiance +=
                    throughput * DirectLighting(
                                     scene, *hit, sampler, [&](const Vec3& wi) { return bsdf.Evaluate(wo, wi); },
                                     [&](const Vec3& wi) { return bsdf.Pdf(wo, wi); });
            }

            const float uc = sampler.Get1D();
            const auto sample = bsdf.Sample(wo, uc, sampler.Get2D());
            if (!sample)
            {
                break;
            }
            throughput *= sample->F * std::abs(glm::dot(sample->Wi, hit->Normal)) / sample->Pdf;
            if (sample->IsTransmission())
            {
                etaScale *= sample->Eta * sample->Eta;
            }
            prev = {.LightSampled = sampleLights && !sample->IsSpecular(), .BsdfPdf = sample->Pdf, .Point = hit->Point};

            if (depth >= RouletteStartBounce && !SurviveRoulette(throughput, etaScale, sampler))
            {
                break;
            }
            ray = SpawnRay(*hit, sample->Wi);
        }
        return radiance;
    }
}