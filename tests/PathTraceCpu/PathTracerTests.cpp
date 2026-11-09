#include "RenderTestHelpers.h"

#include <Duck/PathTraceCpu/PathTracer.h>
#include <Duck/PathTraceCpu/WhittedIntegrator.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace Duck;
using namespace Duck::PathTrace;
using namespace Duck::Test;

TEST_CASE("Path tracer furnace: a diffuse ball shows its albedo exactly", "[pathtracer][furnace]")
{
    // every bounce off a convex ball goes straight to the uniform sky, so each sample is exactly albedo * 1
    const float albedo = GENERATE(0.25f, 0.5f, 1.0f);
    CAPTURE(albedo);

    Scene scene = FurnaceScene();
    const int diffuse = scene.AddMaterial({.Albedo = Vec3(albedo)});
    scene.Spheres = {{{0.0f, 0.0f, -3.0f}, 1.0f, diffuse}};

    PathTracer pathTracer;
    // narrow view filled by the ball
    CheckAllPixels(Render(pathTracer, scene, LookForward(Vec3(0.0f), 10.0f), 9, 4), Vec3(albedo), 1e-5f);
}