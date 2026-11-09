#include "TestScenes.h"

#include <Duck/PathTraceCpu/Camera.h>
#include <Duck/PathTraceCpu/Film.h>
#include <Duck/PathTraceCpu/NormalIntegrator.h>
#include <Duck/PathTraceCpu/PathTracer.h>
#include <Duck/PathTraceCpu/Scene.h>
#include <Duck/PathTraceCpu/WhittedIntegrator.h>
#include <Duck/Platform/Application.h>
#include <Duck/VkCore/CpuFramebufferPresenter.h>
#include <Duck/VkCore/FrameContext.h>
#include <Duck/VkCore/Swapchain.h>
#include <Duck/VkCore/VulkanContext.h>

#include <format>
#include <iterator>
#include <memory>
#include <vector>

using namespace Duck;
using namespace Duck::PathTrace;
using namespace Duck::Vk;

namespace
{
    constexpr int FilmWidth = 640;
    constexpr int FilmHeight = 360;
}

class RaytracerApp : public Application
{
public:
    RaytracerApp()
        : Application({.Title = "DuckEngine Raytracer"}), context(this->GetWindow()),
          swapchain(this->context, this->GetWindow()), frames(this->context, this->swapchain), presenter(this->context),
          film(FilmWidth, FilmHeight), display(FilmWidth, FilmHeight),
          scenes(MakeTestScenes(static_cast<float>(FilmWidth) / static_cast<float>(FilmHeight)))
    {
        integrators.push_back(std::make_unique<PathTracer>());
        integrators.push_back(std::make_unique<WhittedIntegrator>());
        integrators.push_back(std::make_unique<NormalIntegrator>());
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

        IIntegrator& integrator = *integrators[currentIntegrator];
        const TestScene& test = scenes[currentScene];
        if (film.Passes < integrator.RecommendedPasses())
        {
            integrators[currentIntegrator]->RenderPass(test.Scene, test.Camera, film);
            film.Passes++;
            film.Resolve(display, integrator.GetOutputTransform());
        }

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
    void HandleInput()
    {
        const Input& input = GetWindow().GetInput();
        if (input.WasPressed(Key::Escape))
        {
            Quit();
        }

        const Key integratorKeys[] = {Key::D1, Key::D2, Key::D3};
        for (size_t i = 0; i < integrators.size() && i < std::size(integratorKeys); i++)
        {
            if (input.WasPressed(integratorKeys[i]) && i != currentIntegrator)
            {
                currentIntegrator = i;
                film.Reset(); // the accumulated sum belongs to the old integrator
            }
        }

        const Key sceneKeys[] = {Key::Q, Key::W, Key::E};
        for (size_t i = 0; i < scenes.size() && i < std::size(sceneKeys); i++)
        {
            if (input.WasPressed(sceneKeys[i]) && i != currentScene)
            {
                currentScene = i;
                film.Reset(); // the accumulated sum belongs to the old integrator
            }
        }
    }

    void UpdateTitle(float deltaSeconds)
    {
        fpsTime += deltaSeconds;
        fpsFrames++;
        if (fpsTime >= 0.5f)
        {
            GetWindow().SetTitle(
                std::format("DuckEngine Raytracer - {} - {} - {} passes - {} - {:.0f} FPS", scenes[currentScene].Name,
                            integrators[currentIntegrator]->Name(), film.Passes,
                            film.Passes >= integrators[currentIntegrator]->RecommendedPasses() ? "Done" : "In Progress",
                            fpsFrames / fpsTime));
            fpsTime = 0.0f;
            fpsFrames = 0;
        }
    }

    // declaration order is construction order, and reverse is destruction order
    VulkanContext context;
    Swapchain swapchain;
    FrameContext frames;
    CpuFramebufferPresenter presenter;

    Film film;
    Framebuffer display;

    std::vector<TestScene> scenes;
    size_t currentScene = 0;

    std::vector<std::unique_ptr<IIntegrator>> integrators;
    size_t currentIntegrator = 0;

    float fpsTime = 0.0f;
    int fpsFrames = 0;
};

int main()
{
    RaytracerApp app;
    return app.Run();
}