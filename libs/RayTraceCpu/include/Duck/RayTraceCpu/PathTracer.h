#pragma once

#include <Duck/RayTraceCpu/Integrator.h>

namespace Duck::RayTraceCpu
{
    class PathTracer final : public PixelIntegrator
    {
    public:
        explicit PathTracer(int maxBounceCount = 8) : maxBounces(maxBounceCount) {}

        const char* Name() const override { return "Path Tracer"; }

    protected:
        Vec3 Li(const RenderScene& scene, const Ray& cameraRay, ISampler& sampler) const override;

    private:
        int maxBounces;
    };
}