#include <Duck/RayTraceCpu/Film.h>

#include <Duck/Core/Assert.h>
#include <Duck/Core/Color.h>
#include <Duck/Core/ThreadPool.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <numeric>
#include <vector>

namespace Duck::RayTraceCpu
{
    namespace
    {
        // difference between the mean of all samples and the mean of the first half, relative to the square root
        // of the brightness. the two halves are independent estimates, so their difference shrinks at the same rate as
        // the real error. dividing by sqrt keeps dark pixels from stopping too early and bright ones from sampling
        // forever
        float EstimateError(const Vec3& sum, const Vec3& firstHalfSum, int count)
        {
            const Vec3 full = sum / static_cast<float>(count);
            const Vec3 half = firstHalfSum / static_cast<float>(count / 2);
            const Vec3 difference = glm::abs(full - half);
            return (difference.x + difference.y + difference.z) / (0.0001f + std::sqrt(full.x + full.y + full.z));
        }
    }

    void Film::Reset()
    {
        Sum.Clear();
        Checkpoint.Clear();
        StoppedAt.Clear();
        Error.Clear();
        Passes = 0;
        ActivePixels = Sum.Width() * Sum.Height();
    }

    void Film::Resolve(Framebuffer& output, ThreadPool& pool, OutputTransform transform) const
    {
        DUCK_ASSERT(output.Width() == Sum.Width() && output.Height() == Sum.Height());
        const bool encode = transform == OutputTransform::Srgb;

        pool.ParallelFor(Sum.Height(),
                         [&](int y)
                         {
                             for (int x = 0; x < Sum.Width(); x++)
                             {
                                 const int count = SampleCount(x, y);
                                 Vec3 color = count > 0 ? Sum[x, y] / static_cast<float>(count) : Vec3(0.0f);
                                 if (encode)
                                 {
                                     color = LinearToSrgb(color);
                                 }
                                 output[x, y] = PackRgba8(color);
                             }
                         });
    }

    void Film::UpdateAdaptive(ThreadPool& pool, const AdaptiveSettings& settings)
    {
        // only check when every active pixel has a power of two sample count. Sobol samples are evenly spread at
        // those counts, so both halves are good estimates, and stopped pixels keep a well spread set of samples
        if (!settings.Enabled() || Passes <= 0 || !std::has_single_bit(static_cast<unsigned>(Passes)))
        {
            return;
        }

        const int width = Sum.Width();
        const int height = Sum.Height();
        const bool check = Passes >= 2 && Passes >= settings.MinPasses;

        if (check)
        {
            pool.ParallelFor(height,
                             [&](int y)
                             {
                                 for (int x = 0; x < width; x++)
                                 {
                                     Error[x, y] =
                                         IsActive(x, y) ? EstimateError(Sum[x, y], Checkpoint[x, y], Passes) : 0.0f;
                                 }
                             });

            // a pixel only stops if its whole 3x3 neighbourhood is converged. this catches edges where one
            // pixel got lucky, and keeps the stopped region from having single pixel holes
            std::vector<int> activeInRow(static_cast<size_t>(height), 0);
            pool.ParallelFor(height,
                             [&](int y)
                             {
                                 int active = 0;
                                 for (int x = 0; x < width; x++)
                                 {
                                     if (!IsActive(x, y))
                                     {
                                         continue;
                                     }
                                     float worst = 0.0f;
                                     for (int ny = std::max(y - 1, 0); ny <= std::min(y + 1, height - 1); ny++)
                                     {
                                         for (int nx = std::max(x - 1, 0); nx <= std::min(x + 1, width - 1); nx++)
                                         {
                                             worst = std::max(worst, Error[nx, ny]);
                                         }
                                     }
                                     if (worst < settings.Threshold)
                                     {
                                         StoppedAt[x, y] = Passes; // only this threads row is written
                                     }
                                     else
                                     {
                                         active++;
                                     }
                                 }
                                 activeInRow[static_cast<size_t>(y)] = active;
                             });
            ActivePixels = std::reduce(activeInRow.begin(), activeInRow.end());
        }

        // this sum is the first half of the samples at the next check
        pool.ParallelFor(height,
                         [&](int y)
                         {
                             for (int x = 0; x < width; x++)
                             {
                                 if (IsActive(x, y))
                                 {
                                     Checkpoint[x, y] = Sum[x, y];
                                 }
                             }
                         });
    }
}