#pragma once

#include <Duck/RayTraceCpu/Integrators/Integrator.h>

namespace Duck::RayTraceCpu
{
    class PathTracer final : public PixelIntegrator
    {
    public:
        explicit PathTracer(int maxDepthCount = 64, bool nextEventEstimation = true) : maxDepth(maxDepthCount), nee(nextEventEstimation) {}

        const char* Name() const override { return "Path Tracer"; }

        int RecommendedPasses() const override { return 1024; }
        AdaptiveSettings GetAdaptiveSettings() const override { return {.MinPasses = 256, .Threshold = 0.01f}; }
    protected:
        Spectrum Li(const RenderScene& scene, const Ray& cameraRay, ISampler& sampler) const override;

    private:
        int maxDepth;
        bool nee;
    };
}