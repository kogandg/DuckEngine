#include "TestHelpers.h"

#include <Duck/PathTraceCpu/Lights.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Duck;
using namespace Duck::PathTrace;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;

TEST_CASE("Point light samples point at the light and fall off with distance squared", "[lights]")
{
    Random random(1u);
    const Light light = PointLight{{0.0f, 3.0f, 4.0f}, Vec3(10.0f)};
    const LightSample sample = SampleLight(light, Vec3(0.0f), random);

    CHECK_THAT(sample.Direction, Vec3Near(Vec3(0.0f, 0.6f, 0.8f), 1e-6f));
    CHECK_THAT(sample.Distance, WithinAbs(5.0f, 1e-5f));
    CHECK_THAT(sample.Radiance, Vec3Near(Vec3(10.0f / 25.0f), 1e-6f));
    CHECK(sample.Pdf == 1.0f);
    CHECK(sample.IsDelta);
}

TEST_CASE("Directional light samples are the same everywhere", "[lights]")
{
    Random random(1u);
    const Light light = DirectionalLight{{0.0f, 1.0f, 0.0f}, Vec3(2.0f)};
    for (const Vec3 point : {Vec3(0.0f), Vec3(100.0f, -5.0f, 3.0f)})
    {
        const LightSample sample = SampleLight(light, point, random);
        CHECK_THAT(sample.Direction, Vec3Near(Vec3(0.0f, 1.0f, 0.0f), 0.0f));
        CHECK(sample.Distance == Infinity);
        CHECK_THAT(sample.Radiance, Vec3Near(Vec3(2.0f), 0.0f));
        CHECK(sample.IsDelta);
    }
}