#include <Duck/RayTraceCpu/Integrator.h>

#include <Duck/Core/Random.h>
#include <Duck/Core/ThreadPool.h>
#include <Duck/RayTraceCpu/Camera.h>

#include <algorithm>
#include <cstdint>

namespace Duck::RayTraceCpu
{
    void PixelIntegrator::RenderPass(const Scene& scene, const Camera& camera, Film& film, ThreadPool& pool)
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
                                         static_cast<uint64_t>(y) * static_cast<uint64_t>(width) +
                                         static_cast<uint64_t>(x);
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
}