#include <Duck/RayTraceCpu/Sampler.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <vector>

using namespace Duck;
using namespace Duck::RayTraceCpu;

namespace
{
    // fraction of the unit square below a tilted line through it, sampled with one dimension of the sampler
    float EstimateEdgeCoverage(ISampler& sampler, IVec2 pixel, int sampleCount)
    {
        int inside = 0;
        for (int i = 0; i < sampleCount; i++)
        {
            sampler.StartPixelSample(pixel, i);
            const Vec2 p = sampler.Get2D();
            inside += (p.y < 0.2f + 0.6f * p.x) ? 1 : 0; // exact area is 0.5
        }
        return static_cast<float>(inside) / static_cast<float>(sampleCount);
    }

    float RmsEdgeError(ISampler& sampler, int sampleCount)
    {
        double sum = 0.0;
        constexpr int PixelCount = 256;
        for (int i = 0; i < PixelCount; i++)
        {
            const double error = EstimateEdgeCoverage(sampler, {i, 3 * i}, sampleCount) - 0.5;
            sum += error * error;
        }
        return static_cast<float>(std::sqrt(sum / PixelCount));
    }
}

TEST_CASE("Samplers return values in [0, 1)", "[sampler]")
{
    IndependentSampler independent(3u);
    SobolSampler sobol(3u);
    for (ISampler* sampler : {static_cast<ISampler*>(&independent), static_cast<ISampler*>(&sobol)})
    {
        for (int sample = 0; sample < 256; sample++)
        {
            sampler->StartPixelSample({sample % 7, sample % 5}, sample);
            for (int dimension = 0; dimension < 8; dimension++)
            {
                const float u = sampler->Get1D();
                const Vec2 v = sampler->Get2D();
                CHECK(u >= 0.0f);
                CHECK(u < 1.0f);
                CHECK(v.x >= 0.0f);
                CHECK(v.x < 1.0f);
                CHECK(v.y >= 0.0f);
                CHECK(v.y < 1.0f);
            }
        }
    }
}

TEST_CASE("Sobol sampler is deterministic and clones match", "[sampler]")
{
    SobolSampler a(9u);
    const std::unique_ptr<ISampler> b = a.Clone();
    a.StartPixelSample({12, 34}, 5);
    b->StartPixelSample({12, 34}, 5);
    for (int dimension = 0; dimension < 6; dimension++)
    {
        CHECK(a.Get1D() == b->Get1D());
        const Vec2 va = a.Get2D();
        const Vec2 vb = b->Get2D();
        CHECK(va.x == vb.x);
        CHECK(va.y == vb.y);
    }
}

TEST_CASE("Sobol sampler gives different pixels different points", "[sampler]")
{
    SobolSampler sampler;
    sampler.StartPixelSample({0, 0}, 0);
    const Vec2 first = sampler.Get2D();
    sampler.StartPixelSample({1, 0}, 0);
    const Vec2 second = sampler.Get2D();
    sampler.StartPixelSample({0, 1}, 0);
    const Vec2 third = sampler.Get2D();
    CHECK((first.x != second.x || first.y != second.y));
    CHECK((first.x != third.x || first.y != third.y));
}

TEST_CASE("Sobol 2D samples are stratified for every power of two prefix", "[sampler]")
{
    // (0,2)-sequence property: any aligned block of 2^m samples puts exactly one point in each cell of
    // every 2^k x 2^(m-k) grid. checked for several pixels, dimensions and blocks
    SobolSampler sampler(7u);
    for (int pixel = 0; pixel < 16; pixel++)
    {
        for (int dimension = 0; dimension < 4; dimension++)
        {
            for (const int m : {2, 4, 6})
            {
                const int count = 1 << m;
                for (const int block : {0, 1, 3})
                {
                    std::vector<Vec2> points;
                    for (int i = 0; i < count; i++)
                    {
                        sampler.StartPixelSample({pixel, 2 * pixel}, block * count + i);
                        for (int skip = 0; skip < dimension; skip++)
                        {
                            sampler.Get2D();
                        }
                        points.push_back(sampler.Get2D());
                    }

                    for (int k = 0; k <= m; k++)
                    {
                        const int columns = 1 << k;
                        const int rows = 1 << (m - k);
                        std::vector<int> cells(static_cast<size_t>(count), 0);
                        for (const Vec2& p : points)
                        {
                            const int column = static_cast<int>(p.x * static_cast<float>(columns));
                            const int row = static_cast<int>(p.y * static_cast<float>(rows));
                            cells[static_cast<size_t>(row * columns + column)]++;
                        }
                        for (const int cell : cells)
                        {
                            CHECK(cell == 1);
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("Sobol 1D samples are stratified for every power of two prefix", "[sampler]")
{
    SobolSampler sampler(11u);
    constexpr int Count = 32;
    std::vector<int> cells(Count, 0);
    for (int i = 0; i < Count; i++)
    {
        sampler.StartPixelSample({5, 6}, i);
        sampler.Get2D(); // a different dimension first, like the pixel offset
        cells[static_cast<size_t>(sampler.Get1D() * Count)]++;
    }
    for (const int cell : cells)
    {
        CHECK(cell == 1);
    }
}

TEST_CASE("Sobol sampler antialiases an edge with less error than independent", "[sampler]")
{
    // at 64 samples Owen scrambled Sobol is typically 3-4x more accurate on an edge
    IndependentSampler independent(1u);
    SobolSampler sobol(1u);
    const float independentError = RmsEdgeError(independent, 64);
    const float sobolError = RmsEdgeError(sobol, 64);
    CHECK(sobolError * 2.0f < independentError);
}
