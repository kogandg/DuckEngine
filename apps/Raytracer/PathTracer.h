#pragma once

#include "Integrator.h"

#include <Duck/Core/Sampling.h>

namespace Duck
{
    class PathTracer final : public PixelIntegrator
    {
    public:
        explicit PathTracer(int maxBounceCount = 8) : maxBounces(maxBounceCount) {}

        const char* Name() const override { return "Path Tracer"; }

    protected:
        Vec3 Li(const Scene& scene, const Ray& cameraRay, Random& random) const override
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

    private:
        int maxBounces;
    };
}