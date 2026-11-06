#include <Duck/Platform/Window.h>
#include <Duck/Core/Log.h>

using namespace Duck;

int main()
{
    Window window({.Title = "DuckEngine sandbox"});
    bool captured = false;

    while (!window.ShouldClose())
    {
        window.PollEvents();
        const Input& input = window.GetInput();

        if (input.WasPressed(Key::Escape))
        {
            window.RequestClose();
        }
        if (input.WasPressed(Key::Tab))
        {
            captured = !captured;
            window.SetCursorMode(captured ? CursorMode::Captured : CursorMode::Normal);
        }
        if (input.WasPressed(MouseButton::Left))
        {
            Log::Info("Click at ({}, {})", input.MousePosition().x, input.MousePosition().y);
        }
        if (input.IsDown(Key::W))
        {
            Log::Info("W held, mouse delta ({}, {})", input.MouseDelta().x, input.MouseDelta().y);
        }
    }

    Log::Info("Window closed");
}