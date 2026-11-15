#include "TestHelpers.h"

#include <Duck/Core/Random.h>
#include <Duck/RayTraceCpu/Scene/MeshUtils.h>
#include <Duck/RayTraceCpu/Scene/RenderScene.h>
#include <Duck/RayTraceCpu/Scene/Scene.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

TEST_CASE("A sphere instanced with uneven scale is an ellipsoid", "[instance][sphere]")
{
    Scene scene;
    scene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)));
    scene.AddInstance(scene.AddShapes({Sphere{Vec3(0.0f), 1.0f, 0}}), glm::scale(Mat4(1.0f), Vec3(2.0f, 1.0f, 1.0f)));
    const RenderScene built(scene);

    // x^2 / 4 + y^2 = 1 at y = 0.5 gives x = sqrt 3. the normal is the gradient: (x / 4, y, 0)
    const auto hit = built.ClosestHit({{5.0f, 0.5f, 0.0f}, {-1.0f, 0.0f, 0.0f}}, 0.001f, Infinity);
    REQUIRE(hit.has_value());
    CHECK_THAT(hit->T, WithinAbs(5.0f - std::sqrt(3.0f), 1e-5f));
    CHECK_THAT(hit->Point, Vec3Near(Vec3(std::sqrt(3.0f), 0.5f, 0.0f), 1e-5f));
    CHECK_THAT(hit->Normal, Vec3Near(glm::normalize(Vec3(std::sqrt(3.0f) / 4.0f, 0.5f, 0.0f)), 1e-5f));
    CHECK(hit->FrontFace);
    CHECK(hit->InstanceId == 0);
    CHECK(hit->GeometryId == 1);
}

TEST_CASE("An instanced quad is hit like the same quad built in world space", "[instance][quad]")
{
    const Mat4 transform = glm::translate(Mat4(1.0f), Vec3(0.0f, 1.0f, -3.0f)) *
                           glm::rotate(Mat4(1.0f), 0.8f, glm::normalize(Vec3(1.0f, 1.0f, 0.0f))) *
                           glm::scale(Mat4(1.0f), Vec3(1.0f, 2.0f, 0.5f));
    const Quad quad(Vec3(-0.5f, 0.0f, -0.5f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), 0);

    Scene instancedScene;
    instancedScene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)));
    instancedScene.AddInstance(instancedScene.AddShapes({quad}), transform);
    Scene worldScene;
    worldScene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)));
    // a parallelogram stays one under any affine transform: move its corner as a point and its edges as vectors
    worldScene.Quads = {
        Quad(Vec3(transform * Vec4(quad.Corner, 1.0f)), Mat3(transform) * quad.EdgeU, Mat3(transform) * quad.EdgeV, 0)};
    const RenderScene instanced(instancedScene);
    const RenderScene world(worldScene);

    Random random(3u);
    int hits = 0;
    for (int i = 0; i < 5000; i++)
    {
        const Vec3 origin(4.0f * random.NextFloat() - 2.0f, 4.0f * random.NextFloat(), 2.0f);
        const Vec3 target(2.0f * random.NextFloat() - 1.0f, 2.0f * random.NextFloat(), -3.0f);
        const Ray ray{origin, glm::normalize(target - origin)};
        CAPTURE(i);
        const auto a = instanced.ClosestHit(ray, 0.001f, Infinity);
        const auto b = world.ClosestHit(ray, 0.001f, Infinity);
        REQUIRE(a.has_value() == b.has_value());
        if (a)
        {
            hits++;
            REQUIRE_THAT(a->T, WithinAbs(b->T, 1e-4f));
            REQUIRE_THAT(a->Normal, Vec3Near(b->Normal, 1e-4f));
            REQUIRE(a->FrontFace == b->FrontFace);
        }
    }
    CHECK(hits > 300);
}

TEST_CASE("An area light on an ellipsoid samples its surface with the right density", "[instance][emission]")
{
    const Mat4 transform = glm::translate(Mat4(1.0f), Vec3(0.0f, 3.0f, 0.0f)) *
                           glm::rotate(Mat4(1.0f), 0.5f, Vec3(0.0f, 0.0f, 1.0f)) *
                           glm::scale(Mat4(1.0f), Vec3(2.0f, 0.5f, 1.0f));
    Scene scene;
    scene.AddMaterial(GltfMaterial::Emissive(Vec3(1.0f)));
    scene.AddInstance(scene.AddShapes({Sphere{Vec3(0.0f), 1.0f, 0}}), transform);
    const RenderScene built(scene);
    REQUIRE(built.GetAreaLights().size() == 1);

    // reference area: a finely tessellated copy of the ellipsoid
    Mesh reference = MakeUvSphereMesh(Vec3(0.0f), 1.0f, 256, 128, 0);
    TransformMesh(reference, transform);
    float referenceArea = 0.0f;
    for (int i = 0; i < reference.TriangleCount(); i++)
    {
        const auto corner = [&](int c)
        { return reference.Positions[static_cast<size_t>(reference.Indices[static_cast<size_t>(3 * i + c)])]; };
        referenceArea += 0.5f * glm::length(glm::cross(corner(1) - corner(0), corner(2) - corner(0)));
    }

    // 1 / pdf averages to the area for any correct pdf, however uneven the sampling is
    const Mat4 toObject = glm::inverse(transform);
    Random random(9u);
    double sum = 0.0;
    constexpr int Count = 100000;
    for (int i = 0; i < Count; i++)
    {
        const AreaSample sample = built.SampleAreaLightSurface(0, {random.NextFloat(), random.NextFloat()});
        sum += 1.0 / sample.Pdf;
        // on the ellipsoid, facing out of it
        const Vec3 object(toObject * Vec4(sample.Point, 1.0f));
        REQUIRE_THAT(glm::length(object), WithinAbs(1.0f, 1e-5f));
        REQUIRE_THAT(sample.Normal, Vec3Near(glm::normalize(glm::transpose(Mat3(toObject)) * object), 1e-4f));
    }
    CHECK_THAT(sum / Count, WithinRel(static_cast<double>(referenceArea), 0.005));
}

TEST_CASE("A material override replaces every material of its instance", "[instance][material]")
{
    Scene scene;
    const int gray = scene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)));
    const int red = scene.AddMaterial(GltfMaterial::Lambert(Vec3(0.8f, 0.1f, 0.1f)));
    const int lamp = scene.AddMaterial(GltfMaterial::Emissive(Vec3(3.0f)));
    const int box = scene.AddMesh(MakeBoxMesh(Vec3(-0.5f), Vec3(0.5f), gray));
    scene.AddInstance(box, glm::translate(Mat4(1.0f), Vec3(-2.0f, 0.0f, -5.0f)));
    scene.AddInstance(box, glm::translate(Mat4(1.0f), Vec3(0.0f, 0.0f, -5.0f)), red);
    scene.AddInstance(box, glm::translate(Mat4(1.0f), Vec3(2.0f, 0.0f, -5.0f)), lamp);
    const RenderScene built(scene);

    const auto materialAt = [&](float x)
    {
        const auto hit = built.ClosestHit({{x, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}, 0.001f, Infinity);
        REQUIRE(hit.has_value());
        return hit->MaterialId;
    };
    CHECK(materialAt(-2.0f) == gray);
    CHECK(materialAt(0.0f) == red);
    CHECK(materialAt(2.0f) == lamp);

    // only the instance overridden with an emissive material gives off light: one light for its 12 triangles
    REQUIRE(built.GetAreaLights().size() == 1);
    CHECK(built.GetAreaLights()[0].InstanceId == 2);
    CHECK(built.GetAreaLights()[0].Primitives.size() == 12);
}
