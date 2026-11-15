#include "TestHelpers.h"

#include <Duck/Core/Aabb.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using namespace Duck;
using Catch::Matchers::WithinAbs;

namespace
{
    const Aabb UnitBox{Vec3(-1.0f), Vec3(1.0f)};
    const Vec3 Forward(0.0f, 0.0f, -1.0f);

    float Enter(const Aabb& box, const Vec3& origin, const Vec3& direction, float tMin = 0.0f, float tMax = Infinity)
    {
        return box.EntryDistance(TraversalRay(Ray{origin, direction}), tMin, tMax);
    }
}

TEST_CASE("Box hit from outside gives the entry distance", "[box]")
{
    CHECK_THAT(Enter(UnitBox, {0.0f, 0.0f, 5.0f}, Forward), WithinAbs(4.0f, 1e-6f));
    // from (3, 3, 3) toward the origin: enters at the corner (1, 1, 1), 2 sqrt 3 away
    CHECK_THAT(Enter(UnitBox, Vec3(3.0f), glm::normalize(Vec3(-1.0f))), WithinAbs(2.0f * std::sqrt(3.0f), 1e-5f));
}

TEST_CASE("Box misses", "[box]")
{
    CHECK(Enter(UnitBox, {2.0f, 0.0f, 5.0f}, Forward) == Infinity);                          // beside it
    CHECK(Enter(UnitBox, {0.0f, 0.0f, 5.0f}, -Forward) == Infinity);                         // behind the ray
    CHECK(Enter(UnitBox, Vec3(3.0f), glm::normalize(Vec3(-1.0f, -1.0f, 1.0f))) == Infinity); // diagonal past it
}

TEST_CASE("Box respects the t range", "[box]")
{
    CHECK(Enter(UnitBox, {0.0f, 0.0f, 5.0f}, Forward, 0.0f, 3.0f) == Infinity); // ends before the box
    CHECK(Enter(UnitBox, {0.0f, 0.0f, 5.0f}, Forward, 7.0f) == Infinity);       // starts after it
    // a ray starting inside enters immediately
    CHECK(Enter(UnitBox, Vec3(0.0f), Forward, 0.001f) == 0.001f);
}

TEST_CASE("Box with rays parallel to a slab", "[box]")
{
    // direction x = 0, so 1 / x is infinite: only the origin's x decides that slab
    CHECK_THAT(Enter(UnitBox, {0.5f, 0.0f, 5.0f}, Forward), WithinAbs(4.0f, 1e-6f));
    CHECK(Enter(UnitBox, {1.5f, 0.0f, 5.0f}, Forward) == Infinity);
}

TEST_CASE("Flat boxes can be hit", "[box]")
{
    // zero thickness in y, like the box around a floor quad: entry and exit are the same t
    const Aabb flat{Vec3(-1.0f, 0.0f, -1.0f), Vec3(1.0f, 0.0f, 1.0f)};
    CHECK_THAT(Enter(flat, {0.2f, 2.0f, 0.3f}, {0.0f, -1.0f, 0.0f}), WithinAbs(2.0f, 1e-6f));
    CHECK(Enter(flat, {0.0f, 2.0f, 0.0f}, glm::normalize(Vec3(0.3f, -1.0f, 0.2f))) < Infinity);
}