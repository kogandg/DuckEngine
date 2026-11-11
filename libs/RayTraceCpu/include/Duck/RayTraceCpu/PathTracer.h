#pragma once

#include <Duck/PathTraceCpu/Integrator.h>

namespace Duck::PathTrace
{
    class PathTracer final : public PixelIntegrator
    {
    public:
        explicit PathTracer(int maxBounceCount = 8) : maxBounces(maxBounceCount) {}

        const char* Name() const override { return "Path Tracer"; }

    protected:
        Vec3 Li(const Scene& scene, const Ray& cameraRay, Random& random) const override;

    private:
        int maxBounces;
    };
}