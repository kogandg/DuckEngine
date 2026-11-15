#include "TestHelpers.h"

#include <Duck/Core/Random.h>
#include <Duck/RayTraceCpu/Scene/MeshUtils.h>
#include <Duck/RayTraceCpu/Scene/RenderScene.h>
#include <Duck/RayTraceCpu/Scene/Scene.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <utility>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
    struct Placement
    {
        const char* Name;
        Mat4 Transform;
        bool UniformScale; // only then do smooth normals match baking exactly (see the test)
    };

    // the same mesh twice: once placed by an instance, once with the transform baked into its vertices
    std::pair<Scene, Scene> InstancedAndBaked(const Mesh& mesh, const Mat4& transform, const Material& material)
    {
        Scene instanced;
        instanced.AddMaterial(material);
        instanced.AddInstance(instanced.AddMesh(mesh), transform);

        Scene baked;
        baked.AddMaterial(material);
        Mesh copy = mesh;
        TransformMesh(copy, transform);
        baked.AddInstance(baked.AddMesh(std::move(copy)));
        return {std::move(instanced), std::move(baked)};
    }
}

TEST_CASE("An instanced mesh is hit exactly where the baked mesh is", "[instance]")
{
    const Placement placement =
        GENERATE(Placement{"rigid",
                           glm::translate(Mat4(1.0f), Vec3(1.0f, 2.0f, -5.0f)) *
                               glm::rotate(Mat4(1.0f), 0.7f, glm::normalize(Vec3(1.0f, 2.0f, 3.0f))),
                           true},
                 Placement{"uniform scale", glm::scale(Mat4(1.0f), Vec3(2.5f)), true},
                 Placement{"mirrored",
                           glm::rotate(Mat4(1.0f), 0.4f, Vec3(0.0f, 1.0f, 0.0f)) *
                               glm::scale(Mat4(1.0f), Vec3(-1.0f, 1.0f, 1.0f)),
                           true},
                 Placement{"non-uniform scale", glm::scale(Mat4(1.0f), Vec3(2.0f, 0.5f, 1.3f)), false});
    CAPTURE(placement.Name);

    const auto [instancedScene, bakedScene] =
        InstancedAndBaked(MakeUvSphereMesh(Vec3(0.2f, 0.1f, 0.0f), 1.0f, 32, 16, 0), placement.Transform,
                          GltfMaterial::Lambert(Vec3(0.5f)));
    const RenderScene instanced(instancedScene);
    const RenderScene baked(bakedScene);

    const Vec3 center(placement.Transform * Vec4(0.2f, 0.1f, 0.0f, 1.0f));
    Random random(7u);
    int hits = 0;
    for (int i = 0; i < 3000; i++)
    {
        // from in front, and every third one from inside the sphere
        Vec3 origin = center + Vec3(8.0f * random.NextFloat() - 4.0f, 8.0f * random.NextFloat() - 4.0f, 6.0f);
        if (i % 3 == 0)
        {
            origin = center;
        }
        const Vec3 target = center + 3.0f * (Vec3(random.NextFloat(), random.NextFloat(), random.NextFloat()) - 0.5f);
        const Ray ray{origin, glm::normalize(target - origin)};
        CAPTURE(i, ray.Origin, ray.Direction);

        const auto a = instanced.ClosestHit(ray, 0.001f, Infinity);
        const auto b = baked.ClosestHit(ray, 0.001f, Infinity);
        REQUIRE(a.has_value() == b.has_value());
        REQUIRE(instanced.Occluded(ray, 0.001f, Infinity) == baked.Occluded(ray, 0.001f, Infinity));
        if (!a)
        {
            continue;
        }
        hits++;

        REQUIRE_THAT(a->T, WithinAbs(b->T, 1e-4f * std::max(1.0f, b->T)));
        REQUIRE_THAT(a->Point, Vec3Near(b->Point, 1e-4f));
        REQUIRE_THAT(a->GeometricNormal, Vec3Near(b->GeometricNormal, 1e-3f));
        REQUIRE(a->FrontFace == b->FrontFace);
        REQUIRE(a->PrimitiveId == b->PrimitiveId);
        REQUIRE(a->InstanceId == 0);
        REQUIRE(a->GeometryId == 1); // geometry 0 is the (empty) loose shapes
        if (placement.UniformScale)
        {
            // under non-uniform scale they legitimately differ by up to ~2 degrees: baking normalizes each vertex
            // normal after transforming it, an instance interpolates first and transforms after
            REQUIRE_THAT(a->Normal, Vec3Near(b->Normal, 1e-3f));
        }
    }
    CHECK(hits > 1000);
}

TEST_CASE("Instances share one geometry and are told apart by their id", "[instance]")
{
    Scene scene;
    scene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)));
    const int box = scene.AddMesh(MakeBoxMesh(Vec3(-0.5f), Vec3(0.5f), 0));
    scene.AddInstance(box, glm::translate(Mat4(1.0f), Vec3(-2.0f, 0.0f, -5.0f)));
    scene.AddInstance(box, glm::translate(Mat4(1.0f), Vec3(2.0f, 0.0f, -5.0f)));

    const RenderScene built(scene);
    CHECK(built.GetGeometries().size() == 2); // the empty loose shapes, and the box once
    CHECK(built.GetInstances().size() == 2);  // no loose shapes, so no instance for them

    const auto hit = built.ClosestHit({{2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}, 0.001f, Infinity);
    REQUIRE(hit.has_value());
    CHECK(hit->InstanceId == 1);
    CHECK(hit->GeometryId == 1);
    CHECK_THAT(hit->T, WithinAbs(4.5f, 1e-5f));
    CHECK_THAT(hit->Point, Vec3Near(Vec3(2.0f, 0.0f, -4.5f), 1e-5f));
}

TEST_CASE("A light on a mirrored instance emits from the same side as the baked mesh", "[instance][emission]")
{
    const Mat4 mirrored =
        glm::translate(Mat4(1.0f), Vec3(-1.0f, 0.0f, -4.0f)) * glm::scale(Mat4(1.0f), Vec3(-1.5f, 1.5f, 1.5f));
    const auto [instancedScene, bakedScene] =
        InstancedAndBaked(MakeBoxMesh(Vec3(-0.5f), Vec3(0.5f), 0), mirrored, GltfMaterial::Emissive(Vec3(2.0f)));
    const RenderScene instanced(instancedScene);
    const RenderScene baked(bakedScene);

    REQUIRE(instanced.GetAreaLights().size() == 1);
    REQUIRE(baked.GetAreaLights().size() == 1);
    CHECK(instanced.GetAreaLights()[0].InstanceId == 0);
    CHECK(instanced.GetAreaLights()[0].Primitives.size() == 12);

    // the same u picks the same triangle on both. the point inside it can differ (baking swaps two corners of
    // each mirrored triangle), but the side it faces and the density can't
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            const Vec2 u((static_cast<float>(x) + 0.5f) / 8.0f, (static_cast<float>(y) + 0.5f) / 8.0f);
            CAPTURE(x, y);
            const AreaSample a = instanced.SampleAreaLightSurface(0, u);
            const AreaSample b = baked.SampleAreaLightSurface(0, u);
            CHECK_THAT(a.Normal, Vec3Near(b.Normal, 1e-5f));
            CHECK_THAT(a.Pdf, WithinRel(b.Pdf, 1e-5f)); // 1 / 13.5: six faces of 1.5 x 1.5
        }
    }
}
