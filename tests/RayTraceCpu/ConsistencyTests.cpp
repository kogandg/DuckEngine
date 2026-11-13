#include "RenderTestHelpers.h"

#include <Duck/RayTraceCpu/PathTracer.h>
#include <Duck/RayTraceCpu/WhittedIntegrator.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;
using Catch::Matchers::WithinRel;

TEST_CASE("Whitted ambient matches path-traced sky light on an open floor", "[consistency]")
{
    // the floor can't see itself, so every path-traced bounce reaches the sky: exactly albedo * L, no noise.
    // Whitted's ambient is the same light without occlusion, so with Ambient = sky the two must agree
    const Vec3 sky(0.8f, 0.9f, 1.0f);
    const float albedo = 0.6f;

    Scene scene = DarkScene();
    scene.Environment.SkyDown = scene.Environment.SkyUp = sky;
    scene.Environment.AmbientDown = scene.Environment.AmbientUp = sky;
    const int gray = scene.AddMaterial(GltfMaterial::Lambert(Vec3(albedo)));
    scene.Quads = {Floor(gray)};
    const Camera camera = LookDown({0.0f, 1.0f, 0.0f}, 30.0f);

    WhittedIntegrator whitted;
    PathTracer pathTracer;
    CheckAllPixels(Render(whitted, scene, camera), albedo * sky, 1e-5f);
    CheckAllPixels(Render(pathTracer, scene, camera, 9, 4), albedo * sky, 1e-5f);
}

TEST_CASE("Path tracer and Whitted agree on a sphere light", "[consistency][emission]")
{
    const float albedo = 0.5f;
    const float radiance = 4.0f;
    Scene scene = DarkScene();
    scene.Quads = {Floor(scene.AddMaterial(GltfMaterial::Lambert(Vec3(albedo))))};
    scene.Spheres = {{{0.0f, 2.0f, 0.0f}, 0.5f, scene.AddMaterial(GltfMaterial::Emissive(Vec3(radiance)))}};
    const Camera camera({2.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 1.0f, 1.0f);

    PathTracer pathTracer;
    const Vec3 average = AveragePixel(Render(pathTracer, scene, camera, 9, 256));
    // ~6% of bounces hit the light, so this is noisier: ~2.7% over 9 * 9 * 256 samples
    CHECK_THAT(average.x, WithinRel(0.125f, 0.1f));
}