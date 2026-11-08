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
    Scene MakeScene()
    {
        Scene scene;
        scene.Materials = {
            Material{.Albedo = Vec3(0.5f)},                          // 0 ground
            Material{.Albedo = Vec3(0.1f, 0.2f, 0.5f)},              // 1 blue diffuse
            Material{.Type = MaterialType::Dielectric, .Ior = 1.5f}, // 2 glass
            Material{.Type = MaterialType::Mirror, .Albedo = Vec3(0.8f, 0.6f, 0.2f)}, // 3 gold mirror
        };
        scene.Spheres = {
            {{0.0f, -100.5f, -1.0f}, 100.0f, 0}, // huge sphere standing in for a ground plane
            {{0.0f, 0.0f, -1.2f}, 0.5f, 1},
            {{-1.0f, 0.0f, -1.0f}, 0.5f, 2},
            {{1.0f, 0.0f, -1.0f}, 0.5f, 3},
        };
        return scene;
    }
}

class RaytracerApp : public Application
{
public:
    RaytracerApp()
        : Application({.Title = "DuckEngine Raytracer"}), context(this->GetWindow()),
          swapchain(this->context, this->GetWindow()), frames(this->context, this->swapchain), presenter(this->context),
          film(640, 360), display(640, 360),
          camera({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, 90.0f, 640.0f / 360.0f),
          scene(MakeScene())
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

        IIntegrator& integrator = *integrators[current];
        if (film.Passes < integrator.RecommendedPasses())
        {
            integrators[current]->RenderPass(scene, camera, film);
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

        const Key keys[] = {Key::D1, Key::D2, Key::D3};
        for (size_t i = 0; i < integrators.size() && i < std::size(keys); i++)
        {
            if (input.WasPressed(keys[i]) && i != current)
            {
                current = i;
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
            GetWindow().SetTitle(std::format(
                "DuckEngine Raytracer - {} - {} passes - {} - {:.0f} FPS", integrators[current]->Name(), film.Passes,
                film.Passes >= integrators[current]->RecommendedPasses() ? "Done" : "In Progress",
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

    Camera camera;
    Scene scene;

    std::vector<std::unique_ptr<IIntegrator>> integrators;
    size_t current = 0;

    float fpsTime = 0.0f;
    int fpsFrames = 0;
};

int main()
{
    RaytracerApp app;
    return app.Run();
}