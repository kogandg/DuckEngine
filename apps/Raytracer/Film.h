#pragma once

#include <Duck/Core/Framebuffer.h>
#include <Duck/Core/ThreadPool.h>

#include <cstdint>

namespace Duck
{
    enum class OutputTransform
    {
        Srgb,   // linear radiance to sRGB encoding
        Linear, // written as is
    };

    struct Film
    {
        Film(int filmWidth, int filmHeight) : Sum(filmWidth, filmHeight) {}

        void Reset()
        {
            Sum.Clear();
            Passes = 0;
        }

        void Resolve(Framebuffer& output, OutputTransform transform = OutputTransform::Srgb) const
        {
            DUCK_ASSERT(output.Width() == Sum.Width() && output.Height() == Sum.Height());
            const float inverse = Passes > 0 ? 1.0f / static_cast<float>(Passes) : 0.0f;
            const bool encode = transform == OutputTransform::Srgb;

            ThreadPool::Shared().ParallelFor(Sum.Height(),
                                             [&](int y)
                                             {
                                                 for (int x = 0; x < Sum.Width(); x++)
                                                 {
                                                     Vec3 color = Sum[x, y] * inverse;
                                                     if (encode)
                                                     {
                                                         color = LinearToSrgb(color);
                                                     }
                                                     output[x, y] = PackRgba8(color);
                                                 }
                                             });
        }

        HdrFramebuffer Sum; // sum of every pass so far
        int Passes = 0;     // how many passes are in Sum
    };
}