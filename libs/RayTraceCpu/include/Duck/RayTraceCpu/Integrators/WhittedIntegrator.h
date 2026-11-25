#pragma once

#include <Duck/RayTraceCpu/Integrators/Integrator.h>

namespace Duck::RayTraceCpu
{
    class WhittedIntegrator final : public PixelIntegrator
    {
    public:
        explicit WhittedIntegrator(int maxDepthCount = 10) : maxDepth(maxDepthCount) {}

        const char* Name() const override { return "Whitted"; }
        int RecommendedPasses() const override { return 256; }
        AdaptiveSettings GetAdaptiveSettings() const override { return {.MinPasses = 32, .Threshold = 0.01f}; }

    protected:
        Spectrum Li(const RenderScene& scene, const Ray& ray, ISampler& sampler) const override;

    private:
        int maxDepth;
    };
}