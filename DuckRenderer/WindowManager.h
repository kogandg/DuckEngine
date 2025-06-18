#pragma once

#include "Window.h"
#include <unordered_set>

class WindowManager
{
public:
	void AddWindow(Window* window);
	void RemoveWindow(Window* window);
	void CleanRemoveWindow(Window* window);

	void IdleCallback();
	void DisplayCallback();
	void CleanUp();
	void DestroyWindows();

	bool SomeClosed();
	bool AllClosed();

private:
	std::unordered_set<Window*> windows;
};

