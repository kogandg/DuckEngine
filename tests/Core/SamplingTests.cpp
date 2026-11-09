#include "TestHelpers.h"

#include <Duck/Core/Sampling.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Duck;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;

TEST_CASE("ConcentricDisk maps the square onto the unit disk", "[sampling]")
{
    // center of the square to the origin, corners to the rim
    CHECK(ConcentricDisk(0.5f, 0.5f) == Vec2(0.0f));
    CHECK_THAT(glm::length(ConcentricDisk(0.0f, 0.0f)), WithinAbs(1.0f, 1e-5f));
    CHECK_THAT(glm::length(ConcentricDisk(1.0f, 1.0f)), WithinAbs(1.0f, 1e-5f));

    constexpr int Count = 200000;
    Random random(7u);
    int outside = 0;
    double radiusSquaredSum = 0.0;
    double xSum = 0.0;
    for (int i = 0; i < Count; i++)
    {
        const Vec2 p = ConcentricDisk(random.NextFloat(), random.NextFloat());
        const float radiusSquared = glm::dot(p, p);
        outside += radiusSquared > 1.0f + 1e-5f ? 1 : 0;
        radiusSquaredSum += radiusSquared;
        xSum += p.x;
    }
    CHECK(outside == 0);
    // uniform over the area means r^2 is uniform in [0, 1], so its mean is 1/2
    CHECK_THAT(radiusSquaredSum / Count, WithinAbs(0.5, 0.003));
    CHECK_THAT(xSum / Count, WithinAbs(0.0, 0.005));
}

TEST_CASE("CosineHemisphereLocal is cosine distributed around +Z", "[sampling]")
{
    constexpr int Count = 200000;
    Random random(8u);
    int invalid = 0;
    double cosineSum = 0.0;
    for (int i = 0; i < Count; i++)
    {
        const Vec3 w = CosineHemisphereLocal(random.NextFloat(), random.NextFloat());
        invalid += (w.z < 0.0f || std::abs(glm::length(w) - 1.0f) > 1e-5f) ? 1 : 0;
        cosineSum += w.z;
    }
    CHECK(invalid == 0);
    // for pdf cos(theta) / pi the mean of cos(theta) is 2/3 (uniform hemisphere would be 1/2)
    CHECK_THAT(cosineSum / Count, WithinAbs(2.0 / 3.0, 0.003));
}

TEST_CASE("OrthonormalBasis builds a right-handed orthonormal frame", "[sampling]")
{
    auto check = [](const Vec3& n)
    {
        Vec3 tangent;
        Vec3 bitangent;
        OrthonormalBasis(n, tangent, bitangent);
        CAPTURE(n, tangent, bitangent);
        REQUIRE_THAT(glm::length(tangent), WithinAbs(1.0f, 1e-4f));
        REQUIRE_THAT(glm::length(bitangent), WithinAbs(1.0f, 1e-4f));
        REQUIRE_THAT(glm::dot(tangent, bitangent), WithinAbs(0.0f, 1e-4f));
        REQUIRE_THAT(glm::dot(tangent, n), WithinAbs(0.0f, 1e-4f));
        REQUIRE_THAT(glm::dot(bitangent, n), WithinAbs(0.0f, 1e-4f));
        REQUIRE_THAT(glm::cross(tangent, bitangent), Vec3Near(n, 1e-4f));
    };

    // axes, and the spots where the formula switches branch (z = +-0, z near -1)
    check({0.0f, 0.0f, 1.0f});
    check({0.0f, 0.0f, -1.0f});
    check({1.0f, 0.0f, 0.0f});
    check({1.0f, 0.0f, -0.0f});
    check({0.0f, 1.0f, 0.0f});
    check(glm::normalize(Vec3(1e-4f, 0.0f, -1.0f)));

    Random random(5u);
    for (int i = 0; i < PropertySamples; i++)
    {
        check(RandomUnitVector(random));
    }
}

TEST_CASE("SampleCosineHemisphere stays on the normal's side", "[sampling]")
{
    Random random(9u);
    double cosineSum = 0.0;
    for (int i = 0; i < PropertySamples; i++)
    {
        const Vec3 n = RandomUnitVector(random);
        const Vec3 w = SampleCosineHemisphere(n, random);
        CAPTURE(n, w);
        REQUIRE_THAT(glm::length(w), WithinAbs(1.0f, 1e-5f));
        REQUIRE(glm::dot(w, n) >= -1e-5f);
        cosineSum += glm::dot(w, n);
    }
    CHECK_THAT(cosineSum / PropertySamples, WithinAbs(2.0 / 3.0, 0.01));
}