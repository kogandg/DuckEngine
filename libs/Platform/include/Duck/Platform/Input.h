#pragma once

#include <Duck/Core/Math.h>

#include <bitset>
#include <cstddef>

namespace Duck
{
    // values match GLFWs key codes so conversion is a cast
    // clang-format off
    enum class Key : int
    {
        Space = 32,
        D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9,
        A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Escape = 256, Enter, Tab, Backspace,
        Right = 262, Left, Down, Up,
        F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        LeftShift = 340, LeftControl, LeftAlt,
    };
    
    enum class MouseButton : int 
    {
      Left = 0,
      Right = 1,
      Middle = 2,
    };
    // clang-format on

    class Input
    {
    public:
        // once per frame before polling OS events
        void BeginFrame();

        // held right now
        bool IsDown(Key key) const;
        // went down / up since the last beginframe, true for 1 frame
        bool WasPressed(Key key) const;
        bool WasReleased(Key key) const;

        bool IsDown(MouseButton button) const;
        bool WasPressed(MouseButton button) const;
        bool WasReleased(MouseButton button) const;

        // mouse pos in window coordinates, origin top-left
        Vec2 MousePosition() const { return this->mousePosition; }
        // movement accumulated this frame, pos Y is down
        Vec2 MouseDelta() const { return this->mouseDelta; }
        // scroll accumulated this frame, Y is the normal wheel axis
        Vec2 Scroll() const { return this->scroll; }

        // called by window from the GLFW callbacks, not for general use
        void OnKey(int key, int action);
        void OnMouseButton(int button, int action);
        void OnCursorPosition(double x, double y);
        void OnScroll(double x, double y);
        // call when the cursor mode changes so the next move doesnt produce a huge delta
        void ResetMouseTracking();

    private:
        static constexpr std::size_t KeyCount = 349;
        static constexpr std::size_t MouseButtonCount = 8;

        std::bitset<KeyCount> keyDown;
        std::bitset<KeyCount> keyPressed;
        std::bitset<KeyCount> keyReleased;

        std::bitset<MouseButtonCount> buttonDown;
        std::bitset<MouseButtonCount> buttonPressed;
        std::bitset<MouseButtonCount> buttonReleased;

        Vec2 mousePosition{0.0f};
        Vec2 mouseDelta{0.0f};
        Vec2 scroll{0.0f};
        bool hasMousePosition = false;
    };
}