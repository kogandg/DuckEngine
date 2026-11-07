#include <Duck/Platform/Application.h>

#include <algorithm>
#include <chrono>

namespace Duck
{

    Application::Application(const WindowDesc& desc) : window(desc) {}

    void Application::Quit() { window.RequestClose(); }

    int Application::Run()
    {
        using Clock = std::chrono::steady_clock;

        OnStart();

        auto last = Clock::now();
        while (!window.ShouldClose())
        {
            window.PollEvents();

            // sleep instead of spinning while minimized and dont count that time as a frame
            if (window.IsMinimized())
            {
                while (window.IsMinimized() && !window.ShouldClose())
                {
                    window.WaitEvents();
                }
                last = Clock::now();
                continue;
            }

            const auto now = Clock::now();
            // clamped so a breakpoint or a window drag doest produce one big step
            const float deltaSeconds = std::min(std::chrono::duration<float>(now - last).count(), 0.25f);
            last = now;

            OnUpdate(deltaSeconds);
        }

        OnStop();
        return 0;
    }

}