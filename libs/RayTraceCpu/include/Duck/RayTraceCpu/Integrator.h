#pragma once

#include <Duck/Core/Math.h>
#include <Duck/RayTraceCpu/Film.h>

#include <algorithm>
#include <cstdint>
#include <stop_token>


namespace Duck
{
    struct Ray;
    class Random;
    class ThreadPool;
}

namespace Duck::RayTraceCpu
{
    class Camera;
    class RenderScene;
    class ISampler;

    // anything that can add a pass of work onto film
    class IIntegrator
    {
    public:
        virtual ~IIntegrator() = default;

        virtual const char* Name() const = 0;

        // adds one pass onto film. Passes is the index of THIS pass
        virtual void RenderPass(const RenderScene& scene, const Camera& camera, Film& film, ThreadPool& pool, const ISampler& sampler, std::stop_token cancel = {}) = 0;

        // how many passes are worth rendering before image stops improving noticeably
        virtual int RecommendedPasses() const { return 256; }
        //how the film should turn this integrators accumulated values into display pixels
        virtual OutputTransform GetOutputTransform() const { return OutputTransform::Srgb; }
    };

    class PixelIntegrator : public IIntegrator
    {
    public:
        void RenderPass(const RenderScene& scene, const Camera& camera, Film& film, ThreadPool& pool,
                        const ISampler& sampler, std::stop_token cancel = {}) final;

    protected:
        // radiance arriving along ray
        virtual Vec3 Li(const RenderScene& scene, const Ray& ray, ISampler& sampler) const = 0;

        virtual bool JitterSamples() const { return true; }

    private:
        static constexpr int TileSize = 32;
    };
}