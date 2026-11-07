#include <Duck/Platform/Input.h>

#include <GLFW/glfw3.h>

namespace Duck
{
    void Input::BeginFrame()
    {
        keyPressed.reset();
        keyReleased.reset();
        buttonPressed.reset();
        buttonReleased.reset();
        mouseDelta = Vec2(0.0f);
        scroll = Vec2(0.0f);
    }

    bool Input::IsDown(Key key) const { return keyDown[static_cast<std::size_t>(key)]; }
    bool Input::WasPressed(Key key) const { return keyPressed[static_cast<std::size_t>(key)]; }
    bool Input::WasReleased(Key key) const { return keyReleased[static_cast<std::size_t>(key)]; }

    bool Input::IsDown(MouseButton button) const { return buttonDown[static_cast<std::size_t>(button)]; }
    bool Input::WasPressed(MouseButton button) const { return buttonPressed[static_cast<std::size_t>(button)]; }
    bool Input::WasReleased(MouseButton button) const { return buttonReleased[static_cast<std::size_t>(button)]; }

    void Input::OnKey(int key, int action)
    {
        if (key < 0 || key >= static_cast<int>(KeyCount))
        {
            return;
        }

        if (action == GLFW_PRESS)
        {
            keyDown.set(key);
            keyPressed.set(key);
        }
        else if (action == GLFW_RELEASE)
        {
            keyDown.reset(key);
            keyReleased.set(key);
        }
    }

    void Input::OnMouseButton(int button, int action)
    {
        if (button < 0 || button >= static_cast<int>(MouseButtonCount))
        {
            return;
        }

        if (action == GLFW_PRESS)
        {
            buttonDown.set(button);
            buttonPressed.set(button);
        }
        else if (action == GLFW_RELEASE)
        {
            buttonDown.reset(button);
            buttonReleased.set(button);
        }
    }

    void Input::OnCursorPosition(double x, double y)
    {
        const Vec2 position(static_cast<float>(x), static_cast<float>(y));
        if (hasMousePosition)
        {
            mouseDelta += position - mousePosition;
        }
        mousePosition = position;
        hasMousePosition = true;
    }

    void Input::OnScroll(double x, double y) { scroll += Vec2(static_cast<float>(x), static_cast<float>(y)); }

    void Input::ResetMouseTracking() { hasMousePosition = false; }
}