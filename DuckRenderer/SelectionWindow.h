#pragma once
#include "Window.h"

#include <iostream>

class SelectionWindow : public Window
{
public:
	SelectionWindow(const char* title, int width, int height);

	void Init(ImVec4 color);

	void IdleCallback() override;
	void DisplayCallback();

	//inline void SetBackgroundColor(ImVec4 color) { backgroundColor = color; }
	bool offlinePressed = false;

private:

	ImGuiWindowFlags fullWindowFlags;
	ImVec4 glBackgroundColor;
	ImVec4 imguiBackgroundColor;
};

