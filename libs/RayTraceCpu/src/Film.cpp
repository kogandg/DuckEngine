#include <Duck/RayTraceCpu/Film.h>

#include <Duck/Core/Assert.h>
#include <Duck/Core/Color.h>
#include <Duck/Core/ThreadPool.h>

namespace Duck::RayTraceCpu
{
    void Film::Reset()
    {
        Sum.Clear();
        Passes = 0;
    }

    void Film::Resolve(Framebuffer& output, ThreadPool& pool, OutputTransform transform) const
    {
        DUCK_ASSERT(output.Width() == Sum.Width() && output.Height() == Sum.Height());
        const float inverse = Passes > 0 ? 1.0f / static_cast<float>(Passes) : 0.0f;
        const bool encode = transform == OutputTransform::Srgb;

        pool.ParallelFor(Sum.Height(),
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

}