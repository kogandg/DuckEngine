#pragma once

#include "Camera.h"
#include "Film.h"
#include "Scene.h"

#include <Duck/Core/Random.h>
#include <Duck/Core/ThreadPool.h>

#include <algorithm>
#include <cstdint>

namespace Duck
{
    // anything that can add a pass of work onto film
    class IIntegrator
    {
    public:
        virtual ~IIntegrator() = default;

        virtual const char* Name() const = 0;

        // adds one pass onto film. Passes is the index of THIS pass
        virtual void RenderPass(const Scene& scene, const Camera& camera, Film& film) = 0;

        // how many passes are worth rendering before image stops improving noticeably
        virtual int RecommendedPasses() const { return 256; }
        //how the film should turn this integrators accumulated values into display pixels
        virtual OutputTransform GetOutputTransform() const { return OutputTransform::Srgb; }
    };

    class PixelIntegrator : public IIntegrator
    {
    public:
        void RenderPass(const Scene& scene, const Camera& camera, Film& film) final
        {
            const int width = film.Sum.Width();
            const int height = film.Sum.Height();
            const int tilesX = (width + TileSize - 1) / TileSize;
            const int tilesY = (height + TileSize - 1) / TileSize;
            const int pass = film.Passes;
            const bool jitter = JitterSamples();

            // one index per tile, tiles never overlap so threads never write to the same pixel
            ThreadPool::Shared().ParallelFor(
                tilesX * tilesY,
                [&](int tile)
                {
                    // edges of the tile
                    const int x0 = (tile % tilesX) * TileSize;
                    const int y0 = (tile / tilesX) * TileSize;
                    const int x1 = std::min(x0 + TileSize, width);
                    const int y1 = std::min(y0 + TileSize, height);

                    for (int y = y0; y < y1; y++)
                    {
                        for (int x = x0; x < x1; x++)
                        {
                            // same pixel with different pass shoud use different stream
                            const uint64_t pixelIndex =
                                static_cast<uint64_t>(y) * static_cast<uint64_t>(width) + static_cast<uint64_t>(x);
                            Random random(HashU64(pixelIndex), HashU64(static_cast<uint64_t>(pass)));

                            const float jx = jitter ? random.NextFloat() : 0.5f;
                            const float jy = jitter ? random.NextFloat() : 0.5f;

                            // sample the pixel center
                            const float s = (static_cast<float>(x) + jx) / static_cast<float>(width);
                            const float t = (static_cast<float>(y) + jy) / static_cast<float>(height);

                            film.Sum[x, y] += Li(scene, camera.GetRay(s, t), random);
                        }
                    }
                });
        }

    protected:
        // radiance arriving along ray
        virtual Vec3 Li(const Scene& scene, const Ray& ray, Random& random) const = 0;

        virtual bool JitterSamples() const { return true; }

    private:
        static constexpr int TileSize = 32;
    };
}