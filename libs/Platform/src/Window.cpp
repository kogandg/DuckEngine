#include <Duck/Platform/Window.h>

#include <Duck/Core/Assert.h>
#include <Duck/Core/Log.h>

#include <GLFW/glfw3.h>

namespace Duck
{
    namespace
    {
        int windowCount = 0; // glfw init with first window and only terminated with last
                             // window closing

        void OnGlfwError(int code, const char* description) { Log::Error("GLFW error {}: {}", code, description); }

        void AcquireGlfw()
        {
            if (windowCount == 0)
            {
                glfwSetErrorCallback(OnGlfwError);
                DUCK_VERIFY(glfwInit() == GLFW_TRUE, "glfwInit failed");
            }
            windowCount++;
        }

        void ReleaseGlfw()
        {
            windowCount--;
            if (windowCount <= 0)
            {
                glfwTerminate();
            }
        }

        Window& Self(GLFWwindow* handle) { return *static_cast<Window*>(glfwGetWindowUserPointer(handle)); }

        void OnKeyCallback(GLFWwindow* handle, int key, int, int action, int)
        {
            Self(handle).GetInput().OnKey(key, action);
        }
        void OnMouseButtonCallback(GLFWwindow* handle, int button, int action, int)
        {
            Self(handle).GetInput().OnMouseButton(button, action);
        }
        void OnCursorPositionCallback(GLFWwindow* handle, double x, double y)
        {
            Self(handle).GetInput().OnCursorPosition(x, y);
        }
        void OnScrollCallback(GLFWwindow* handle, double x, double y) { Self(handle).GetInput().OnScroll(x, y); }
    }

    Window::Window(const WindowDesc& desc)
    {
        AcquireGlfw();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, desc.Resizable ? GLFW_TRUE : GLFW_FALSE);

        this->handle = glfwCreateWindow(static_cast<int>(desc.Width), static_cast<int>(desc.Height), desc.Title.c_str(),
                                        nullptr, nullptr);
        DUCK_VERIFY(this->handle != nullptr, "Failed to create window");

        glfwSetWindowUserPointer(this->handle, this);
        glfwSetKeyCallback(this->handle, OnKeyCallback);
        glfwSetMouseButtonCallback(this->handle, OnMouseButtonCallback);
        glfwSetCursorPosCallback(this->handle, OnCursorPositionCallback);
        glfwSetScrollCallback(this->handle, OnScrollCallback);
    }
    Window::~Window()
    {
        glfwDestroyWindow(this->handle);
        ReleaseGlfw();
    }

    bool Window::ShouldClose() const { return glfwWindowShouldClose(this->handle) == GLFW_TRUE; }
    void Window::RequestClose() { glfwSetWindowShouldClose(this->handle, GLFW_TRUE); }

    void Window::SetTitle(const std::string& title) { glfwSetWindowTitle(this->handle, title.c_str()); }

    UVec2 Window::Size() const
    {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(this->handle, &width, &height);
        return {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
    }
    UVec2 Window::FramebufferSize() const
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(this->handle, &width, &height);
        return {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
    }
    bool Window::IsMinimized() const
    {
        const UVec2 size = this->FramebufferSize();
        return size.x == 0 || size.y == 0;
    }

    void Window::PollEvents()
    {
        this->input.BeginFrame();
        glfwPollEvents();
    }

    void Window::SetCursorMode(CursorMode mode)
    {
        int glfwMode = GLFW_CURSOR_NORMAL;
        switch (mode)
        {
        case CursorMode::Normal:
            glfwMode = GLFW_CURSOR_NORMAL;
            break;
        case CursorMode::Hidden:
            glfwMode = GLFW_CURSOR_HIDDEN;
            break;
        case CursorMode::Captured:
            glfwMode = GLFW_CURSOR_DISABLED;
            break;
        }

        glfwSetInputMode(this->handle, GLFW_CURSOR, glfwMode);
        if (glfwRawMouseMotionSupported())
        {
            glfwSetInputMode(this->handle, GLFW_RAW_MOUSE_MOTION,
                             mode == CursorMode::Captured ? GLFW_TRUE : GLFW_FALSE);
        }
        this->input.ResetMouseTracking();
    }
}