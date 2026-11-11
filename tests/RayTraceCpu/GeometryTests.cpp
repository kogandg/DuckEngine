#include "TestHelpers.h"

#include <Duck/PathTraceCpu/Geometry.h>
#include <Duck/PathTraceCpu/Optics.h>
#include <Duck/PathTraceCpu/Scene.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Duck;
using namespace Duck::PathTrace;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;

namespace
{
    constexpr float TMin = 0.001f;
    const Ray ForwardRay{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
}

TEST_CASE("Sphere hit from outside", "[geometry]")
{
    const Sphere sphere{{0.0f, 0.0f, -5.0f}, 1.0f, 3};

    const auto hit = sphere.Intersect(ForwardRay, TMin, Infinity);
    REQUIRE(hit.has_value());
    CHECK_THAT(hit->T, WithinAbs(4.0f, 1e-5f));
    CHECK_THAT(hit->Point, Vec3Near(Vec3(0.0f, 0.0f, -4.0f), 1e-5f));
    CHECK_THAT(hit->Normal, Vec3Near(Vec3(0.0f, 0.0f, 1.0f), 1e-5f));
    CHECK(hit->FrontFace);
    CHECK(hit->MaterialId == 3);
}

TEST_CASE("Sphere respects the t range", "[geometry]")
{
    const Sphere sphere{{0.0f, 0.0f, -5.0f}, 1.0f, 0};

    CHECK_FALSE(sphere.Intersect(ForwardRay, TMin, 3.0f).has_value());
    CHECK_FALSE(sphere.Intersect({{0.0f, 2.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}, TMin, Infinity).has_value());

    // starting past the near side finds the far side, seen from the inside
    const auto far = sphere.Intersect(ForwardRay, 5.0f, Infinity);
    REQUIRE(far.has_value());
    CHECK_THAT(far->T, WithinAbs(6.0f, 1e-5f));
    CHECK_FALSE(far->FrontFace);
    CHECK_THAT(far->Normal, Vec3Near(Vec3(0.0f, 0.0f, 1.0f), 1e-5f)); // flipped to face the ray
}

TEST_CASE("Sphere with negative radius is inside out", "[geometry]")
{
    // what the hollow glass ball relies on
    const Sphere sphere{{0.0f, 0.0f, -5.0f}, -1.0f, 0};
    const auto hit = sphere.Intersect(ForwardRay, TMin, Infinity);
    REQUIRE(hit.has_value());
    CHECK_THAT(hit->T, WithinAbs(4.0f, 1e-5f));
    CHECK_FALSE(hit->FrontFace);
    CHECK_THAT(hit->Normal, Vec3Near(Vec3(0.0f, 0.0f, 1.0f), 1e-5f));
}

TEST_CASE("Sphere hits are on the surface and face the ray", "[geometry]")
{
    Random random(3u);
    for (int i = 0; i < PropertySamples; i++)
    {
        const Vec3 center = 10.0f * (Vec3(random.NextFloat(), random.NextFloat(), random.NextFloat()) - 0.5f);
        const float radius = 0.5f + 1.5f * random.NextFloat();
        const Sphere sphere{center, radius, 0};

        // start outside, aim at a point inside, so it must hit
        const Vec3 origin = center + RandomUnitVector(random) * radius * (1.5f + 3.5f * random.NextFloat());
        const Vec3 aim = center + RandomUnitVector(random) * radius * 0.9f * random.NextFloat();
        const Ray ray{origin, glm::normalize(aim - origin)};

        const auto hit = sphere.Intersect(ray, TMin, Infinity);
        CAPTURE(center, radius, origin, aim);
        REQUIRE(hit.has_value());
        REQUIRE(hit->FrontFace);
        REQUIRE_THAT(glm::length(hit->Point - center), WithinAbs(radius, 1e-3f * radius));
        REQUIRE_THAT(hit->Point, Vec3Near(ray.At(hit->T), 1e-4f));
        REQUIRE_THAT(glm::length(hit->Normal), WithinAbs(1.0f, 1e-5f));
        REQUIRE(glm::dot(hit->Normal, ray.Direction) <= 0.0f);
    }
}

TEST_CASE("Rays bouncing inside a sphere stay on its surface", "[geometry]")
{
    // each bounce starts from the last hit point, so rounding carries over from one hit to the next.
    // with a normal that isn't exactly unit length the error grows about 5x per bounce
    const Sphere sphere{{0.3f, -0.2f, -3.0f}, 1.0f, 0};
    Random random(4u);
    Ray ray{sphere.Center + 0.5f * RandomUnitVector(random), RandomUnitVector(random)};
    for (int bounce = 0; bounce < 100; bounce++)
    {
        const auto hit = sphere.Intersect(ray, 0.001f, Infinity);
        CAPTURE(bounce);
        REQUIRE(hit.has_value());
        REQUIRE_THAT(glm::length(hit->Point - sphere.Center), WithinAbs(1.0f, 1e-4f));
        REQUIRE_THAT(glm::length(hit->Normal), WithinAbs(1.0f, 1e-5f));
        ray = {hit->Point, Reflect(ray.Direction, hit->Normal)};
    }
}

TEST_CASE("Quad hits inside and misses outside", "[geometry]")
{
    // unit square on the XZ plane. cross(EdgeU, EdgeV) points down, so the front is the bottom
    const Quad quad{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 2};
    const Vec3 down(0.0f, -1.0f, 0.0f);

    const auto hit = quad.Intersect({{0.25f, 1.0f, 0.75f}, down}, TMin, Infinity);
    REQUIRE(hit.has_value());
    CHECK_THAT(hit->T, WithinAbs(1.0f, 1e-6f));
    CHECK_THAT(hit->Point, Vec3Near(Vec3(0.25f, 0.0f, 0.75f), 1e-6f));
    CHECK_THAT(hit->Normal, Vec3Near(Vec3(0.0f, 1.0f, 0.0f), 1e-6f)); // flipped to face the ray
    CHECK_FALSE(hit->FrontFace);
    CHECK(hit->MaterialId == 2);

    const auto fromBelow = quad.Intersect({{0.25f, -1.0f, 0.75f}, -down}, TMin, Infinity);
    REQUIRE(fromBelow.has_value());
    CHECK(fromBelow->FrontFace);
    CHECK_THAT(fromBelow->Normal, Vec3Near(down, 1e-6f));

    CHECK_FALSE(quad.Intersect({{1.5f, 1.0f, 0.5f}, down}, TMin, Infinity).has_value());
    CHECK_FALSE(quad.Intersect({{0.5f, 1.0f, -0.5f}, down}, TMin, Infinity).has_value());
    CHECK(quad.Intersect({{1.0f, 1.0f, 0.5f}, down}, TMin, Infinity).has_value()); // edges are inclusive
    CHECK_FALSE(quad.Intersect({{0.5f, 1.0f, 0.5f}, {1.0f, 0.0f, 0.0f}}, TMin, Infinity).has_value()); // parallel
    CHECK_FALSE(quad.Intersect({{0.5f, -1.0f, 0.5f}, down}, TMin, Infinity).has_value());              // behind the ray
}

TEST_CASE("Checker alternates on the XZ plane", "[geometry]")
{
    const GltfMaterial checker{
        .BaseColor = Vec3(1.0f), .Roughness = 1.0f, .Specular = 0.0f, .CheckerColor = Vec3(0.0f), .CheckerSize = 1.0f};
    CHECK(checker.BaseColorAt({0.5f, 0.0f, 0.5f}) == Vec3(1.0f));
    CHECK(checker.BaseColorAt({1.5f, 0.0f, 0.5f}) == Vec3(0.0f));
    CHECK(checker.BaseColorAt({-0.5f, 0.0f, 0.5f}) == Vec3(0.0f)); // floor, not truncation, below zero
    CHECK(checker.BaseColorAt({0.5f, 7.3f, 0.5f}) == Vec3(1.0f));  // height doesn't matter

    const GltfMaterial plain = GltfMaterial::Lambert(Vec3(0.2f));
    CHECK(plain.BaseColorAt({1.5f, 0.0f, 0.5f}) == Vec3(0.2f));
}

TEST_CASE("Scene finds the closest hit across shape types", "[geometry]")
{
    Scene scene;
    scene.Spheres = {{{0.0f, 0.0f, -10.0f}, 1.0f, 0}};
    scene.Quads = {{{-1.0f, -1.0f, -5.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, 1}};

    const auto hit = scene.ClosestHit(ForwardRay, TMin, Infinity);
    REQUIRE(hit.has_value());
    CHECK(hit->MaterialId == 1); // the quad is in front
    CHECK_THAT(hit->T, WithinAbs(5.0f, 1e-5f));
    CHECK_FALSE(scene.ClosestHit(ForwardRay, TMin, 4.0f).has_value());

    CHECK(scene.Occluded(ForwardRay, TMin, Infinity));
    CHECK_FALSE(scene.Occluded(ForwardRay, TMin, 4.0f));
    CHECK(scene.Occluded(ForwardRay, 6.0f, Infinity)); // only the sphere is past 6

    scene.Spheres[0].Center = {0.0f, 0.0f, -3.0f}; // now the sphere is in front
    const auto nearer = scene.ClosestHit(ForwardRay, TMin, Infinity);
    REQUIRE(nearer.has_value());
    CHECK(nearer->MaterialId == 0);
    CHECK_THAT(nearer->T, WithinAbs(2.0f, 1e-5f));
}