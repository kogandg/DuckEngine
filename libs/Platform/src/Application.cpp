#include <Duck/Platform/Application.h>

#include <algorithm>
#include <chrono>

namespace Duck
{

    Application::Application(const WindowDesc& desc) : window(desc) {}

    void Application::Quit() { this->window.RequestClose(); }

    int Application::Run()
    {
        using Clock = std::chrono::steady_clock;

        this->OnStart();

        auto last = Clock::now();
        while (!this->window.ShouldClose())
        {
            this->window.PollEvents();

            // sleep instead of spinning while minimized and dont count that time as a frame
            if (this->window.IsMinimized())
            {
                while (this->window.IsMinimized() && !this->window.ShouldClose())
                {
                    this->window.WaitEvents();
                }
                last = Clock::now();
                continue;
            }

            const auto now = Clock::now();
            // clamped so a breakpoint or a window drag doest produce one big step
            const float deltaSeconds = std::min(std::chrono::duration<float>(now - last).count(), 0.25f);
            last = now;

            this->OnUpdate(deltaSeconds);
        }

        this->OnStop();
        return 0;
    }

}