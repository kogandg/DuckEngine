#pragma once

#include <Duck/Platform/Window.h>

namespace Duck
{
    class Application
    {
    public:
        explicit Application(const WindowDesc& desc = {});
        virtual ~Application() = default;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        // runs until the window is closed or Quit() is called, returns the process exit code
        int Run();

        // asks the loop to stop after the current frame
        void Quit();

    protected:
        // called once right before the first update
        virtual void OnStart() {}
        // called once per frame, not called while the window is minimized
        virtual void OnUpdate(float deltaSeconds) { (void)deltaSeconds; }
        // called once after the loop ends, while the window and your members still exist
        virtual void OnStop() {}

        Window& GetWindow() noexcept { return this->window; }

    private:
        Window window;
    };
}