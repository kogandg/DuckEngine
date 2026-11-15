#pragma once

#include <Duck/RayTraceCpu/Integrator.h>

namespace Duck::RayTraceCpu
{
    class PathTracer final : public PixelIntegrator
    {
    public:
        explicit PathTracer(int maxBounceCount = 64, bool nextEventEstimation = true) : maxBounces(maxBounceCount), nee(nextEventEstimation) {}

        const char* Name() const override { return "Path Tracer"; }

        int RecommendedPasses() const override { return 1024; }
        AdaptiveSettings GetAdaptiveSettings() const override { return {.MinPasses = 256, .Threshold = 0.01f}; }
    protected:
        Vec3 Li(const RenderScene& scene, const Ray& cameraRay, ISampler& sampler) const override;

    private:
        int maxBounces;
        bool nee;
    };
}