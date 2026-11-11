#include "RenderTestHelpers.h"

#include <Duck/RayTraceCpu/Shading.h>
#include <Duck/RayTraceCpu/Materials.h>
#include <Duck/RayTraceCpu/Scene.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;

using Catch::Matchers::WithinAbs;

TEST_CASE("Rays leaving a surface right next to an edge still hit the neighboring face", "[shading]")
{
    // an inside corner: a floor at y = 0 meets a wall at x = 0
    Scene scene;
    const int gray = scene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)));
    scene.Quads = {
        {{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 2.0f}, {1.0f, 0.0f, 0.0f}, gray}, // floor, x from 0 to 1
        {{0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 2.0f}, gray}, // wall at x = 0
    };

    // leave the floor 0.0005 from the corner, heading for the wall almost along the floor.
    // the wall is ~0.0005 away: closer than the old fixed 0.001, so the old code went straight through it
    Hit hit;
    hit.Point = {0.0005f, 0.0f, 0.0f};
    hit.Normal = {0.0f, 1.0f, 0.0f};
    const Ray ray = SpawnRay(hit, glm::normalize(Vec3(-1.0f, 0.1f, 0.0f)));

    const auto next = scene.ClosestHit(ray, RayEpsilon, Infinity);
    REQUIRE(next.has_value());
    CHECK_THAT(next->Point.x, WithinAbs(0.0f, 1e-5f)); // on the wall
}