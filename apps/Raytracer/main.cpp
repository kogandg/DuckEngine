#include "Camera.h"
#include "Geometry.h"

#include <Duck/Core/Framebuffer.h>
#include <Duck/Core/ThreadPool.h>
#include <Duck/Platform/Application.h>
#include <Duck/VkCore/CpuFramebufferPresenter.h>
#include <Duck/VkCore/FrameContext.h>
#include <Duck/VkCore/Swapchain.h>
#include <Duck/VkCore/VulkanContext.h>

#include <algorithm>
#include <format>
#include <vector>

using namespace Duck;

namespace
{
    constexpr std::uint32_t TileSize = 32;

    // sky with white at horizon and blue on top
    Vec3 SkyColor(const Ray& ray)
    {
        const float a = 0.5f * (ray.Direction.y + 1.0f);
        return glm::mix(Vec3(1.0f), Vec3(0.5f, 0.7f, 1.0f), a);
    }

    std::optional<Hit> ClosestHit(const std::vector<Sphere>& spheres, const Ray& ray, float tMin, float tMax)
    {
        std::optional<Hit> closest;
        for (const Sphere& sphere : spheres)
        {
            // shrinking tMax means later spheres only count if they are nearer
            if (const auto hit = sphere.Intersect(ray, tMin, tMax))
            {
                closest = hit;
                tMax = hit->T;
            }
        }
        return closest;
    }

    Vec3 TraceRay(const std::vector<Sphere>& spheres, const Ray& ray)
    {
        if (const auto hit = ClosestHit(spheres, ray, 0.001f, Infinity))
        {
            // map the normal components from [-1, 1] to [0, 1] so they can be shown as a color
            return 0.5f * (hit->Normal + Vec3(1.0f));
        }
        return SkyColor(ray);
    }
}

class RaytracerApp : public Application
{
public:
    RaytracerApp()
        : Application({.Title = "DuckEngine Raytracer"}), context(this->GetWindow()),
          swapchain(this->context, this->GetWindow()), frames(this->context, this->swapchain), presenter(this->context),
          hdr(640, 360), display(640, 360),
          camera({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, 90.0f, 640.0f / 360.0f)
    {
    }

protected:
    void OnUpdate(float deltaSeconds) override
    {
        if (GetWindow().GetInput().WasPressed(Key::Escape))
        {
            Quit();
        }

        Render();
        UpdateTitle(deltaSeconds);

        const auto frame = frames.BeginFrame();
        if (!frame)
        {
            return;
        }
        presenter.Present(*frame, display);
        frames.EndFrame();
    }

    void OnStop() override { vkDeviceWaitIdle(context.Device()); }

private:
    void Render()
    {
        const std::uint32_t width = hdr.Width();
        const std::uint32_t height = hdr.Height();

        const std::uint32_t tilesX = (width + TileSize - 1) / TileSize;
        const std::uint32_t tilesY = (height + TileSize - 1) / TileSize;

        // one index per tile, tiles never overlap so threads never write to the same pixel
        ThreadPool::Shared().ParallelFor(tilesX * tilesY,
                                         [&](std::uint32_t tile)
                                         {
                                             // edges of the tile
                                             const std::uint32_t x0 = (tile % tilesX) * TileSize;
                                             const std::uint32_t y0 = (tile / tilesX) * TileSize;
                                             const std::uint32_t x1 = std::min(x0 + TileSize, width);
                                             const std::uint32_t y1 = std::min(y0 + TileSize, height);

                                             for (std::uint32_t y = y0; y < y1; ++y)
                                             {
                                                 for (std::uint32_t x = x0; x < x1; ++x)
                                                 {
                                                     // sample the pixel center
                                                     const float s =
                                                         (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
                                                     const float t =
                                                         (static_cast<float>(y) + 0.5f) / static_cast<float>(height);

                                                     const Vec3 color = TraceRay(spheres, camera.GetRay(s, t));
                                                     hdr[x, y] = color;
                                                     display[x, y] = PackRgba8(LinearToSrgb(color));
                                                 }
                                             }
                                         });
    }

    void UpdateTitle(float deltaSeconds)
    {
        fpsTime += deltaSeconds;
        fpsFrames++;
        if (fpsTime >= 0.5f)
        {
            GetWindow().SetTitle(std::format("DuckEngine Sandbox - {:.0f} FPS", fpsFrames / fpsTime));
            fpsTime = 0.0f;
            fpsFrames = 0;
        }
    }

    // declaration order is construction order, and reverse is destruction order
    VulkanContext context;
    Swapchain swapchain;
    FrameContext frames;
    CpuFramebufferPresenter presenter;
    HdrFramebuffer hdr;
    Framebuffer display;

    Camera camera;
    std::vector<Sphere> spheres{
        {{0.0f, 0.0f, -1.0f}, 0.5f},
        {{0.0f, -100.5f, -1.0f}, 100.0f}, // huge sphere standing in for a ground plane
    };

    float fpsTime = 0.0f;
    std::uint32_t fpsFrames = 0;
};

int main()
{
    RaytracerApp app;
    return app.Run();
}