#pragma once

#include "Integrator.h"

namespace Duck
{
    class NormalIntegrator final : public PixelIntegrator
    {
    public:
        const char* Name() const override { return "Normals"; }
        int RecommendedPasses() const override { return 1; }
        OutputTransform GetOutputTransform() const override { return OutputTransform::Linear; }

    protected:
        bool JitterSamples() const override { return false; }

        Vec3 Li(const Scene& scene, const Ray& ray, Random&) const override
        {
            if (const auto hit = scene.ClosestHit(ray, 0.001f, Infinity))
            {
                return 0.5f * (hit->Normal + Vec3(1.0f));
            }
            return Vec3(0.0f);
        }
    };
}