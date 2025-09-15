#pragma once

#include <array>
#include <GLFW/glfw3.h>

#include "InputEnums.h"

class GLFWInputMap
{
public:

    void setWindow(GLFWwindow* w) { windowRef = w; }

    const std::array<Key, GLFW_KEY_LAST + 1> keyMap = createKeyMap();
    const std::array<MouseButton, GLFW_MOUSE_BUTTON_LAST + 1> mouseButtonMap = createMouseMap();

    inline Key translateKey(int glfwKey)
    {
        return keyMap[glfwKey];
    }

    inline MouseButton translateMouseButton(int glfwMouseButton)
    {
        return mouseButtonMap[glfwMouseButton];
    }

    inline bool translateButtonAction(int glfwAction)
    {
        return (glfwAction != GLFW_RELEASE);
    }

    void setCursorMode(CursorMode mode)
    {
        switch (mode)
        {
        case CursorMode::Normal:
            glfwSetInputMode(windowRef, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;
        case CursorMode::Locked:
            glfwSetInputMode(windowRef, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            break;
        default:
            break;
        }
    }

private:
    GLFWwindow* windowRef;

    inline std::array<Key, GLFW_KEY_LAST + 1> createKeyMap()
    {
        std::array<Key, GLFW_KEY_LAST + 1> map{};
        map.fill(Key::Unknown);

        // Letters
        map[GLFW_KEY_W] = Key::W;
        map[GLFW_KEY_A] = Key::A;
        map[GLFW_KEY_S] = Key::S;
        map[GLFW_KEY_D] = Key::D;

        // Arrow keys
        map[GLFW_KEY_UP] = Key::Up;
        map[GLFW_KEY_DOWN] = Key::Down;
        map[GLFW_KEY_LEFT] = Key::Left;
        map[GLFW_KEY_RIGHT] = Key::Right;

        // Modifiers
        map[GLFW_KEY_SPACE] = Key::Space;
        map[GLFW_KEY_ESCAPE] = Key::Escape;
        map[GLFW_KEY_ENTER] = Key::Enter;
        map[GLFW_KEY_TAB] = Key::Tab;
        map[GLFW_KEY_LEFT_SHIFT] = Key::Shift;
        map[GLFW_KEY_LEFT_CONTROL] = Key::Control;
        map[GLFW_KEY_LEFT_ALT] = Key::Alt;

        return map;
    }

    inline std::array<MouseButton, GLFW_MOUSE_BUTTON_LAST + 1> createMouseMap()
    {
        std::array<MouseButton, GLFW_MOUSE_BUTTON_LAST + 1> map{};
        map.fill(MouseButton::Unknown);

        map[GLFW_MOUSE_BUTTON_LEFT] = MouseButton::Left;
        map[GLFW_MOUSE_BUTTON_RIGHT] = MouseButton::Right;
        map[GLFW_MOUSE_BUTTON_MIDDLE] = MouseButton::Middle;

        return map;
    }
};