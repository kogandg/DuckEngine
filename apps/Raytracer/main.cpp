#include "TestScenes.h"

#include <Duck/Core/ThreadPool.h>
#include <Duck/RayTraceCpu/BackgroundRenderer.h>
#include <Duck/RayTraceCpu/Camera.h>
#include <Duck/RayTraceCpu/Film.h>
#include <Duck/RayTraceCpu/NormalIntegrator.h>
#include <Duck/RayTraceCpu/PathTracer.h>
#include <Duck/RayTraceCpu/Scene.h>
#include <Duck/RayTraceCpu/WhittedIntegrator.h>
#include <Duck/Platform/Application.h>
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
    void RestartRender()
    {
        const TestScene& test = scenes[currentScene];
        renderer.Restart(test.Scene, test.Camera, *integrators[currentIntegrator]);
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
            RestartRender();
        }
    }

    void UpdateTitle(float deltaSeconds)
    {
        fpsTime += deltaSeconds;
        fpsFrames++;
        if (fpsTime >= 0.5f)
        {
            const auto progress = renderer.GetProgress();
            GetWindow().SetTitle(std::format(
                "DuckEngine Raytracer - {} - {} - {}/{} passes - {} - {:.0f} ms/pass - {:.0f} FPS", scenes[currentScene].Name,
                integrators[currentIntegrator]->Name(), progress.Passes, progress.TargetPasses,
                progress.Done() ? "Done" : "In Progress", progress.LastPassMs, fpsFrames / fpsTime));
            fpsTime = 0.0f;
            fpsFrames = 0;
        }
    }

    // declaration order is construction order, and reverse is destruction order
    VulkanContext context;
    Swapchain swapchain;
    FrameContext frames;
    CpuFramebufferPresenter presenter;

    Framebuffer display;

    std::vector<TestScene> scenes;
    size_t currentScene = 0;

    std::vector<std::unique_ptr<IIntegrator>> integrators;
    size_t currentIntegrator = 0;

    BackgroundRenderer renderer{FilmWidth, FilmHeight, RenderThreadCount()};

    float fpsTime = 0.0f;
    int fpsFrames = 0;
};

int main()
{
    RaytracerApp app;
    return app.Run();
}