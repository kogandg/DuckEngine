#pragma once

#include <Duck/PathTraceCpu/Integrator.h>

namespace Duck::PathTrace
{
    class WhittedIntegrator final : public PixelIntegrator
    {
    public:
        const char* Name() const override { return "Whitted"; }

        int RecommendedPasses() const override { return 32; }

    protected:
        Vec3 Li(const Scene& scene, const Ray& ray, Random& random) const override;
    };
}