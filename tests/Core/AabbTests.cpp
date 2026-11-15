#include "TestHelpers.h"

#include <Duck/Core/Aabb.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

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

TEST_CASE("Box surface area", "[box]")
{
    CHECK_THAT(UnitBox.SurfaceArea(), WithinAbs(24.0f, 1e-5f)); // six faces of 2 x 2
    const Aabb flat{Vec3(0.0f), Vec3(2.0f, 0.0f, 3.0f)};
    CHECK_THAT(flat.SurfaceArea(), WithinAbs(12.0f, 1e-5f)); // both sides of a 2 x 3 sheet
}

TEST_CASE("Transformed box holds every transformed corner", "[box]")
{
    const Mat4 transform = glm::translate(Mat4(1.0f), Vec3(5.0f, 0.0f, 0.0f)) *
                           glm::rotate(Mat4(1.0f), Radians(45.0f), Vec3(0.0f, 1.0f, 0.0f));
    const Aabb moved = UnitBox.Transformed(transform);

    // a unit box turned 45 degrees about y reaches sqrt 2 along x and z, and still 1 along y
    const float reach = std::sqrt(2.0f);
    CHECK_THAT(moved.Min.x, WithinAbs(5.0f - reach, 1e-5f));
    CHECK_THAT(moved.Max.x, WithinAbs(5.0f + reach, 1e-5f));
    CHECK_THAT(moved.Min.y, WithinAbs(-1.0f, 1e-5f));
    CHECK_THAT(moved.Max.y, WithinAbs(1.0f, 1e-5f));
    CHECK_THAT(moved.Max.z, WithinAbs(reach, 1e-5f));

    for (int corner = 0; corner < 8; corner++)
    {
        const Vec3 point((corner & 1) ? 1.0f : -1.0f, (corner & 2) ? 1.0f : -1.0f, (corner & 4) ? 1.0f : -1.0f);
        CHECK(moved.Contains(Vec3(transform * Vec4(point, 1.0f)), 1e-5f));
    }
}
