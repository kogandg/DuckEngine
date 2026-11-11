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
        int RecommendedPasses() const override { return 32; }

    protected:
        Vec3 Li(const Scene& scene, const Ray& ray, Random& random) const override;

    private:
        // weight = how much this ray can still contribute to the pixel, used to cut off useless branches
        Vec3 Trace(const Scene& scene, const Ray& ray, Random& random, int depth, float weight) const;

        int maxDepth;
    };
}