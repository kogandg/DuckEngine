#include <Duck/RayTraceCpu/Integrators/WhittedIntegrator.h>

#include <Duck/Core/Ray.h>
#include <Duck/RayTraceCpu/Lights/DirectLighting.h>
#include <Duck/RayTraceCpu/Materials/Bsdf.h>
#include <Duck/RayTraceCpu/Materials/Materials.h>
#include <Duck/RayTraceCpu/Scene/Scene.h>

namespace Duck::RayTraceCpu
{
    namespace
    {
        constexpr float MinWeight = 0.001f; // below this a branch cant visibly change the pixel
    }

    Vec3 WhittedIntegrator::Li(const RenderScene& scene, const Ray& cameraRay, ISampler& sampler) const
    {
        Ray ray = cameraRay;
        Vec3 radiance(0.0f);
        Vec3 throughput(1.0f); // how much of what the current ray sees reaches the pixel

        for (int depth = 0; depth <= maxDepth; depth++)
        {
            const auto hit = scene.ClosestHit(ray, RayEpsilon, Infinity);
            if (!hit)
            {
                radiance += throughput * scene.GetEnvironment().SkyColor(ray.Direction);
                break;
            }

            const Material& material = scene.GetMaterial(hit->MaterialId);
            throughput *= SegmentTransmittance(material, *hit);

            const Vec3 wo = -ray.Direction;
            const Bsdf bsdf = GetBsdf(material, *hit, wo);

            radiance += throughput * EmissionAt(material, *hit);
            if (bsdf.HasNonDeltaLobes())
            {
                radiance += throughput * (bsdf.DiffuseReflectance() * scene.GetEnvironment().Ambient(hit->Normal) +
                                          DirectLighting(scene, *hit, sampler,
                                                         [&](const Vec3& wi) { return bsdf.Evaluate(wo, wi); }));
            }

            // continue along one smooth branch. picking a diffuse lobe ends the path: the light the diffuse part
            // reflects was already added above, so it counts as a branch that carries nothing further
            const auto sample = bsdf.Sample(wo, sampler);
            if (!sample || !sample->IsDelta)
            {
                break;
            }
            throughput *= sample->Value * std::abs(glm::dot(sample->Direction, hit->Normal)) / sample->Pdf;
            if (MaxComponent(throughput) < MinWeight)
            {
                break;
            }
            ray = SpawnRay(*hit, sample->Direction);
        }
        return radiance;
    }
}