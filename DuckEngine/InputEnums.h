#pragma once

enum class MouseButton
{
	Unknown,
	Left,
	Right,
	Middle
};

enum class Key
{
	Unknown,
	W, A, S, D,
	Up, Down, Left, Right,
	Space, Escape, Enter, Tab,
	Shift, Control, Alt
};

enum class CursorMode
{
	Normal,
	Locked
};