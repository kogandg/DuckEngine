#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Platform/Input.h>

#include <cstdint>
#include <string>

struct GLFWwindow;

namespace Duck
{

    enum class CursorMode
    {
        Normal,   // visible, free
        Hidden,   // invisible while over the window, free
        Captured, // invisible and locked, for mouselook
    };

    struct WindowDesc
    {
        std::string Title = "DuckEngine";
        std::uint32_t Width = 1280;
        std::uint32_t Height = 720;
        bool Resizable = true;
    };

    class Window
    {
    public:
        explicit Window(const WindowDesc& desc = {});
        ~Window();

        // window shoudlnt be copied or moved
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        bool ShouldClose() const;
        void RequestClose();
        void SetTitle(const std::string& title);

        UVec2 Size() const;
        UVec2 FramebufferSize() const;
        bool IsMinimized() const; // framebuffer size should report 0 if minimized

        GLFWwindow* Handle() const noexcept { return this->handle; }

        //resets perframe input state, then processes OS events
        void PollEvents();
        //like PollEvents but sleeps until an event arives, for minimized windows
        void WaitEvents();

        Input& GetInput() noexcept { return this->input; }
        const Input& GetInput() const noexcept { return this->input; }

        void SetCursorMode(CursorMode mode);

    private:
        GLFWwindow* handle = nullptr;
        Input input;
    };
}