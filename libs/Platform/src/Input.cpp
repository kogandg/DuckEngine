#include <Duck/Platform/Input.h>

#include <GLFW/glfw3.h>

namespace Duck
{
    void Input::BeginFrame()
    {
        this->keyPressed.reset();
        this->keyReleased.reset();
        this->buttonPressed.reset();
        this->buttonReleased.reset();
        this->mouseDelta = Vec2(0.0f);
        this->scroll = Vec2(0.0f);
    }

    bool Input::IsDown(Key key) const { return this->keyDown[static_cast<std::size_t>(key)]; }
    bool Input::WasPressed(Key key) const { return this->keyPressed[static_cast<std::size_t>(key)]; }
    bool Input::WasReleased(Key key) const { return this->keyReleased[static_cast<std::size_t>(key)]; }

    bool Input::IsDown(MouseButton button) const { return this->buttonDown[static_cast<std::size_t>(button)]; }
    bool Input::WasPressed(MouseButton button) const { return this->buttonPressed[static_cast<std::size_t>(button)]; }
    bool Input::WasReleased(MouseButton button) const { return this->buttonReleased[static_cast<std::size_t>(button)]; }

    void Input::OnKey(int key, int action)
    {
        if (key < 0 || key >= static_cast<int>(KeyCount))
        {
            return;
        }

        if (action == GLFW_PRESS)
        {
            this->keyDown.set(key);
            this->keyPressed.set(key);
        }
        else if (action == GLFW_RELEASE)
        {
            this->keyDown.reset(key);
            this->keyReleased.set(key);
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
            this->buttonDown.set(button);
            this->buttonPressed.set(button);
        }
        else if (action == GLFW_RELEASE)
        {
            this->buttonDown.reset(button);
            this->buttonReleased.set(button);
        }
    }

    void Input::OnCursorPosition(double x, double y)
    {
        const Vec2 position(static_cast<float>(x), static_cast<float>(y));
        if (this->hasMousePosition)
        {
            this->mouseDelta += position - this->mousePosition;
        }
        this->mousePosition = position;
        this->hasMousePosition = true;
    }

    void Input::OnScroll(double x, double y) { this->scroll += Vec2(static_cast<float>(x), static_cast<float>(y)); }

    void Input::ResetMouseTracking() { this->hasMousePosition = false; }
}