#pragma once

#include <unordered_set>
#include <glm/glm.hpp>

#include "InputEnums.h"


class InputManager
{
public:


	void Update();

	bool inline IsKeyDown(Key key) const { return keysDown.contains(key); }
	bool inline WasKeyPressed(Key key) const { return keysPressed.contains(key); }
	bool inline WasKeyReleased(Key key) const { return keysReleased.contains(key); }

	glm::vec2 GetMousePos() const { return mousePos; }
	glm::vec2 GetMouseDelta() const { return mouseDelta; }

	bool inline IsMouseDown(MouseButton button) const { return mouseDown.contains(button); }
	bool inline WasMousePressed(MouseButton button) const { return mousePressed.contains(button); }
	bool inline WasMouseReleased(MouseButton button) const { return mouseReleased.contains(button); }

	void UpdateCursorMode(CursorMode mode);
	CursorMode inline GetCursorMode() const { return cursorMode; }

	void onKeyEvent(Key key, bool pressed) 
	{
		if (pressed) {
			keysDown.insert(key);
			keysPressed.insert(key);
		}
		else {
			keysDown.erase(key);
			keysReleased.insert(key);
		}
	}

	void onMouseButtonEvent(MouseButton button, bool pressed) 
	{
		if (pressed) {
			mouseDown.insert(button);
			mousePressed.insert(button);
		}
		else {
			mouseDown.erase(button);
			mouseReleased.insert(button);
		}
	}

	void onMouseMove(double x, double y) 
	{
		mousePos = { (float)x, (float)y };
		if (firstMouseMove)
		{
			lastMousePos = mousePos;
			firstMouseMove = false;
		}
		if (ignoreNextMouseMovementEvent)
		{
			lastMousePos = mousePos;
			ignoreNextMouseMovementEvent = false;
		}
	}

private:
	std::unordered_set<Key> keysDown;
	std::unordered_set<Key> keysPressed;
	std::unordered_set<Key> keysReleased;

	std::unordered_set<MouseButton> mouseDown;
	std::unordered_set<MouseButton> mousePressed;
	std::unordered_set<MouseButton> mouseReleased;

	glm::vec2 mousePos{ 0.0f };
	glm::vec2 lastMousePos{ 0.0f };
	glm::vec2 mouseDelta{ 0.0f };
	bool firstMouseMove = true;

	CursorMode cursorMode = CursorMode::Normal;
	bool ignoreNextMouseMovementEvent = false;
};

