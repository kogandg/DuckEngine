#include "TestHelpers.h"

#include <Duck/Core/Random.h>
#include <Duck/RayTraceCpu/Scene/MeshUtils.h>
#include <Duck/RayTraceCpu/Scene/RenderScene.h>
#include <Duck/RayTraceCpu/Scene/Scene.h>
#include <Duck/RayTraceCpu/Lights/DirectLighting.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <vector>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

TEST_CASE("Area lights: one per instance and emissive material", "[lights][emission]")
{
    Scene scene;
    const int gray = scene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)));
    const int lamp = scene.AddMaterial(GltfMaterial::Emissive(Vec3(3.0f)));
    const int red = scene.AddMaterial(GltfMaterial::Emissive(Vec3(1.0f, 0.0f, 0.0f)));
    scene.AddInstance(scene.AddMesh(MakeBoxMesh(Vec3(0.0f), Vec3(1.0f), lamp))); // 12 triangles, 1 light
    scene.AddInstance(scene.AddShapes({Sphere{Vec3(0.0f), 0.5f, lamp}, Sphere{Vec3(2.0f, 0.0f, 0.0f), 0.5f, gray},
                                       Quad({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, red),
                                       Sphere{Vec3(4.0f, 0.0f, 0.0f), 0.5f, lamp}}),
                      glm::translate(Mat4(1.0f), Vec3(0.0f, 5.0f, 0.0f)));
    const RenderScene built(scene);

    REQUIRE(built.GetAreaLights().size() == 3);
    const AreaLight& box = built.GetAreaLights()[0];
    CHECK(box.InstanceId == 0);
    CHECK(box.Primitives.size() == 12);
    CHECK_THAT(box.TotalArea, WithinAbs(6.0f, 1e-5f));

    const AreaLight& spheres = built.GetAreaLights()[1]; // both lamp spheres, skipping the gray one
    CHECK(spheres.InstanceId == 1);
    CHECK(spheres.MaterialId == lamp);
    CHECK(spheres.Primitives == std::vector<int>{0, 3});
    CHECK_THAT(spheres.TotalArea, WithinAbs(2.0f * Pi, 1e-4f)); // two spheres of radius 0.5

    const AreaLight& quad = built.GetAreaLights()[2];
    CHECK(quad.MaterialId == red);
    CHECK(quad.Primitives == std::vector<int>{2});
    CHECK_THAT(quad.Radiance, Vec3Near(Vec3(1.0f, 0.0f, 0.0f), 0.0f));
}

TEST_CASE("Area light samples pick each primitive in proportion to its area", "[lights][emission]")
{
    Scene scene;
    scene.AddMaterial(GltfMaterial::Emissive(Vec3(1.0f)));
    Mesh mesh; // two triangles, areas 1 and 3
    mesh.Positions = {{0, 0, 0}, {1, 0, 0}, {0, 2, 0}, {5, 0, 0}, {8, 0, 0}, {5, 2, 0}};
    mesh.Indices = {0, 1, 2, 3, 4, 5};
    scene.AddInstance(scene.AddMesh(mesh));
    const RenderScene built(scene);

    Random random(1u);
    int onBig = 0;
    constexpr int Count = 100000;
    for (int i = 0; i < Count; i++)
    {
        const AreaSample sample = built.SampleAreaLightSurface(0, {random.NextFloat(), random.NextFloat()});
        onBig += sample.Point.x >= 5.0f ? 1 : 0;
        REQUIRE_THAT(sample.Pdf, WithinRel(0.25f, 1e-5f)); // uniform over 4 units of area
    }
    CHECK_THAT(static_cast<double>(onBig) / Count, WithinAbs(0.75, 0.01));
}

TEST_CASE("Area light samples on a transformed mesh are on its surface with the right density", "[lights][emission]")
{
    const Mat4 transform = glm::translate(Mat4(1.0f), Vec3(1.0f, 2.0f, 3.0f)) *
                           glm::rotate(Mat4(1.0f), 0.6f, glm::normalize(Vec3(1.0f, 2.0f, 0.0f))) *
                           glm::scale(Mat4(1.0f), Vec3(2.0f, 0.5f, -1.3f)); // uneven, and mirrored
    const Mesh mesh = MakeUvSphereMesh(Vec3(0.0f), 1.0f, 16, 8, 0);
    Scene scene;
    scene.AddMaterial(GltfMaterial::Emissive(Vec3(1.0f)));
    scene.AddInstance(scene.AddMesh(mesh), transform);
    const RenderScene built(scene);

    Mesh baked = mesh;
    TransformMesh(baked, transform);
    float worldArea = 0.0f;
    for (int i = 0; i < baked.TriangleCount(); i++)
    {
        const auto corner = [&](int c)
        { return baked.Positions[static_cast<size_t>(baked.Indices[static_cast<size_t>(3 * i + c)])]; };
        worldArea += 0.5f * glm::length(glm::cross(corner(1) - corner(0), corner(2) - corner(0)));
    }

    Random random(2u);
    double inversePdfSum = 0.0;
    constexpr int Count = 50000;
    for (int i = 0; i < Count; i++)
    {
        const AreaSample sample = built.SampleAreaLightSurface(0, {random.NextFloat(), random.NextFloat()});
        inversePdfSum += 1.0 / sample.Pdf;
        // on the mesh, on its front: a short ray back in along the normal hits it right there, from outside
        const auto hit = built.ClosestHit({sample.Point + 0.01f * sample.Normal, -sample.Normal}, 0.0f, 0.02f);
        REQUIRE(hit.has_value());
        REQUIRE(hit->FrontFace);
        REQUIRE_THAT(hit->Point, Vec3Near(sample.Point, 1e-3f));
    }
    // 1 / pdf averages to the area for any correct pdf
    CHECK_THAT(inversePdfSum / Count, WithinRel(static_cast<double>(worldArea), 0.005));
}

TEST_CASE("Sampling one area light per point averages to the sum over all of them", "[lights][emission]")
{
    Scene scene;
    scene.Quads = {{{-5.0f, 0.0f, 5.0f},
                    {10.0f, 0.0f, 0.0f},
                    {0.0f, 0.0f, -10.0f},
                    scene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)))}};
    for (int k = 0; k < 6; k++) // six lights of different brightness, sizes and distances
    {
        const int lamp = scene.AddMaterial(GltfMaterial::Emissive(Vec3(1.0f + static_cast<float>(k))));
        const float fk = static_cast<float>(k);
        scene.Quads.push_back(
            Quad(Vec3(-3.0f + fk, 2.0f + 0.3f * fk, -1.0f), Vec3(0.5f, 0.0f, 0.0f), Vec3(0.0f, -0.1f, -0.5f), lamp));
    }
    scene.AddInstance(
        scene.AddShapes({Sphere{Vec3(0.0f), 1.0f, scene.AddMaterial(GltfMaterial::Emissive(Vec3(4.0f)))}}),
        glm::translate(Mat4(1.0f), Vec3(2.0f, 3.0f, 1.0f)) * glm::scale(Mat4(1.0f), Vec3(0.6f, 0.2f, 0.4f)));
    const RenderScene built(scene);
    REQUIRE(built.GetAreaLights().size() == 7);

    Hit hit;
    hit.Point = Vec3(0.3f, 0.0f, 0.2f);
    hit.Normal = hit.GeometricNormal = Vec3(0.0f, 1.0f, 0.0f);
    const auto brdf = [](const Vec3&) { return Vec3(0.5f * InvPi); };

    IndependentSampler one(5u);
    IndependentSampler every(6u);
    double oneSum = 0.0;
    double everySum = 0.0;
    constexpr int Count = 200000;
    for (int i = 0; i < Count; i++)
    {
        one.StartPixelSample({0, 0}, i);
        oneSum += DirectLighting(built, hit, one, brdf).x;

        every.StartPixelSample({0, 0}, i);
        for (int light = 0; light < static_cast<int>(built.GetAreaLights().size()); light++)
        {
            everySum += ShadowedContribution(built, hit, built.SampleAreaLight(light, hit.Point, every), brdf).x;
        }
    }
    // same average. picking one light is noisier (~1% standard error at this count), so allow 4 standard errors.
    // a real mistake is far bigger: forgetting to divide by the pick probability would be 7x too dark
    CHECK_THAT(oneSum / Count, WithinRel(everySum / Count, 0.04));
}

TEST_CASE("Area lights are picked in proportion to their power", "[lights][emission]")
{
    Scene scene;
    const int dim = scene.AddMaterial(GltfMaterial::Emissive(Vec3(1.0f)));
    const int bright = scene.AddMaterial(GltfMaterial::Emissive(Vec3(2.0f)));
    // radiance * area: 1 * 1 against 2 * 4, so 1 : 8
    scene.Quads = {{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, dim},
                   {{5.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 2.0f}, bright}};
    scene.AddInstance(scene.AddShapes({Quad({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, dim)}));
    const RenderScene built(scene);

    // the loose quads are one instance with two emissive materials, so two lights. the instanced copy is a third
    REQUIRE(built.GetAreaLights().size() == 3);
    // weights 1, 8 and 1 (the instanced copy of the dim quad)
    CHECK_THAT(built.AreaLightProbability(0), WithinAbs(0.1f, 1e-4f));
    CHECK_THAT(built.AreaLightProbability(1), WithinAbs(0.8f, 1e-4f));
    CHECK_THAT(built.AreaLightProbability(2), WithinAbs(0.1f, 1e-4f));

    // each light owns a slice of [0, 1) as wide as its chance
    CHECK(built.ChooseAreaLight(0.05f)->Index == 0);
    CHECK(built.ChooseAreaLight(0.5f)->Index == 1);
    CHECK(built.ChooseAreaLight(0.95f)->Index == 2);
    CHECK_THAT(built.ChooseAreaLight(0.5f)->Probability, WithinAbs(0.8f, 1e-4f));
}

TEST_CASE("An instance's scale changes its light's power", "[lights][emission]")
{
    Scene scene;
    const int lamp = scene.AddMaterial(GltfMaterial::Emissive(Vec3(1.0f)));
    const int sphere = scene.AddShapes({Sphere{Vec3(0.0f), 1.0f, lamp}});
    scene.AddInstance(sphere);
    scene.AddInstance(sphere, glm::translate(Mat4(1.0f), Vec3(5.0f, 0.0f, 0.0f)) * glm::scale(Mat4(1.0f), Vec3(2.0f)));
    const RenderScene built(scene);

    // twice the size is four times the area, so four times the light
    REQUIRE(built.GetAreaLights().size() == 2);
    CHECK_THAT(built.AreaLightProbability(0), WithinAbs(0.2f, 1e-4f));
    CHECK_THAT(built.AreaLightProbability(1), WithinAbs(0.8f, 1e-4f));
}

TEST_CASE("Even a very dim area light can still be picked", "[lights][emission]")
{
    Scene scene;
    scene.Quads = {{{0.0f, 0.0f, 0.0f},
                    {1.0f, 0.0f, 0.0f},
                    {0.0f, 0.0f, 1.0f},
                    scene.AddMaterial(GltfMaterial::Emissive(Vec3(1e-9f)))},
                   {{5.0f, 0.0f, 0.0f},
                    {1.0f, 0.0f, 0.0f},
                    {0.0f, 0.0f, 1.0f},
                    scene.AddMaterial(GltfMaterial::Emissive(Vec3(1e3f)))}};
    const RenderScene built(scene);

    // a light that's never picked is light missing from the image, so every light keeps a small share
    REQUIRE(built.GetAreaLights().size() == 2);
    CHECK(built.AreaLightProbability(0) > 0.0f);
    CHECK(built.ChooseAreaLight(0.0f)->Index == 0);
    CHECK_THAT(built.AreaLightProbability(0) + built.AreaLightProbability(1), WithinAbs(1.0f, 1e-6f));
}

TEST_CASE("AreaLightPdf matches the density light sampling picks the point with", "[lights][emission][mis]")
{
    // MIS needs both: the pdf a light sample reports, and the pdf of the same point when a ray just hits it
    Scene scene;
    const int lamp = scene.AddMaterial(GltfMaterial::Emissive(Vec3(3.0f, 2.0f, 1.0f)));
    scene.AddInstance(scene.AddShapes({Sphere{Vec3(0.0f), 0.3f, lamp}}),
                      glm::translate(Mat4(1.0f), Vec3(-0.5f, 1.5f, 0.5f)) *
                          glm::scale(Mat4(1.0f), Vec3(2.0f, 0.5f, 1.0f)));
    const Quad unit{{-0.5f, 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, lamp};
    scene.AddInstance(scene.AddShapes({unit}),
                      glm::translate(Mat4(1.0f), Vec3(0.5f, 1.2f, -0.5f)) *
                          glm::rotate(Mat4(1.0f), 2.6f, glm::normalize(Vec3(1.0f, 0.3f, 0.2f))) *
                          glm::scale(Mat4(1.0f), Vec3(0.6f, 1.0f, 0.3f)));
    const RenderScene built(scene);

    IndependentSampler sampler(3u);
    int checked = 0;
    for (int i = 0; i < 2000; i++)
    {
        sampler.StartPixelSample({i, 0}, 0);
        const Vec3 point(sampler.Get1D() * 4.0f - 2.0f, 0.01f + sampler.Get1D() * 0.5f, sampler.Get1D() * 4.0f - 2.0f);
        const auto choice = built.ChooseAreaLight(sampler.Get1D());
        REQUIRE(choice);
        const LightSample sample = built.SampleAreaLight(choice->Index, point, sampler);
        if (sample.Pdf <= 0.0f)
        {
            continue;
        }
        const auto hit = built.ClosestHit({point, sample.Direction}, 1e-6f, Infinity);
        // skip points behind the other light, and edge-on ones where 1 / cos turns float noise into big differences
        if (!hit || std::abs(hit->T - sample.Distance) > 1e-3f ||
            std::abs(glm::dot(hit->GeometricNormal, sample.Direction)) < 0.1f)
        {
            continue;
        }
        checked++;
        REQUIRE(built.AreaLightAt(*hit) == choice->Index);
        CHECK_THAT(built.AreaLightPdf(choice->Index, *hit, point), WithinRel(choice->Probability * sample.Pdf, 1e-3f));
    }
    CHECK(checked > 500);
}