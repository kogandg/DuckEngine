#include <Duck/RayTraceCpu/PathTracer.h>

#include <Duck/Core/Ray.h>
#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Bsdf.h>
#include <Duck/RayTraceCpu/Materials.h>
#include <Duck/RayTraceCpu/Scene.h>
#include <Duck/RayTraceCpu/Shading.h>

#include <cmath>

namespace
{
    constexpr int RouletteStartBounce = 3;
}

namespace Duck::RayTraceCpu
{
    Vec3 PathTracer::Li(const RenderScene& scene, const Ray& cameraRay, ISampler& sampler) const
    {
        Ray ray = cameraRay;
        Vec3 radiance(0.0f);
        Vec3 throughput(1.0f);

        // about the bounce of the current ray
        bool lightSampled = false; // whether light sampling also covered this direction at the last hit
        float bsdfPdf = 0.0f;      // density the bounce picked its direciton with
        Vec3 from(0.0f);           // where the bounce happened

        for (int bounce = 0; bounce < this->maxBounces; bounce++)
        {
            const auto hit = scene.ClosestHit(ray, RayEpsilon, Infinity);
            if (!hit)
            {
                radiance += throughput * scene.GetEnvironment().SkyColor(ray.Direction);
                break;
            }

            const Material& material = scene.GetMaterial(hit->MaterialId);
            throughput *= SegmentTransmittance(material, *hit);

            const Vec3 emitted = EmissionAt(material, *hit);
            if (MaxComponent(emitted) > 0.0f)
            {
                float weight = 1.0f;
                if (lightSampled)
                {
                    const int light = scene.AreaLightAt(*hit);
                    if (light >= 0)
                    {
                        weight = PowerHeuristic(bsdfPdf, scene.AreaLightPdf(light, *hit, from));
                    }
                }
                radiance += throughput * emitted * weight;
            }

            const Vec3 wo = -ray.Direction;
            const Bsdf bsdf = GetBsdf(material, *hit, wo);

            const bool sampleLights = nee && bsdf.HasNonDeltaLobes();
            if (sampleLights)
            {
                radiance += throughput * DirectLighting(
                                             scene, *hit, sampler, [&](const Vec3& wi) { return bsdf.Evaluate(wo, wi); },
                                     [&](const Vec3& wi) { return bsdf.Pdf(wo, wi); });
            }

            const auto sample = bsdf.Sample(wo, sampler);
            if (!sample)
            {
                break; // absorbed
            }

            throughput *= sample->Value * std::abs(glm::dot(sample->Direction, hit->Normal)) / sample->Pdf;

            lightSampled = sampleLights && !sample->IsDelta;
            bsdfPdf = sample->Pdf;
            from = hit->Point;

            if (bounce >= RouletteStartBounce)
            {
                const float survive = std::min(MaxComponent(throughput), 1.0f);
                if (sampler.Get1D() >= survive)
                {
                    break;
                }
                throughput /= survive;
            }

            ray = SpawnRay(*hit, sample->Direction);
        }
        return radiance;
    }
}