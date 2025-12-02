#include <window/WindowManager.h>

void WindowManager::AddWindow(Window* window)
{
	windows.emplace(window);
}

void WindowManager::RemoveWindow(Window* window)
{
	windows.erase(window);
}

void WindowManager::CleanRemoveWindow(Window* window)
{
	if (!windows.contains(window)) { return; }
	window->CleanUp();
	window->Destroy();
	windows.erase(window);
}

void WindowManager::IdleCallback()
{
	for (auto window : windows)
	{
		window->IdleCallback();
	}
}

void WindowManager::DisplayCallback()
{
	for (auto window : windows)
	{
		window->DisplayCallback();
	}
}

void WindowManager::CleanUp()
{
	for (auto window : windows)
	{
		window->CleanUp();
	}
}

void WindowManager::DestroyWindows()
{
	for (auto window : windows)
	{
		window->Destroy();
	}
}

bool WindowManager::SomeClosed()
{
	for (auto window : windows)
	{
		if (window->Initialized() && window->ShouldClose())
		{
			return true;
		}
	}
	return false;
}

bool WindowManager::AllClosed()
{
	for (auto window : windows)
	{
		if (window->Initialized() && !window->ShouldClose())
		{
			return false;
		}
	}
	return true;
}
