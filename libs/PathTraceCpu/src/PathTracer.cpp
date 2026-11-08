#include <Duck/PathTraceCpu/PathTracer.h>

#include <Duck/Core/Random.h>
#include <Duck/Core/Ray.h>
#include <Duck/Core/Sampling.h>
#include <Duck/PathTraceCpu//Scene.h>

namespace Duck::PathTrace
{
    Vec3 PathTracer::Li(const Scene& scene, const Ray& cameraRay, Random& random) const
    {
        Ray ray = cameraRay;
        Vec3 radiance(0.0f);
        Vec3 throughput(1.0f);

        for (int bounce = 0; bounce < this->maxBounces; ++bounce)
        {
            const auto hit = scene.ClosestHit(ray, 0.001f, Infinity);
            if (!hit)
            {
                radiance += throughput * scene.SkyColor(ray);
                break;
            }

            // lambertian diffuse with cosine sampling: f * cos / pdf = albedo
            throughput *= scene.Materials[hit->MaterialId].Albedo;
            ray = {hit->Point, SampleCosineHemisphere(hit->Normal, random)};
        }
        return radiance;
    }
}