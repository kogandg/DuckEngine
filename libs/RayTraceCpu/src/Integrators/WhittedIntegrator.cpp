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
        float etaScale = 1.0f;

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
            const Bsdf bsdf = GetBsdf(material, *hit, sampler);

            radiance += throughput * EmissionAt(material, *hit);
            if (IsNonSpecular(bsdf.Flags()))
            {
                radiance += throughput * (bsdf.DiffuseAlbedo(wo) * scene.GetEnvironment().Ambient(hit->Normal) +
                                          DirectLighting(scene, *hit, sampler,
                                                         [&](const Vec3& wi) { return bsdf.Evaluate(wo, wi); }));
            }

            // continue along one smooth branch. picking a diffuse part ends the path: the light it reflects was
            // already added above, so it counts as a branch that carries nothing further
            const float uc = sampler.Get1D();
            const Vec2 u = sampler.Get2D();
            const auto sample = bsdf.Sample(wo, uc, u);
            if (!sample || !sample->IsSpecular())
            {
                break;
            }
            throughput *= sample->F * std::abs(glm::dot(sample->Wi, hit->Normal)) / sample->Pdf;
            if (sample->IsTransmission())
            {
                etaScale *= sample->Eta * sample->Eta;
            }
            if (MaxComponent(throughput) * etaScale < MinWeight)
            {
                break;
            }
            ray = SpawnRay(*hit, sample->Wi);
        }
        return radiance;
    }
}