#pragma once

#include <GL/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_stdlib.h"

#include <iostream>

class Window
{
public:
	Window(const char* title, int width, int height);

	virtual void Init();
	virtual void CleanUp();
	virtual void Destroy();

	virtual void IdleCallback();
	virtual void DisplayCallback();

	virtual bool Begin();
	virtual void End();

	virtual int ShouldClose();

	inline GLFWwindow* GetWindow() { return glfwWindow; }
	//inline ImGuiContext* GetImGuiContext() { return imguiContext; }
	inline bool Initialized() { return initialized; }

	void MakeActive();

protected:
	int width;
	int height;
	const char* title;

	GLFWwindow* glfwWindow;
	ImGuiContext* imguiContext;

	bool initialized = false;

	virtual void onFramebufferResize(int width, int height);
	virtual void onKey(int key, int scancode, int action, int mods);
	virtual void onChar(unsigned int codepoint);
	virtual void onMouseButton(int button, int action, int mods);
	virtual void onCursorPos(double currX, double currY);
	virtual void onScroll(double xOffset, double yOffset);

private:
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void charCallback(GLFWwindow* window, unsigned int codepoint);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void cursorPosCallback(GLFWwindow* window, double currX, double currY);
	static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
};

