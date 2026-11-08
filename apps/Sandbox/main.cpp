#include <Duck/Core/Framebuffer.h>
#include <Duck/Core/Log.h>
#include <Duck/Core/ThreadPool.h>
#include <Duck/Platform/Application.h>
#include <Duck/VkCore/CpuFramebufferPresenter.h>
#include <Duck/VkCore/FrameContext.h>
#include <Duck/VkCore/Swapchain.h>
#include <Duck/VkCore/VulkanContext.h>

#include <cmath>
#include <format>

using namespace Duck;
using namespace Duck::Vk;

class SandboxApp : public Application
{
public:
    SandboxApp()
        : Application({.Title = "DuckEngine Sandbox"}), context(this->GetWindow()),
          swapchain(this->context, this->GetWindow()), frames(this->context, this->swapchain), presenter(this->context),
          framebuffer(320, 180)
    {
    }

protected:
    void OnUpdate(float deltaSeconds) override
    {
        if (GetWindow().GetInput().WasPressed(Key::Escape))
        {
            Quit();
        }

        time += deltaSeconds;
        UpdateTitle(deltaSeconds);

        const float blue = 0.5f + 0.5f * std::sin(time * 2.0f);
        ThreadPool::Shared().ParallelFor(framebuffer.Height(),
                                         [&](int y)
                                         {
                                             for (int x = 0; x < framebuffer.Width(); x++)
                                             {
                                                 framebuffer[x, y] = PackRgba8(
                                                     Vec3(static_cast<float>(x) / framebuffer.Width(),
                                                          static_cast<float>(y) / framebuffer.Height(), blue));
                                             }
                                         });

        const auto frame = frames.BeginFrame();
        if (!frame)
        {
            return;
        }
        presenter.Present(*frame, framebuffer);
        frames.EndFrame();
    }

    void OnStop() override { vkDeviceWaitIdle(context.Device()); }

private:
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
    Framebuffer framebuffer;

    float time = 0.0f;
    float fpsTime = 0.0f;
    int fpsFrames = 0;
};

int main()
{
    SandboxApp app;
    return app.Run();
}