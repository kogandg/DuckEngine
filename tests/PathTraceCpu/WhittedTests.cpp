#include "RenderTestHelpers.h"

#include <Duck/PathTraceCpu/WhittedIntegrator.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <tuple>
#include <algorithm>

using namespace Duck;
using namespace Duck::PathTrace;
using namespace Duck::Test;
using Catch::Generators::table;
using Catch::Matchers::WithinAbs;

// ---------- sky and local shading ----------

TEST_CASE("Whitted: rays that miss see the sky", "[whitted]")
{
    Scene scene = DarkScene();
    scene.SkyDown = Vec3(1.0f, 0.0f, 0.0f);
    scene.SkyUp = Vec3(0.0f, 0.0f, 1.0f);
    WhittedIntegrator whitted;

    CheckAllPixels(Render(whitted, scene, LookUp(Vec3(0.0f), 1.0f)), scene.SkyUp, 1e-3f);
    CheckAllPixels(Render(whitted, scene, LookDown(Vec3(0.0f), 1.0f)), scene.SkyDown, 1e-3f);
    CheckAllPixels(Render(whitted, scene, LookForward(Vec3(0.0f), 1.0f)), Vec3(0.5f, 0.0f, 0.5f), 1e-2f);
}

TEST_CASE("Whitted: a diffuse floor follows Lambert's cosine law", "[whitted]")
{
    const float angle = GENERATE(0.0f, 30.0f, 60.0f, 85.0f); // sun angle from straight up
    CAPTURE(angle);

    Scene scene = DarkScene();
    const float albedo = 0.5f;
    const Vec3 strength(0.8f);
    const int gray = scene.AddMaterial({.Albedo = Vec3(albedo)});
    scene.Quads = {Floor(gray)};
    scene.DirectionalLights = {{Vec3(std::sin(Radians(angle)), std::cos(Radians(angle)), 0.0f), strength}};

    WhittedIntegrator whitted;
    // rho * E * cos(theta), with the 1/pi folded into E (see P-006)
    CheckAllPixels(Render(whitted, scene, LookDown({0.0f, 1.0f, 0.0f}, 30.0f)),
                   albedo * strength * std::cos(Radians(angle)), 1e-5f);
}

TEST_CASE("Whitted: light doesn't leak through a surface", "[whitted]")
{
    Scene scene = DarkScene();
    const int gray = scene.AddMaterial({.Albedo = Vec3(0.5f)});
    scene.Quads = {Floor(gray)};
    scene.DirectionalLights = {{Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f)}};

    WhittedIntegrator whitted;
    // seen from below, the normal faces down, away from the light above
    CheckAllPixels(Render(whitted, scene, LookUp({0.0f, -1.0f, 0.0f}, 30.0f)), Vec3(0.0f), 0.0f);
}

TEST_CASE("Whitted: ambient is blended by the normal's direction", "[whitted]")
{
    Scene scene = DarkScene();
    scene.AmbientUp = Vec3(0.2f, 0.4f, 0.6f);
    scene.AmbientDown = Vec3(0.6f, 0.4f, 0.2f);
    const int gray = scene.AddMaterial({.Albedo = Vec3(0.5f)});
    scene.Quads = {
        Floor(gray),
        {{-50.0f, -50.0f, -2.0f}, {100.0f, 0.0f, 0.0f}, {0.0f, 100.0f, 0.0f}, gray}, // wall facing +Z
    };

    WhittedIntegrator whitted;
    CheckAllPixels(Render(whitted, scene, LookDown({0.0f, 1.0f, 0.0f}, 30.0f)), 0.5f * scene.AmbientUp, 1e-5f);
    CheckAllPixels(Render(whitted, scene, LookUp({0.0f, -1.0f, 0.0f}, 30.0f)), 0.5f * scene.AmbientDown, 1e-5f);
    CheckAllPixels(Render(whitted, scene, LookForward({0.0f, 1.0f, 0.0f}, 30.0f)),
                   0.25f * (scene.AmbientUp + scene.AmbientDown), 1e-5f); // sideways normal: halfway
}

// ---------- shadows and lights ----------

TEST_CASE("Whitted: blockers shadow a directional light", "[whitted]")
{
    Scene scene = DarkScene();
    const int gray = scene.AddMaterial({.Albedo = Vec3(0.5f)});
    scene.Quads = {Floor(gray)};
    const Vec3 toSun = glm::normalize(Vec3(1.0f, 1.0f, 0.0f));
    scene.DirectionalLights = {{toSun, Vec3(1.0f)}};

    const Camera camera = LookDown({0.0f, 1.0f, 0.0f}, 5.0f);
    WhittedIntegrator whitted;
    CheckAllPixels(Render(whitted, scene, camera), Vec3(0.5f * toSun.y), 1e-5f);

    // a ball on the line from the viewed spot toward the sun, outside the camera's view
    scene.Spheres = {{{2.0f, 2.0f, 0.0f}, 0.5f, gray}};
    CheckAllPixels(Render(whitted, scene, camera), Vec3(0.0f), 0.0f);
}

TEST_CASE("Whitted: point lights fall off with the inverse square law", "[whitted]")
{
    // light position, and cos(theta) / r^2 at the spot straight below the camera
    const auto [light, cosineOverDistanceSquared] = GENERATE(table<Vec3, float>({
        {Vec3(0.0f, 1.0f, 0.0f), 1.0f},
        {Vec3(0.0f, 2.0f, 0.0f), 1.0f / 4.0f},
        {Vec3(0.0f, 4.0f, 0.0f), 1.0f / 16.0f},
        {Vec3(1.0f, 1.0f, 0.0f), 0.35355339f}, // cos 45 / 2
    }));
    CAPTURE(light);

    Scene scene = DarkScene();
    const float albedo = 0.5f;
    const Vec3 intensity(2.0f);
    const int gray = scene.AddMaterial({.Albedo = Vec3(albedo)});
    scene.Quads = {Floor(gray)};
    scene.PointLights = {{light, intensity}};

    // very narrow view so the center pixel sees one spot, averaged over a few jittered passes
    WhittedIntegrator whitted;
    const Vec3 center = CenterPixel(Render(whitted, scene, LookDown({0.0f, 5.0f, 0.0f}, 0.25f), 9, 8));
    CHECK_THAT(center, Vec3Near(albedo * intensity * cosineOverDistanceSquared, 1e-3f));
}

TEST_CASE("Whitted: point light shadows stop at the light", "[whitted]")
{
    Scene scene = DarkScene();
    const int gray = scene.AddMaterial({.Albedo = Vec3(0.5f)});
    scene.Quads = {Floor(gray)};
    scene.PointLights = {{Vec3(1.0f, 1.0f, 0.0f), Vec3(1.0f)}};
    const Camera camera = LookDown({0.0f, 1.0f, 0.0f}, 0.25f);
    WhittedIntegrator whitted;

    // behind the light: must not block
    scene.Spheres = {{{2.0f, 2.0f, 0.0f}, 0.5f, gray}};
    CHECK(CenterPixel(Render(whitted, scene, camera)).x > 0.1f);

    // between the floor and the light: blocks
    scene.Spheres = {{{0.5f, 0.5f, 0.0f}, 0.2f, gray}};
    CHECK(CenterPixel(Render(whitted, scene, camera)) == Vec3(0.0f));
}

// ---------- mirrors ----------

TEST_CASE("Whitted furnace: a mirror only tints what it reflects", "[whitted][furnace]")
{
    const Vec3 tint = GENERATE(Vec3(1.0f), Vec3(0.9f, 0.5f, 0.2f));
    Scene scene = FurnaceScene();
    const int mirror = scene.AddMaterial({.Type = MaterialType::Mirror, .Albedo = tint});
    scene.Spheres = {{{0.0f, 0.0f, -3.0f}, 1.0f, mirror}};

    WhittedIntegrator whitted;
    // narrow view filled by the ball. every reflection escapes straight to the sky
    CheckAllPixels(Render(whitted, scene, LookForward(Vec3(0.0f), 10.0f)), tint, 1e-5f);
}

TEST_CASE("Whitted: a mirror reflects in the mirror direction", "[whitted]")
{
    Scene scene = DarkScene();
    scene.SkyDown = Vec3(1.0f, 0.0f, 0.0f);
    scene.SkyUp = Vec3(0.0f, 0.0f, 1.0f);
    const int mirror = scene.AddMaterial({.Type = MaterialType::Mirror, .Albedo = Vec3(1.0f)});
    // tilted 45 degrees and centered 3 units ahead, so a level ray reflects straight up
    scene.Quads = {{{-1.0f, -1.0f, -2.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 2.0f, -2.0f}, mirror}};

    WhittedIntegrator whitted;
    // reflecting the wrong way would show the red SkyDown
    CheckAllPixels(Render(whitted, scene, LookForward(Vec3(0.0f), 1.0f)), scene.SkyUp, 1e-3f);
}

TEST_CASE("Whitted: recursion stops at max depth", "[whitted]")
{
    Scene scene = FurnaceScene();
    const int mirror = scene.AddMaterial({.Type = MaterialType::Mirror, .Albedo = Vec3(1.0f)});
    // two facing mirrors with the camera between: no ray ever escapes
    scene.Quads = {
        {{-10.0f, -10.0f, -1.0f}, {20.0f, 0.0f, 0.0f}, {0.0f, 20.0f, 0.0f}, mirror},
        {{-10.0f, -10.0f, 1.0f}, {20.0f, 0.0f, 0.0f}, {0.0f, 20.0f, 0.0f}, mirror},
    };

    WhittedIntegrator whitted;
    // the sky is 1, so anything but 0 means a path escaped. past maxDepth the result is black by design
    CheckAllPixels(Render(whitted, scene, LookForward(Vec3(0.0f), 1.0f)), Vec3(0.0f), 0.0f);
}

// ---------- glass ----------

TEST_CASE("Whitted: a glass slab seen head-on reflects 2R / (1 + R)", "[whitted][glass]")
{
    // white above, black below. the camera looks down, so only reflected light comes back
    Scene scene = DarkScene();
    scene.SkyUp = Vec3(1.0f);
    const int glass = scene.AddMaterial({.Type = MaterialType::Dielectric, .Ior = 1.5f});
    // a glass quad's front, cross(EdgeU, EdgeV), must face OUT of the glass: FrontFace decides entering vs leaving
    scene.Quads = {
        {{-10.0f, 0.0f, -10.0f}, {0.0f, 0.0f, 20.0f}, {20.0f, 0.0f, 0.0f}, glass},  // top, front faces up
        {{-10.0f, -1.0f, -10.0f}, {20.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 20.0f}, glass}, // bottom, front faces down
    };

    WhittedIntegrator whitted;
    const Vec3 pixel = CenterPixel(Render(whitted, scene, LookDown({0.0f, 1.0f, 0.0f}, 1.0f)));

    // R0 = ((1 - 1.5) / (1 + 1.5))^2 = 0.04 per surface. summed over every bounce between the surfaces:
    // R + (1 - R)^2 R (1 + R^2 + R^4 + ...) = 2R / (1 + R)
    const float r0 = 0.04f;
    // MinWeight drops the bounces after the second (~6e-5), hence the tolerance
    CHECK_THAT(pixel.x, WithinAbs(2.0f * r0 / (1.0f + r0), 1e-4f));
}

TEST_CASE("Whitted furnace: a glass ball vanishes in a uniform sky", "[whitted][furnace][glass]")
{
    Scene scene = FurnaceScene();
    const int glass = scene.AddMaterial({.Type = MaterialType::Dielectric, .Ior = 1.5f});
    scene.Spheres = {{{0.0f, 0.0f, -3.0f}, 1.0f, glass}};
    WhittedIntegrator whitted;

    SECTION("away from the rim, R + (1 - R) = 1 at every surface")
    {
        // 20 degree view keeps every ray well inside the ball's 19.5 degree angular radius
        CheckAllPixels(Render(whitted, scene, LookForward(Vec3(0.0f), 20.0f)), Vec3(1.0f), 2e-3f);
    }

    SECTION("at the rim, glancing light circles inside and the depth/weight cutoffs lose a little")
    {
        const HdrFramebuffer image = Render(whitted, scene, LookForward(Vec3(0.0f), 45.0f), 33);
        double sum = 0.0;
        float lowest = 1.0f;
        for (int y = 0; y < image.Height(); y++)
        {
            for (int x = 0; x < image.Width(); x++)
            {
                const float value = image[x, y].x;
                sum += value;
                lowest = std::min(lowest, value);
            }
        }
        CHECK_THAT(sum / static_cast<double>(image.PixelCount()), WithinAbs(1.0, 1e-3));
        CHECK(lowest > 0.95f); // ~0.972 with depth 10. before the normal fix this was ~0.93
    }
}

TEST_CASE("Whitted furnace: a hollow glass ball vanishes with enough depth", "[whitted][furnace][glass]")
{
    Scene scene = FurnaceScene();
    const int glass = scene.AddMaterial({.Type = MaterialType::Dielectric, .Ior = 1.5f});
    scene.Spheres = {
        {{0.0f, 0.0f, -3.0f}, 1.0f, glass},
        {{0.0f, 0.0f, -3.0f}, -0.9f, glass}, // negative radius: the inside wall of the shell
    };

    // the straight path crosses 4 surfaces, and one reflection inside the far wall needs 7 levels.
    // depth 6 gives ~0.964 here, depth 10 gives ~0.999
    WhittedIntegrator whitted(10);
    // narrow view through the middle. near the rim the shell traps light by total internal reflection,
    // and the cutoffs lose a few percent there even at high depth
    CheckAllPixels(Render(whitted, scene, LookForward(Vec3(0.0f), 10.0f)), Vec3(1.0f), 2e-3f);
}