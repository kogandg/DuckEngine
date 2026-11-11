#include <Duck/RayTraceCpu/PathTracer.h>

#include <Duck/Core/Random.h>
#include <Duck/Core/Ray.h>
#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Scene.h>
#include <Duck/RayTraceCpu/Shading.h>
#include <Duck/RayTraceCpu/Bsdf.h>
#include <Duck/RayTraceCpu/Materials.h>

#include <cmath>

namespace Duck::RayTraceCpu
{
    Vec3 PathTracer::Li(const Scene& scene, const Ray& cameraRay, Random& random) const
    {
        Ray ray = cameraRay;
        Vec3 radiance(0.0f);
        Vec3 throughput(1.0f);

        for (int bounce = 0; bounce < this->maxBounces; ++bounce)
        {
            const auto hit = scene.ClosestHit(ray, RayEpsilon, Infinity);
            if (!hit)
            {
                radiance += throughput * scene.SkyColor(ray);
                break;
            }

            const Material& material = scene.Materials[hit->MaterialId];
            throughput *= SegmentTransmittance(material, *hit);
            radiance += throughput * EmissionAt(material, *hit);

            const Vec3 wo = -ray.Direction;
            const Bsdf bsdf = GetBsdf(material, *hit, wo);
            const auto sample = bsdf.Sample(wo, random);
            if (!sample)
            {
                break; //absorbed
            }

            throughput *= sample->Value * std::abs(glm::dot(sample->Direction, hit->Normal)) / sample->Pdf;
            ray = SpawnRay(*hit, sample->Direction);
        }
        return radiance;
    }
}