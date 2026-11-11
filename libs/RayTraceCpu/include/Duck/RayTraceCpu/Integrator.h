#pragma once

#include <Duck/Core/Math.h>
#include <Duck/RayTraceCpu/Film.h>

#include <algorithm>
#include <cstdint>


namespace Duck
{
    struct Ray;
    class Random;
    class ThreadPool;
}

namespace Duck::RayTraceCpu
{
    class Camera;
    struct Scene;

    // anything that can add a pass of work onto film
    class IIntegrator
    {
    public:
        virtual ~IIntegrator() = default;

        virtual const char* Name() const = 0;

        // adds one pass onto film. Passes is the index of THIS pass
        virtual void RenderPass(const Scene& scene, const Camera& camera, Film& film, ThreadPool& pool) = 0;

        // how many passes are worth rendering before image stops improving noticeably
        virtual int RecommendedPasses() const { return 256; }
        //how the film should turn this integrators accumulated values into display pixels
        virtual OutputTransform GetOutputTransform() const { return OutputTransform::Srgb; }
    };

    class PixelIntegrator : public IIntegrator
    {
    public:
        void RenderPass(const Scene& scene, const Camera& camera, Film& film, ThreadPool& pool) final;

    protected:
        // radiance arriving along ray
        virtual Vec3 Li(const Scene& scene, const Ray& ray, Random& random) const = 0;

        virtual bool JitterSamples() const { return true; }

    private:
        static constexpr int TileSize = 32;
    };
}