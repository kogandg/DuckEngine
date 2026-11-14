#include <Duck/RayTraceCpu/Integrator.h>

#include <Duck/Core/Random.h>
#include <Duck/Core/ThreadPool.h>
#include <Duck/RayTraceCpu/Camera.h>
#include <Duck/RayTraceCpu/RayStats.h>
#include <Duck/RayTraceCpu/Sampler.h>

#include <algorithm>
#include <cstdint>
#include <memory>

namespace Duck::RayTraceCpu
{
    void PixelIntegrator::RenderPass(const RenderScene& scene, const Camera& camera, Film& film, ThreadPool& pool,
                                     const ISampler& samplerPrototype, std::stop_token cancel)
    {
        const int width = film.Sum.Width();
        const int height = film.Sum.Height();
        const int tilesX = (width + TileSize - 1) / TileSize;
        const int tilesY = (height + TileSize - 1) / TileSize;
        const int pass = film.Passes;
        const bool jitter = JitterSamples();

        // one index per tile, tiles never overlap so threads never write to the same pixel
        pool.ParallelFor(tilesX * tilesY,
                         [&](int tile)
                         {
                             if (cancel.stop_requested())
                             {
                                 return;
                             }
                             const std::unique_ptr<ISampler> sampler = samplerPrototype.Clone();

                             // edges of the tile
                             const int x0 = (tile % tilesX) * TileSize;
                             const int y0 = (tile / tilesX) * TileSize;
                             const int x1 = std::min(x0 + TileSize, width);
                             const int y1 = std::min(y0 + TileSize, height);

                             for (int y = y0; y < y1; y++)
                             {
                                 if (cancel.stop_requested())
                                 {
                                     break;
                                 }
                                 for (int x = x0; x < x1; x++)
                                 {
                                     sampler->StartPixelSample({x, y}, pass);
                                     const Vec2 offset = jitter ? sampler->GetPixel2D() : Vec2(0.5f);

                                     // sample the pixel center
                                     const float s = (static_cast<float>(x) + offset.x) / static_cast<float>(width);
                                     const float t = (static_cast<float>(y) + offset.y) / static_cast<float>(height);

                                     film.Sum[x, y] += Li(scene, camera.GetRay(s, t), *sampler);
                                 }
                             }

                             RayStats::FlushThread();
                         });
    }
}