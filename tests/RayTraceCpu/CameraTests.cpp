#include "TestHelpers.h"

#include <Duck/RayTraceCpu/Camera.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;

TEST_CASE("Camera rays span the field of view", "[camera]")
{
    const Vec3 forward(0.0f, 0.0f, -1.0f);
    const Camera camera({0.0f, 0.0f, 0.0f}, forward, {0.0f, 1.0f, 0.0f}, 90.0f, 2.0f);

    CHECK_THAT(camera.GetRay(0.5f, 0.5f).Direction, Vec3Near(forward, 1e-6f));

    // t = 0 is the TOP edge, half the vertical fov above the center
    const Vec3 top = camera.GetRay(0.5f, 0.0f).Direction;
    CHECK(top.y > 0.0f);
    CHECK_THAT(Degrees(std::acos(glm::dot(top, forward))), WithinAbs(45.0f, 1e-3f));

    // s = 0 is the LEFT edge, wider by the aspect ratio: atan(aspect * tan(fov / 2))
    const Vec3 left = camera.GetRay(0.0f, 0.5f).Direction;
    CHECK(left.x < 0.0f);
    CHECK_THAT(Degrees(std::acos(glm::dot(left, forward))), WithinAbs(Degrees(std::atan(2.0f)), 1e-3f));
}

TEST_CASE("Camera center ray points at the target", "[camera]")
{
    const Vec3 position(1.0f, 2.0f, 3.0f);
    const Vec3 target(-2.0f, 0.0f, -4.0f);
    const Camera camera(position, target, {0.0f, 1.0f, 0.0f}, 60.0f, 16.0f / 9.0f);

    const Ray ray = camera.GetRay(0.5f, 0.5f);
    CHECK_THAT(ray.Origin, Vec3Near(position, 1e-6f));
    CHECK_THAT(ray.Direction, Vec3Near(glm::normalize(target - position), 1e-5f));
}