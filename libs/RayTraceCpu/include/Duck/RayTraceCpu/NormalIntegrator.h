#pragma once

#include <Duck/RayTraceCpu/Integrator.h>

namespace Duck::RayTraceCpu
{
    class NormalIntegrator final : public PixelIntegrator
    {
    public:
        const char* Name() const override { return "Normals"; }
        int RecommendedPasses() const override { return 1; }
        OutputTransform GetOutputTransform() const override { return OutputTransform::Linear; }

    protected:
        bool JitterSamples() const override { return false; }

        Vec3 Li(const Scene& scene, const Ray& ray, Random&) const override;
    };
}