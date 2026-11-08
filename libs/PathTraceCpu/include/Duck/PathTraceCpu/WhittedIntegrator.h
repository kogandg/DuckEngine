#pragma once

#include <Duck/PathTraceCpu/Integrator.h>

namespace Duck::PathTrace
{
    struct Hit;
    struct Material;

    class WhittedIntegrator final : public PixelIntegrator
    {
    public:
        explicit WhittedIntegrator(int maxDepthCount = 6) : maxDepth(maxDepthCount) {}

        const char* Name() const override { return "Whitted"; }
        int RecommendedPasses() const override { return 32; }

    protected:
        Vec3 Li(const Scene& scene, const Ray& ray, Random& random) const override;

    private:
        // weight = how much this ray can still contribute to the pixel, used to cut off useless branches
        Vec3 Trace(const Scene& scene, const Ray& ray, int depth, float weight) const;
        Vec3 TraceDielectric(const Scene& scene, const Ray& ray, const Hit& hit, const Material& material, int depth,
                             float weight) const;

        int maxDepth;
    };
}