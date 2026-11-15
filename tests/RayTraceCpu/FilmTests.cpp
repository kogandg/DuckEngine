#include <Duck/Core/Color.h>
#include <Duck/Core/Framebuffer.h>
#include <Duck/Core/ThreadPool.h>
#include <Duck/RayTraceCpu/Film.h>
#include <Duck/Core/Random.h>

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

namespace
{
    // runs passes on a 5x1 film. pixel 0 is noisy, the rest are a flat colour
    Film RunNoisyRow(int passes, const AdaptiveSettings& settings)
    {
        Film film(5, 1);
        film.Reset();
        Random random(42u);
        for (int pass = 0; pass < passes; pass++)
        {
            for (int x = 0; x < 5; x++)
            {
                if (film.IsActive(x, 0))
                {
                    film.Sum[x, 0] += x == 0 ? Vec3(random.NextFloat(), random.NextFloat(), random.NextFloat())
                                             : Vec3(0.25f, 0.5f, 0.75f);
                }
            }
            film.Passes++;
            film.UpdateAdaptive(ThreadPool::Shared(), settings);
        }
        return film;
    }
}

TEST_CASE("Adaptive film stops flat pixels and keeps noisy ones and their neighbours", "[film]")
{
    const Film film = RunNoisyRow(16, {.MinPasses = 4, .Threshold = 0.01f});

    CHECK(film.IsActive(0, 0));       // noisy
    CHECK(film.IsActive(1, 0));       // flat, but next to a noisy pixel
    CHECK(film.StoppedAt[2, 0] == 4); // flat: stops at the first check
    CHECK(film.StoppedAt[3, 0] == 4);
    CHECK(film.StoppedAt[4, 0] == 4);
    CHECK(film.ActivePixels == 2);
    CHECK(film.SampleCount(0, 0) == 16);
    CHECK(film.SampleCount(4, 0) == 4);
}

TEST_CASE("Adaptive film respects the minimum pass count", "[film]")
{
    const Film film = RunNoisyRow(8, {.MinPasses = 16, .Threshold = 0.01f});
    CHECK(film.ActivePixels == 5);
    for (int x = 0; x < 5; x++)
    {
        CHECK(film.IsActive(x, 0));
    }
}

TEST_CASE("Adaptive film does nothing when disabled", "[film]")
{
    const Film film = RunNoisyRow(16, {});
    CHECK(film.ActivePixels == 5);
}

TEST_CASE("Film resolve divides each pixel by its own sample count", "[film]")
{
    Film film(2, 1);
    film.Reset();
    film.Sum[0, 0] = Vec3(4.0f); // 8 passes ran, so 0.5
    film.Sum[1, 0] = Vec3(2.0f); // stopped after 4, so 0.5 as well
    film.Passes = 8;
    film.StoppedAt[1, 0] = 4;

    Framebuffer display(2, 1);
    film.Resolve(display, ThreadPool::Shared(), OutputTransform::Linear);
    CHECK(display[0, 0] == PackRgba8(128, 128, 128));
    CHECK(display[1, 0] == PackRgba8(128, 128, 128));
}