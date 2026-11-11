#include <Duck/Core/Color.h>
#include <Duck/Core/Framebuffer.h>
#include <Duck/Core/ThreadPool.h>
#include <Duck/RayTraceCpu/Film.h>

#include <catch2/catch_test_macros.hpp>

using namespace Duck;
using namespace Duck::RayTraceCpu;

TEST_CASE("Film resolve averages in linear space and encodes sRGB once", "[film]")
{
    Film film(1, 1);
    film.Sum[0, 0] = Vec3(1.0f);
    film.Passes = 2; // average is linear 0.5
    Framebuffer display(1, 1);

    // linear 0.5 -> sRGB 0.7354 -> 188. encoding twice would give 223, not encoding at all 128
    film.Resolve(display, ThreadPool::Shared(), OutputTransform::Srgb);
    const Rgba8 encoded = display[0, 0];
    CHECK(encoded == PackRgba8(188, 188, 188));

    film.Resolve(display, ThreadPool::Shared(), OutputTransform::Linear);
    const Rgba8 linear = display[0, 0];
    CHECK(linear == PackRgba8(128, 128, 128));
}