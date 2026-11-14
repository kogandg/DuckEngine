#pragma once

#include <Duck/RayTraceCpu/Integrator.h>

namespace Duck::RayTraceCpu
{
    // debug view: how many intersection tests each pixels camera ray needed, as a heatmap (dark blue = few,
    // red = many) on a log scale up to MaxTests. the scale is fixed
    class CostIntegrator final : public PixelIntegrator
    {
    public:
        enum class Count
        {
            Total,
            BoxTests,
            PrimitiveTests,
        };

        explicit CostIntegrator(Count countWhat = Count::Total, int maxTestCount = 2048)
            : what(countWhat), maxTests(maxTestCount)
        {
        }

        const char* Name() const override;
        int RecommendedPasses() const override { return 1; }
        OutputTransform GetOutputTransform() const override
        {
            return OutputTransform::Linear;
        } // already display colors

    protected:
        bool JitterSamples() const override { return false; }
        Vec3 Li(const RenderScene& scene, const Ray& ray, ISampler&) const override;

    private:
        Count what;
        int maxTests;
    };
}