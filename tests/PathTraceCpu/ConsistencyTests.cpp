#include "RenderTestHelpers.h"

#include <Duck/PathTraceCpu/PathTracer.h>
#include <Duck/PathTraceCpu/WhittedIntegrator.h>

#include <catch2/catch_test_macros.hpp>

using namespace Duck;
using namespace Duck::PathTrace;
using namespace Duck::Test;

TEST_CASE("Whitted ambient matches path-traced sky light on an open floor", "[consistency]")
{
    // the floor can't see itself, so every path-traced bounce reaches the sky: exactly albedo * L, no noise.
    // Whitted's ambient is the same light without occlusion, so with Ambient = sky the two must agree
    const Vec3 sky(0.8f, 0.9f, 1.0f);
    const float albedo = 0.6f;

    Scene scene = DarkScene();
    scene.SkyDown = scene.SkyUp = sky;
    scene.AmbientDown = scene.AmbientUp = sky;
    const int gray = scene.AddMaterial({.Albedo = Vec3(albedo)});
    scene.Quads = {Floor(gray)};
    const Camera camera = LookDown({0.0f, 1.0f, 0.0f}, 30.0f);

    WhittedIntegrator whitted;
    PathTracer pathTracer;
    CheckAllPixels(Render(whitted, scene, camera), albedo * sky, 1e-5f);
    CheckAllPixels(Render(pathTracer, scene, camera, 9, 4), albedo * sky, 1e-5f);
}