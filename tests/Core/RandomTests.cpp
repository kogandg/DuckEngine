#include <Duck/Core/Random.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <limits>

using namespace Duck;
using Catch::Matchers::WithinAbs;

TEST_CASE("Random matches the PCG32 reference output", "[random]")
{
    // first outputs of pcg32-demo from the reference pcg-c-basic, seeded with (42, 54)
    constexpr std::array<uint32_t, 6> expected = {0xa15c02b7u, 0x7b47f409u, 0xba1d3330u,
                                                  0x83d2f293u, 0xbfa4784bu, 0xcbed606eu};
    Random random(42u, 54u);
    for (const uint32_t value : expected)
    {
        CHECK(random.NextUint() == value);
    }
}

TEST_CASE("Random repeats for the same seed and stream", "[random]")
{
    Random a(123u, 7u);
    Random b(123u, 7u);
    for (int i = 0; i < 1000; i++)
    {
        REQUIRE(a.NextUint() == b.NextUint());
    }
}

TEST_CASE("Random streams with the same seed differ", "[random]")
{
    Random a(123u, 1u);
    Random b(123u, 2u);
    int matches = 0;
    for (int i = 0; i < 1000; i++)
    {
        matches += a.NextUint() == b.NextUint() ? 1 : 0;
    }
    CHECK(matches == 0);
}

TEST_CASE("ToUnitFloat stays below 1", "[random]")
{
    CHECK(Random::ToUnitFloat(0u) == 0.0f);
    // a careless conversion rounds the largest input up to exactly 1.0
    const float largest = Random::ToUnitFloat(std::numeric_limits<uint32_t>::max());
    CHECK(largest < 1.0f);
    CHECK(largest == 1.0f - 1.0f / 16777216.0f);
}

TEST_CASE("NextFloat is uniform", "[random]")
{
    constexpr int Count = 640000;
    constexpr int Buckets = 64;

    Random random(2024u);
    std::array<int, Buckets> histogram{};
    int outOfRange = 0;
    double sum = 0.0;
    double sumSquares = 0.0;
    for (int i = 0; i < Count; i++)
    {
        const float value = random.NextFloat();
        if (value < 0.0f || value >= 1.0f)
        {
            outOfRange++;
            continue;
        }
        sum += value;
        sumSquares += static_cast<double>(value) * value;
        histogram[static_cast<size_t>(value * Buckets)]++;
    }
    REQUIRE(outOfRange == 0);

    // tolerances are about 5 standard errors. the seed is fixed, so these can't fail randomly
    const double mean = sum / Count;
    const double variance = sumSquares / Count - mean * mean;
    CHECK_THAT(mean, WithinAbs(0.5, 0.002));
    CHECK_THAT(variance, WithinAbs(1.0 / 12.0, 0.0005));

    // chi-square against a flat histogram. 103.4 is the 99.9th percentile for 63 degrees of freedom
    const double expectedPerBucket = static_cast<double>(Count) / Buckets;
    double chiSquare = 0.0;
    for (const int observed : histogram)
    {
        const double difference = observed - expectedPerBucket;
        chiSquare += difference * difference / expectedPerBucket;
    }
    CHECK(chiSquare < 103.4);
}

TEST_CASE("HashU64 matches SplitMix64", "[random]")
{
    // first SplitMix64 output for state 0
    CHECK(HashU64(0u) == 0xe220a8397b1dcdafull);
}

TEST_CASE("HashU64 avalanches", "[random]")
{
    // flipping any one input bit should flip about half (32) of the output bits
    long long flipped = 0;
    int trials = 0;
    for (uint64_t i = 0; i < 1000; i++)
    {
        const uint64_t x = HashU64(i + 12345u);
        for (int bit = 0; bit < 64; bit++)
        {
            flipped += std::popcount(HashU64(x) ^ HashU64(x ^ (1ull << bit)));
            trials++;
        }
    }
    CHECK_THAT(static_cast<double>(flipped) / trials, WithinAbs(32.0, 0.2));
}