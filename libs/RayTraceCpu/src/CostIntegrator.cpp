#include <Duck/RayTraceCpu/CostIntegrator.h>

#include <Duck/RayTraceCpu/RayStats.h>
#include <Duck/RayTraceCpu/RenderScene.h>
#include <Duck/RayTraceCpu/Shading.h>

#include <algorithm>
#include <cmath>

namespace Duck::RayTraceCpu
{
    namespace
    {
        // Google's Turbo colormap,even perceptual steps from dark blue to dark red. x in [0, 1], returns a display
        // color
        Vec3 Turbo(float x)
        {
            x = std::clamp(x, 0.0f, 1.0f);
            const Vec4 v4(1.0f, x, x * x, x * x * x);
            const Vec2 v2 = Vec2(v4.z, v4.w) * v4.z;
            return {glm::dot(v4, Vec4(0.13572138f, 4.61539260f, -42.66032258f, 132.13108234f)) +
                        glm::dot(v2, Vec2(-152.94239396f, 59.28637943f)),
                    glm::dot(v4, Vec4(0.09140261f, 2.19418839f, 4.84296658f, -14.18503333f)) +
                        glm::dot(v2, Vec2(4.27729857f, 2.82956604f)),
                    glm::dot(v4, Vec4(0.10667330f, 12.64194608f, -60.58204836f, 110.36276771f)) +
                        glm::dot(v2, Vec2(-89.90310912f, 27.34824973f))};
        }
    }

    const char* CostIntegrator::Name() const
    {
        switch (what)
        {
        case Count::BoxTests:
            return "Cost: box tests";
        case Count::PrimitiveTests:
            return "Cost: primitive tests";
        case Count::Total:
            break;
        }
        return "Cost: all tests";
    }

    Vec3 CostIntegrator::Li(const RenderScene& scene, const Ray& ray, ISampler&) const
    {
        TraversalStats stats;
        scene.ClosestHit(ray, RayEpsilon, Infinity, &stats);

        int count = stats.NodeTests + stats.PrimitiveTests;
        if (what == Count::BoxTests)
        {
            count = stats.NodeTests;
        }
        else if (what == Count::PrimitiveTests)
        {
            count = stats.PrimitiveTests;
        }

        // log scale, so 10 tests and 2000 tests are both readable in one image
        const float x = std::log1p(static_cast<float>(count)) / std::log1p(static_cast<float>(maxTests));
        return Turbo(x);
    }
}