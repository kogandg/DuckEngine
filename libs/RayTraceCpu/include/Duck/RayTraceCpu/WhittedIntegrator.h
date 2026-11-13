#pragma once

#include <Duck/RayTraceCpu/Integrator.h>

namespace Duck::RayTraceCpu
{
    struct Hit;

    class WhittedIntegrator final : public PixelIntegrator
    {
    public:
        explicit WhittedIntegrator(int maxDepthCount = 10) : maxDepth(maxDepthCount) {}

        const char* Name() const override { return "Whitted"; }
        int RecommendedPasses() const override { return 64; }

    protected:
        Vec3 Li(const RenderScene& scene, const Ray& ray, ISampler& sampler) const override;

    private:
        // weight = how much this ray can still contribute to the pixel, used to cut off useless branches
        Vec3 Trace(const RenderScene& scene, const Ray& ray, ISampler& sampler, int depth, float weight) const;

        int maxDepth;
    };
}