#include <window/InputManager.h>

void InputManager::Update()
{
	keysPressed.clear();
	keysReleased.clear();
	mousePressed.clear();
	mouseReleased.clear();

	mouseDelta.x = mousePos.x - lastMousePos.x;
	mouseDelta.y = lastMousePos.y - mousePos.y;
	lastMousePos = mousePos;
}

void InputManager::UpdateCursorMode(CursorMode mode)
{
	cursorMode = mode;
	ignoreNextMouseMovementEvent = true;
}
