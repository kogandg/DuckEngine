#include <Duck/RayTraceCpu/Integrators/PathTracer.h>

#include <Duck/Core/Ray.h>
#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Lights/DirectLighting.h>
#include <Duck/RayTraceCpu/Materials/Bsdf.h>
#include <Duck/RayTraceCpu/Materials/Materials.h>
#include <Duck/RayTraceCpu/Scene/Scene.h>

#include <cmath>

namespace
{
    constexpr int RouletteStartBounce = 3;
}

namespace Duck::RayTraceCpu
{
    Spectrum PathTracer::Li(const RenderScene& scene, const Ray& cameraRay, ISampler& sampler) const
    {
        Ray ray = cameraRay;
        Spectrum radiance(0.0f);
        Spectrum throughput(1.0f);

        // about the bounce of the current ray
        bool lightSampled = false; // whether light sampling also covered this direction at the last hit
        float bsdfPdf = 0.0f;      // density the bounce picked its direciton with
        Vec3 from(0.0f);           // where the bounce happened
        float etaScale = 1.0f;     // the eta^2 refraction

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

            const Spectrum emitted = EmissionAt(material, *hit);
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
            const Vec2 u = sampler.Get2D();
            const auto sample = bsdf.Sample(wo, uc, u);
            if (!sample)
            {
                break; // absorbed
            }

            throughput *= sample->F * std::abs(glm::dot(sample->Wi, hit->Normal)) / sample->Pdf;
            if (sample->IsTransmission())
            {
                etaScale *= sample->Eta * sample->Eta;
            }

            lightSampled = sampleLights && !sample->IsSpecular();
            bsdfPdf = sample->Pdf;
            from = hit->Point;

            if (bounce >= RouletteStartBounce)
            {
                const float survive = std::min(MaxComponent(throughput) * etaScale, 1.0f);
                if (sampler.Get1D() >= survive)
                {
                    break;
                }
                throughput /= survive;
            }

            ray = SpawnRay(*hit, sample->Wi);
        }
        return radiance;
    }
}