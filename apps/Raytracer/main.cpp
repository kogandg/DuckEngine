#include "TestScenes.h"

#include <Duck/Core/Console.h>
#include <Duck/Core/Log.h>
#include <Duck/Core/ThreadPool.h>
#include <Duck/Platform/Application.h>
#include <Duck/RayTraceCpu/BackgroundRenderer.h>
#include <Duck/RayTraceCpu/Camera.h>
#include <Duck/RayTraceCpu/CostIntegrator.h>
#include <Duck/RayTraceCpu/Film.h>
#include <Duck/RayTraceCpu/NormalIntegrator.h>
#include <Duck/RayTraceCpu/PathTracer.h>
#include <Duck/RayTraceCpu/RayStats.h>
#include <Duck/RayTraceCpu/RenderScene.h>
#include <Duck/RayTraceCpu/Scene.h>
#include <Duck/RayTraceCpu/WhittedIntegrator.h>
#include <Duck/VkCore/CpuFramebufferPresenter.h>
#include <Duck/VkCore/FrameContext.h>
#include <Duck/VkCore/Swapchain.h>
#include <Duck/VkCore/VulkanContext.h>

#include <algorithm>
#include <format>
#include <iterator>
#include <memory>
#include <thread>
#include <vector>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Vk;

namespace
{
    constexpr int FilmWidth = 640;
    constexpr int FilmHeight = 360;

    unsigned RenderThreadCount()
    {
        const unsigned hardware = std::max(1u, std::thread::hardware_concurrency());
        return hardware > 1 ? hardware - 1 : 1;
    }
}

class RaytracerApp : public Application
{
public:
    RaytracerApp()
        : Application({.Title = "DuckEngine Raytracer"}), context(this->GetWindow()),
          swapchain(this->context, this->GetWindow()), frames(this->context, this->swapchain), presenter(this->context),
          display(FilmWidth, FilmHeight),
          scenes(MakeTestScenes(static_cast<float>(FilmWidth) / static_cast<float>(FilmHeight)))
    {
        integrators.push_back(std::make_unique<PathTracer>());
        integrators.push_back(std::make_unique<WhittedIntegrator>());
        integrators.push_back(std::make_unique<NormalIntegrator>());
        integrators.push_back(std::make_unique<CostIntegrator>());

        for (const TestScene& test : scenes)
        {
            builtScenes.push_back(BuildScene(test.Scene));
        }

        RestartRender();
    }

protected:
    void OnStart() override
    {
        // #ifdef _DEBUG
        //         Log::Info("Running in Debug");
        // #elif defined(NDEBUG)
        //         Log::Info("Running in Release");
        // #else
        //         Log::Info("Running in Unknown");
        // #endif
    }

    void OnUpdate(float deltaSeconds) override
    {
        HandleInput();
        renderer.TakeLatest(display);

        UpdateStatus(deltaSeconds);

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
    void RestartRender()
    {
        renderer.Restart(builtScenes[currentScene], scenes[currentScene].Camera, *integrators[currentIntegrator]);
    }

    void HandleInput()
    {
        const Input& input = GetWindow().GetInput();
        if (input.WasPressed(Key::Escape))
        {
            Quit();
        }

        bool changed = false;

        const Key integratorKeyNext = Key::Up;
        const Key integratorKeyPrev = Key::Down;
        if (input.WasPressed(integratorKeyNext))
        {
            currentIntegrator++;
            if (currentIntegrator > integrators.size() - 1)
            {
                currentIntegrator = 0;
            }
            changed = true;
        }
        if (input.WasPressed(integratorKeyPrev))
        {
            if (currentIntegrator <= 0)
            {
                currentIntegrator = integrators.size() - 1;
            }
            else
            {
                currentIntegrator--;
            }
            changed = true;
        }

        const Key sceneKeyNext = Key::Right;
        const Key sceneKeyPrev = Key::Left;
        if (input.WasPressed(sceneKeyNext))
        {
            currentScene++;
            if (currentScene > scenes.size() - 1)
            {
                currentScene = 0;
            }
            changed = true;
        }
        if (input.WasPressed(sceneKeyPrev))
        {
            if (currentScene <= 0)
            {
                currentScene = scenes.size() - 1;
            }
            else
            {
                currentScene--;
            }
            changed = true;
        }

        if (changed)
        {
            Log::Info("{} with {}", scenes[currentScene].Name, integrators[currentIntegrator]->Name());
            RestartRender();
        }
    }

    void UpdateStatus(float deltaSeconds)
    {
        statusSeconds += deltaSeconds;
        statusFrames++;
        if (statusSeconds < 0.25f)
        {
            return;
        }

        // rates over the time since the last update
        const RayStats::Counters totals = RayStats::Totals();
        const RayStats::Counters delta = totals - lastTotals;
        lastTotals = totals;
        const double rays = static_cast<double>(delta.Rays());
        const auto perSecond = [&](uint64_t count) { return static_cast<double>(count) / statusSeconds / 1e6; };
        const auto perRay = [&](uint64_t count) { return rays > 0.0 ? static_cast<double>(count) / rays : 0.0; };

        const auto progress = renderer.GetProgress();
        constexpr int BarWidth = 30;
        const int filled = progress.TargetPasses > 0 ? BarWidth * progress.Passes / progress.TargetPasses : 0;

        int spheres = 0;
        int quads = 0;
        int triangles = 0;
        const RenderScene& built = *builtScenes[currentScene];
        for (const Shape& shape : built.GetShapes())
        {
            spheres += std::holds_alternative<Sphere>(shape) ? 1 : 0;
            quads += std::holds_alternative<Quad>(shape) ? 1 : 0;
            triangles += std::holds_alternative<Triangle>(shape) ? 1 : 0;
        }

        const std::string lines[] = {
            "--------------------------------------------------------------------",
            std::format("Scene       {} ({}/{})  [Left/Right]", scenes[currentScene].Name, currentScene + 1,
                        scenes.size()),
            std::format("Integrator  {} ({}/{})  [Up/Down]", integrators[currentIntegrator]->Name(),
                        currentIntegrator + 1, integrators.size()),
            std::format("Progress    [{}{}] {}/{} passes  {}", std::string(static_cast<size_t>(filled), '#'),
                        std::string(static_cast<size_t>(BarWidth - filled), '-'), progress.Passes,
                        progress.TargetPasses, progress.Done() ? "done" : "rendering"),
            std::format("Timing      {:.1f} ms/pass   display {:.0f} fps", progress.LastPassMs,
                        statusFrames / statusSeconds),
            std::format("Rays        {:.2f} M/s   ({:.2f} closest hit + {:.2f} shadow)", perSecond(delta.Rays()),
                        perSecond(delta.ClosestHitRays), perSecond(delta.OcclusionRays)),
            std::format("Per ray     {:.1f} primitive tests   {:.1f} box tests", perRay(delta.PrimitiveTests),
                        perRay(delta.NodeTests)),
            std::format("Geometry    {} spheres  {} quads  {} triangles  {} lights", spheres, quads, triangles,
                        built.GetLights().size()),
        };
        Console::SetStatus(lines);

        statusSeconds = 0.0f;
        statusFrames = 0;
    }

    float statusSeconds = 0.0f;
    int statusFrames = 0;
    RayStats::Counters lastTotals;

    // declaration order is construction order, and reverse is destruction order
    VulkanContext context;
    Swapchain swapchain;
    FrameContext frames;
    CpuFramebufferPresenter presenter;

    Framebuffer display;

    std::vector<TestScene> scenes;
    std::vector<std::shared_ptr<const RenderScene>> builtScenes;
    size_t currentScene = 0;

    std::vector<std::unique_ptr<IIntegrator>> integrators;
    size_t currentIntegrator = 0;

    BackgroundRenderer renderer{FilmWidth, FilmHeight, RenderThreadCount()};
};

int main()
{
    RaytracerApp app;
    return app.Run();
}