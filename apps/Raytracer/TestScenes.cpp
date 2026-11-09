#include "TestScenes.h"

#include <utility>

using namespace Duck;
using namespace Duck::PathTrace;

namespace
{
    constexpr Vec3 WorldUp{0.0f, 1.0f, 0.0f};

    TestScene MakeSpheres(float aspectRatio)
    {
        Scene scene;
        const int ground = scene.AddMaterial({.Albedo = Vec3(0.5f)});
        const int blue = scene.AddMaterial({.Albedo = Vec3(0.1f, 0.2f, 0.5f)});
        const int glass = scene.AddMaterial({.Type = MaterialType::Dielectric, .Ior = 1.5f});
        const int gold = scene.AddMaterial({.Type = MaterialType::Mirror, .Albedo = Vec3(0.8f, 0.6f, 0.2f)});

        scene.Spheres = {
            {{0.0f, -100.5f, -1.0f}, 100.0f, ground}, // huge sphere standing in for a ground plane
            {{0.0f, 0.0f, -1.2f}, 0.5f, blue},
            {{-1.0f, 0.0f, -1.0f}, 0.5f, glass},
            {{1.0f, 0.0f, -1.0f}, 0.5f, gold},
        };
        scene.DirectionalLights = {{glm::normalize(Vec3(0.6f, 1.0f, 0.4f)), Vec3(0.7f)}};

        return {.Name = "Spheres",
                .Scene = std::move(scene),
                .Camera = Camera({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, WorldUp, 90.0f, aspectRatio)};
    }

    // cornell box with a mirror ball and a glass ball
    TestScene MakeCornellBox(float aspectRatio)
    {
        Scene scene;
        const int white = scene.AddMaterial({.Albedo = Vec3(0.73f)});
        const int red = scene.AddMaterial({.Albedo = Vec3(0.65f, 0.05f, 0.05f)});
        const int green = scene.AddMaterial({.Albedo = Vec3(0.12f, 0.45f, 0.15f)});
        const int mirror = scene.AddMaterial({.Type = MaterialType::Mirror, .Albedo = Vec3(0.95f)});
        const int glass = scene.AddMaterial({.Type = MaterialType::Dielectric, .Ior = 1.5f});

        // box spans x and z in [-1, 1] and y in [0, 2], open toward the camera at z = +1
        scene.Quads = {
            {{-1.0f, 0.0f, 1.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -2.0f}, white}, // floor
            {{-1.0f, 2.0f, 1.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -2.0f}, white}, // ceiling
            {{-1.0f, 0.0f, -1.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, white}, // back
            {{-1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -2.0f}, {0.0f, 2.0f, 0.0f}, red},   // left
            {{1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -2.0f}, {0.0f, 2.0f, 0.0f}, green},  // right
        };
        scene.Spheres = {
            {{-0.45f, 0.4f, -0.35f}, 0.4f, mirror},
            {{0.45f, 0.4f, 0.3f}, 0.4f, glass},
        };

        // a point light just under the ceiling stands in for the usual area light
        scene.PointLights = {{{0.0f, 1.9f, 0.0f}, Vec3(3.0f)}};

        // closed room: black background, and a small flat ambient so shadows aren't pure black
        scene.SkyDown = scene.SkyUp = Vec3(0.0f);
        scene.AmbientDown = scene.AmbientUp = Vec3(0.08f);

        return {.Name = "Cornell Box",
                .Scene = std::move(scene),
                .Camera = Camera({0.0f, 1.0f, 3.75f}, {0.0f, 1.0f, 0.0f}, WorldUp, 40.0f, aspectRatio)};
    }

    // after figure 2 of Whitted's 1980 paper: checkerboard floor, mirror ball, hollow glass ball, blue sky
    TestScene MakeWhitted(float aspectRatio)
    {
        Scene scene;
        const int checker = scene.AddMaterial(
            {.Albedo = Vec3(0.8f, 0.1f, 0.05f), .CheckerAlbedo = Vec3(0.9f, 0.8f, 0.1f), .CheckerSize = 1.0f});
        const int mirror = scene.AddMaterial({.Type = MaterialType::Mirror, .Albedo = Vec3(0.9f)});
        const int glass = scene.AddMaterial({.Type = MaterialType::Dielectric, .Ior = 1.5f});

        scene.Quads = {{{-8.0f, 0.0f, 4.0f}, {16.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -24.0f}, checker}};
        scene.Spheres = {
            {{0.8f, 1.6f, -5.0f}, 1.0f, mirror},
            {{-0.5f, 1.3f, -2.6f}, 0.6f, glass},
            {{-0.5f, 1.3f, -2.6f}, -0.55f, glass}, // negative radius flips the normal: a hollow shell
        };

        scene.DirectionalLights = {{glm::normalize(Vec3(-0.4f, 1.0f, 0.6f)), Vec3(1.0f)}};
        scene.SkyDown = Vec3(0.45f, 0.6f, 0.95f);
        scene.SkyUp = Vec3(0.1f, 0.25f, 0.7f);
        scene.AmbientDown = Vec3(0.15f);
        scene.AmbientUp = Vec3(0.2f, 0.22f, 0.3f);

        return {.Name = "Whitted 1980",
                .Scene = std::move(scene),
                .Camera = Camera({0.0f, 1.6f, 2.5f}, {0.0f, 1.2f, -3.0f}, WorldUp, 40.0f, aspectRatio)};
    }
}

std::vector<TestScene> MakeTestScenes(float aspectRatio)
{
    std::vector<TestScene> scenes;
    scenes.push_back(MakeSpheres(aspectRatio));
    scenes.push_back(MakeCornellBox(aspectRatio));
    scenes.push_back(MakeWhitted(aspectRatio));
    return scenes;
}