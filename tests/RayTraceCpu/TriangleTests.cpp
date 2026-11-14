#include "RenderTestHelpers.h"

#include <Duck/RayTraceCpu/Geometry.h>
#include <Duck/RayTraceCpu/MeshUtils.h>
#include <Duck/RayTraceCpu/RenderScene.h>
#include <Duck/RayTraceCpu/Scene.h>
#include <Duck/RayTraceCpu/WhittedIntegrator.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <glm/gtc/matrix_transform.hpp>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;

namespace
{
    // one triangle in the plane z = -2, counter-clockwise seen from +z (where the test rays start)
    Mesh SingleTriangle()
    {
        Mesh mesh;
        mesh.Positions = {{-1.0f, -1.0f, -2.0f}, {1.0f, -1.0f, -2.0f}, {0.0f, 1.0f, -2.0f}};
        mesh.Indices = {0, 1, 2};
        mesh.MaterialId = 3;
        return mesh;
    }

    const Vec3 Forward(0.0f, 0.0f, -1.0f);
}

TEST_CASE("Triangle hit from the front", "[triangle]")
{
    const Mesh mesh = SingleTriangle();
    const Triangle triangle{&mesh, 0, mesh.MaterialId};

    const auto hit = triangle.Intersect({Vec3(0.0f), Forward}, 0.0f, Infinity);
    REQUIRE(hit.has_value());
    CHECK_THAT(hit->T, WithinAbs(2.0f, 1e-6f));
    CHECK_THAT(hit->Point, Vec3Near(Vec3(0.0f, 0.0f, -2.0f), 1e-6f));
    CHECK(hit->FrontFace);
    CHECK_THAT(hit->Normal, Vec3Near(Vec3(0.0f, 0.0f, 1.0f), 1e-6f));
    CHECK_THAT(hit->GeometricNormal, Vec3Near(Vec3(0.0f, 0.0f, 1.0f), 1e-6f));
    CHECK(hit->MaterialId == 3);
    // (0, 0) = 0.25 * p0 + 0.25 * p1 + 0.5 * p2, and without mesh uvs the uv is (b1, b2)
    CHECK(glm::all(glm::lessThan(glm::abs(hit->Uv - Vec2(0.25f, 0.5f)), Vec2(1e-6f))));
}

TEST_CASE("Triangle misses and hits from behind", "[triangle]")
{
    const Mesh mesh = SingleTriangle();
    const Triangle triangle{&mesh, 0, mesh.MaterialId};

    CHECK_FALSE(triangle.Intersect({{2.0f, 0.0f, 0.0f}, Forward}, 0.0f, Infinity).has_value());  // beside it
    CHECK_FALSE(triangle.Intersect({{0.0f, 1.1f, 0.0f}, Forward}, 0.0f, Infinity).has_value());  // above the tip
    CHECK_FALSE(triangle.Intersect({{0.0f, 0.0f, -3.0f}, Forward}, 0.0f, Infinity).has_value()); // behind the ray
    CHECK_FALSE(triangle.Intersect({{0.0f, 0.0f, -2.0f}, {1.0f, 0.0f, 0.0f}}, 0.0f, Infinity).has_value()); // parallel
    CHECK_FALSE(triangle.Intersect({Vec3(0.0f), Forward}, 0.0f, 1.0f).has_value());                         // past tMax

    const auto back = triangle.Intersect({{0.0f, 0.0f, -4.0f}, -Forward}, 0.0f, Infinity);
    REQUIRE(back.has_value());
    CHECK_FALSE(back->FrontFace);
    CHECK_THAT(back->Normal, Vec3Near(Vec3(0.0f, 0.0f, -1.0f), 1e-6f)); // flipped to face the ray
}

TEST_CASE("Triangle vertex normals and uvs are interpolated", "[triangle]")
{
    Mesh mesh = SingleTriangle();
    mesh.Normals.assign(3, glm::normalize(Vec3(1.0f, 0.0f, 1.0f)));
    mesh.Uvs = {{0.0f, 0.0f}, {2.0f, 0.0f}, {0.0f, 4.0f}};
    const Triangle triangle{&mesh, 0, mesh.MaterialId};

    const auto hit = triangle.Intersect({Vec3(0.0f), Forward}, 0.0f, Infinity);
    REQUIRE(hit.has_value());
    CHECK_THAT(hit->Normal, Vec3Near(glm::normalize(Vec3(1.0f, 0.0f, 1.0f)), 1e-6f)); // smooth
    CHECK_THAT(hit->GeometricNormal, Vec3Near(Vec3(0.0f, 0.0f, 1.0f), 1e-6f));        // the real face
    CHECK(glm::all(glm::lessThan(glm::abs(hit->Uv - Vec2(0.5f, 2.0f)), Vec2(1e-5f))));

    // vertex normals on the other side than the winding: the winding wins
    mesh.Normals.assign(3, Vec3(0.0f, 0.0f, -1.0f));
    const auto flipped = triangle.Intersect({Vec3(0.0f), Forward}, 0.0f, Infinity);
    REQUIRE(flipped.has_value());
    CHECK_THAT(flipped->Normal, Vec3Near(Vec3(0.0f, 0.0f, 1.0f), 1e-6f));
}

TEST_CASE("Triangle area and surface samples", "[triangle]")
{
    const Mesh mesh = SingleTriangle();
    const Triangle triangle{&mesh, 0, mesh.MaterialId};
    CHECK_THAT(triangle.Area(), WithinAbs(2.0f, 1e-6f)); // base 2, height 2

    Random random(81u);
    Vec3 sum(0.0f);
    constexpr int Count = 20000;
    for (int i = 0; i < Count; i++)
    {
        const float u1 = random.NextFloat();
        const float u2 = random.NextFloat();
        const SurfaceSample sample = triangle.SampleSurface({u1, u2});
        REQUIRE(triangle.Bounds().Contains(sample.Point, 1e-5f));
        REQUIRE_THAT(sample.Normal, Vec3Near(Vec3(0.0f, 0.0f, 1.0f), 1e-6f));
        // a ray from in front of the sample point must hit the triangle there
        const auto hit = triangle.Intersect({sample.Point + sample.Normal, -sample.Normal}, 0.0f, Infinity);
        REQUIRE(hit.has_value());
        sum += sample.Point;
    }
    // uniform by area: the samples average to the centroid
    CHECK_THAT(sum / static_cast<float>(Count), Vec3Near(Vec3(0.0f, -1.0f / 3.0f, -2.0f), 0.02f));
}

TEST_CASE("A mirroring transform keeps the front outside", "[triangle][mesh]")
{
    Mesh mesh = SingleTriangle();
    mesh.Normals.assign(3, glm::normalize(Vec3(1.0f, 1.0f, 0.0f)));
    TransformMesh(mesh, glm::scale(Mat4(1.0f), Vec3(-2.0f, 1.0f, 1.0f))); // mirror in x, and stretch

    const Triangle triangle{&mesh, 0, mesh.MaterialId};
    CHECK_THAT(triangle.FrontNormal(), Vec3Near(Vec3(0.0f, 0.0f, 1.0f), 1e-6f)); // still facing +z
    // inverse transpose of scale(-2, 1, 1) is scale(-1/2, 1, 1)
    CHECK_THAT(mesh.Normals[0], Vec3Near(glm::normalize(Vec3(-0.5f, 1.0f, 0.0f)), 1e-6f));
}

TEST_CASE("RenderScene splits meshes into triangles and skips degenerate ones", "[scene][mesh]")
{
    Scene scene;
    const int gray = scene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)));
    const int lamp = scene.AddMaterial(GltfMaterial::Emissive(Vec3(1.0f)));
    scene.Spheres = {{{0.0f, 0.0f, -10.0f}, 1.0f, gray}};
    // 8 x 4 globe: 64 triangles, of which the 16 touching the poles have zero area
    scene.Meshes.push_back(MakeUvSphereMesh({0.0f, 0.0f, -3.0f}, 1.0f, 8, 4, gray));
    scene.Meshes.push_back(MakeBoxMesh(Vec3(5.0f), Vec3(6.0f), lamp)); // 12 emissive triangles

    const RenderScene built(scene);
    CHECK(built.GetShapes().size() == 1 + 48 + 12);
    CHECK(built.GetLights().size() == 12); // one area light per emissive triangle

    const auto hit = built.ClosestHit({Vec3(0.0f), Forward}, 0.0f, Infinity);
    REQUIRE(hit.has_value());
    CHECK(std::holds_alternative<Triangle>(built.GetShapes()[static_cast<size_t>(hit->PrimitiveId)]));
    CHECK_THAT(hit->T, WithinAbs(2.0f, 1e-3f)); // the mesh sphere's front, not the sphere behind it
}

TEST_CASE("Whitted: a smooth mesh sphere shades like the real one", "[whitted][mesh]")
{
    // lit head-on, the point facing the camera has a vertex normal of exactly (0, 0, 1): albedo / pi * E
    Scene scene = DarkScene();
    const int gray = scene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)));
    scene.Meshes.push_back(MakeUvSphereMesh({0.0f, 0.0f, -3.0f}, 1.0f, 64, 32, gray));
    scene.Lights = {DirectionalLight{Vec3(0.0f, 0.0f, 1.0f), Vec3(Pi)}};

    WhittedIntegrator whitted;
    const Vec3 center = CenterPixel(Render(whitted, scene, LookForward(Vec3(0.0f), 1.0f)));
    CHECK_THAT(center, Vec3Near(Vec3(0.5f), 1e-3f));
}