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
        if (this->GetWindow().GetInput().WasPressed(Key::Escape))
        {
            this->Quit();
        }

        this->time += deltaSeconds;
        this->UpdateTitle(deltaSeconds);

        const float blue = 0.5f + 0.5f * std::sin(this->time * 2.0f);
        ThreadPool::Shared().ParallelFor(this->framebuffer.Height(),
                                         [&](std::uint32_t y)
                                         {
                                             for (std::uint32_t x = 0; x < this->framebuffer.Width(); ++x)
                                             {
                                                 this->framebuffer[x, y] = PackRgba8(
                                                     Vec3(static_cast<float>(x) / this->framebuffer.Width(),
                                                          static_cast<float>(y) / this->framebuffer.Height(), blue));
                                             }
                                         });

        const auto frame = this->frames.BeginFrame();
        if (!frame)
        {
            return;
        }
        this->presenter.Present(*frame, this->framebuffer);
        this->frames.EndFrame();
    }

    void OnStop() override { vkDeviceWaitIdle(this->context.Device()); }

private:
    void UpdateTitle(float deltaSeconds)
    {
        this->fpsTime += deltaSeconds;
        this->fpsFrames++;
        if (this->fpsTime >= 0.5f)
        {
            this->GetWindow().SetTitle(std::format("DuckEngine Sandbox - {:.0f} FPS", this->fpsFrames / this->fpsTime));
            this->fpsTime = 0.0f;
            this->fpsFrames = 0;
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
    std::uint32_t fpsFrames = 0;
};

int main()
{
    SandboxApp app;
    return app.Run();
}