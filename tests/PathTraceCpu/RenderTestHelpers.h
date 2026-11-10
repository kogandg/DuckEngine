#pragma once

#include "TestHelpers.h"

#include <Duck/Core/Framebuffer.h>
#include <Duck/PathTraceCpu/Camera.h>
#include <Duck/PathTraceCpu/Film.h>
#include <Duck/PathTraceCpu/Integrator.h>
#include <Duck/PathTraceCpu/Scene.h>
#include <Duck/Core/ThreadPool.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <format>

namespace Duck::Test
{
    // average linear radiance per pixel over `passes` passes: what Resolve shows, before sRGB encoding
    inline HdrFramebuffer Render(PathTrace::IIntegrator& integrator, const PathTrace::Scene& scene,
                                 const PathTrace::Camera& camera, int size = 9, int passes = 1)
    {
        PathTrace::Film film(size, size);
        for (int i = 0; i < passes; i++)
        {
            integrator.RenderPass(scene, camera, film, ThreadPool::Shared());
            film.Passes++;
        }

        HdrFramebuffer image(size, size);
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                image[x, y] = film.Sum[x, y] / static_cast<float>(passes);
            }
        }
        return image;
    }

    inline Vec3 CenterPixel(const HdrFramebuffer& image) { return image[image.Width() / 2, image.Height() / 2]; }

    // no sky and no ambient: only what a test adds gives light
    inline PathTrace::Scene DarkScene()
    {
        PathTrace::Scene scene;
        scene.SkyDown = scene.SkyUp = Vec3(0.0f);
        scene.AmbientDown = scene.AmbientUp = Vec3(0.0f);
        return scene;
    }

    // uniform sky of radiance 1 and nothing else: anything that conserves energy must vanish in it
    inline PathTrace::Scene FurnaceScene()
    {
        PathTrace::Scene scene = DarkScene();
        scene.SkyDown = scene.SkyUp = Vec3(1.0f);
        return scene;
    }

    // up can't be +Y when looking straight up or down, it would be parallel to the view direction
    inline PathTrace::Camera LookDown(const Vec3& position, float fovDegrees)
    {
        return {position, position - Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), fovDegrees, 1.0f};
    }

    inline PathTrace::Camera LookUp(const Vec3& position, float fovDegrees)
    {
        return {position, position + Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), fovDegrees, 1.0f};
    }

    // level camera looking down -Z
    inline PathTrace::Camera LookForward(const Vec3& position, float fovDegrees)
    {
        return {position, position - Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), fovDegrees, 1.0f};
    }

    // large horizontal quad through the origin
    inline PathTrace::Quad Floor(int materialId, float halfSize = 100.0f)
    {
        return {{-halfSize, 0.0f, -halfSize}, {2.0f * halfSize, 0.0f, 0.0f}, {0.0f, 0.0f, 2.0f * halfSize}, materialId};
    }

    // checks every pixel against expected and reports the worst one if any are off
    inline void CheckAllPixels(const HdrFramebuffer& image, const Vec3& expected, float tolerance)
    {
        int wrong = 0;
        float worstError = 0.0f;
        int worstX = 0;
        int worstY = 0;
        for (int y = 0; y < image.Height(); y++)
        {
            for (int x = 0; x < image.Width(); x++)
            {
                const Vec3 difference = glm::abs(image[x, y] - expected);
                const float error = std::max({difference.x, difference.y, difference.z});
                if (error > tolerance)
                {
                    wrong++;
                }
                if (error > worstError)
                {
                    worstError = error;
                    worstX = x;
                    worstY = y;
                }
            }
        }

        const Vec3 worst = image[worstX, worstY];
        INFO(std::format("{} of {} pixels off by more than {}. worst ({}, {}) = {}, expected {}", wrong,
                         image.PixelCount(), tolerance, worstX, worstY, Catch::StringMaker<Vec3>::convert(worst),
                         Catch::StringMaker<Vec3>::convert(expected)));
        CHECK(wrong == 0);
    }
}