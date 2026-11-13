#include "RenderTestHelpers.h"

#include <Duck/RayTraceCpu/PathTracer.h>
#include <Duck/RayTraceCpu/WhittedIntegrator.h>
#include <Duck/Core/ThreadPool.h>
#include <Duck/RayTraceCpu/RenderScene.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;

TEST_CASE("Path tracer furnace: a diffuse ball shows its albedo exactly", "[pathtracer][furnace]")
{
    // every bounce off a convex ball goes straight to the uniform sky, so each sample is exactly albedo * 1
    const float albedo = GENERATE(0.25f, 0.5f, 1.0f);
    CAPTURE(albedo);

    Scene scene = FurnaceScene();
    const int diffuse = scene.AddMaterial(GltfMaterial::Lambert(Vec3(albedo)));
    scene.Spheres = {{{0.0f, 0.0f, -3.0f}, 1.0f, diffuse}};

    PathTracer pathTracer;
    // narrow view filled by the ball
    CheckAllPixels(Render(pathTracer, scene, LookForward(Vec3(0.0f), 10.0f), 9, 4), Vec3(albedo), 1e-5f);
}

TEST_CASE("Path tracer furnace: smooth glass and metal vanish", "[pathtracer][furnace][material]")
{
    // each choice's probability equals its weight and the tints are white, so every sample is exactly 1
    const GltfMaterial material = GENERATE(GltfMaterial::Glass(1.5f), GltfMaterial::Metal(Vec3(1.0f)));
    Scene scene = FurnaceScene();
    scene.Spheres = {{{0.0f, 0.0f, -3.0f}, 1.0f, scene.AddMaterial(material)}};

    PathTracer pathTracer;
    CheckAllPixels(Render(pathTracer, scene, LookForward(Vec3(0.0f), 10.0f), 9, 4), Vec3(1.0f), 1e-5f);
}

TEST_CASE("Path tracer: emissive surfaces show their radiance", "[pathtracer][material]")
{
    Scene scene = DarkScene();
    scene.Quads = {Floor(scene.AddMaterial(GltfMaterial::Emissive(Vec3(2.0f, 1.0f, 0.5f))))};
    PathTracer pathTracer;
    CheckAllPixels(Render(pathTracer, scene, LookDown({0.0f, 1.0f, 0.0f}, 30.0f)), Vec3(2.0f, 1.0f, 0.5f), 1e-6f);
}

TEST_CASE("Renders don't depend on the number of threads", "[threading]")
{
    // per-pixel seeding (D-016): every thread count must give bit-identical images
    Scene scene = FurnaceScene();
    scene.Spheres = {{{0.0f, 0.0f, -3.0f}, 1.0f, scene.AddMaterial(GltfMaterial::Lambert(Vec3(0.5f)))}};
    const RenderScene built(scene);
    const Camera camera = LookForward(Vec3(0.0f), 45.0f);
    const IndependentSampler sampler;
    PathTracer pathTracer;

    ThreadPool single(1);
    ThreadPool many(8);
    Film a(33, 33);
    Film b(33, 33);
    for (int pass = 0; pass < 4; pass++)
    {
        pathTracer.RenderPass(built, camera, a, single, sampler);
        a.Passes++;
        pathTracer.RenderPass(built, camera, b, many, sampler);
        b.Passes++;
    }

    int different = 0;
    for (int y = 0; y < 33; y++)
    {
        for (int x = 0; x < 33; x++)
        {
            different += (a.Sum[x, y] == b.Sum[x, y]) ? 0 : 1;
        }
    }
    CHECK(different == 0);
}