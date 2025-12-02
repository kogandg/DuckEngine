#include <window/Window.h>

Window::Window(const char* title, int width, int height)
{
	this->title = title;
	this->width = width;
	this->height = height;
}

void Window::Init()
{
	// Create GLFW window
	glfwWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!glfwWindow)
	{
		std::cerr << "Failed to create GLFW window: " << title << std::endl;
	}

	// Make the window's context current
	glfwMakeContextCurrent(glfwWindow);

	glfwSwapInterval(1); // Enable vsync

	// Create a separate ImGui context for this window
	imguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(imguiContext);

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	// Initialize ImGui for this window
	ImGui_ImplGlfw_InitForOpenGL(glfwWindow, false);
	ImGui_ImplOpenGL3_Init("#version 330");


	// Setup input callbacks
	glfwSetWindowUserPointer(glfwWindow, this);

	glfwSetFramebufferSizeCallback(glfwWindow, framebufferResizeCallback);
	glfwSetKeyCallback(glfwWindow, keyCallback);
	glfwSetCharCallback(glfwWindow, charCallback);
	glfwSetMouseButtonCallback(glfwWindow, mouseButtonCallback);
	glfwSetCursorPosCallback(glfwWindow, cursorPosCallback);
	glfwSetScrollCallback(glfwWindow, scrollCallback);

	initialized = true;
}

void Window::CleanUp()
{
	if (!initialized) return;
	// Set the context before shutting down
	ImGui::SetCurrentContext(imguiContext);

	// Shutdown ImGui for this window
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	// Destroy the context
	ImGui::DestroyContext(imguiContext);

	//glfwDestroyWindow(glfwWindow);
}

void Window::Destroy()
{
	// Destroy the GLFW window
	glfwDestroyWindow(glfwWindow);
}

void Window::IdleCallback()
{
	if (!initialized) return;
}

void Window::DisplayCallback()
{
	Begin();
	End();
}

bool Window::Begin()
{
	if (!initialized) return false;
	if (glfwWindowShouldClose(glfwWindow)) return false;

	// Make this window's context current
	if (glfwGetCurrentContext() != glfwWindow)
	{
		glfwMakeContextCurrent(glfwWindow);
	}
	// Set ImGui context for this window
	ImGui::SetCurrentContext(imguiContext);

	// wipe the drawing surface clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Start ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//glfwPollEvents();

	return true;
}

void Window::End()
{
	// Render ImGui
	ImGui::Render();
	ImGui::EndFrame();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
	//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
		//delete backup_current_context;
	}

	// Swap buffers
	glfwSwapBuffers(glfwWindow);
}

int Window::ShouldClose()
{
	return initialized ? glfwWindowShouldClose(glfwWindow) : GLFW_FALSE;
}

void Window::MakeActive()
{
	if (!initialized) return;
	if (glfwWindowShouldClose(glfwWindow)) return;
	// Make this window's context current
	if (glfwGetCurrentContext() != glfwWindow)
	{
		glfwMakeContextCurrent(glfwWindow);
	}
	// Set ImGui context for this window
	ImGui::SetCurrentContext(imguiContext);
}

void Window::onFramebufferResize(int width, int height)
{
	glfwMakeContextCurrent(glfwWindow);

	this->width = width;
	this->height = height;
	// Update viewport
	glViewport(0, 0, width, height);
}

void Window::onKey(int key, int scancode, int action, int mods)
{
	// Switch to this window's ImGui context
	ImGui::SetCurrentContext(imguiContext);
	ImGui_ImplGlfw_KeyCallback(glfwWindow, key, scancode, action, mods);

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			// Close the window. This causes the program to also terminate.
			glfwSetWindowShouldClose(glfwWindow, GL_TRUE);
			break;

		default:
			break;
		}
	}
}

void Window::onChar(unsigned int codepoint)
{
	ImGui::SetCurrentContext(imguiContext);
	ImGui_ImplGlfw_CharCallback(glfwWindow, codepoint);
}

void Window::onMouseButton(int button, int action, int mods)
{
	ImGui::SetCurrentContext(imguiContext);
	ImGui_ImplGlfw_MouseButtonCallback(glfwWindow, button, action, mods);
}

void Window::onCursorPos(double currX, double currY)
{
	ImGui::SetCurrentContext(imguiContext);
	ImGui_ImplGlfw_CursorPosCallback(glfwWindow, currX, currY);
}

void Window::onScroll(double xOffset, double yOffset)
{
	ImGui::SetCurrentContext(imguiContext);
	ImGui_ImplGlfw_ScrollCallback(glfwWindow, xOffset, yOffset);
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!instance) return;
	if (instance->glfwWindow != window) return;

	instance->onFramebufferResize(width, height);
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!instance) return;
	if (instance->glfwWindow != window) return;

	instance->onKey(key, scancode, action, mods);
}

void Window::charCallback(GLFWwindow* window, unsigned int codepoint)
{
	auto instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!instance) return;
	if (instance->glfwWindow != window) return;

	instance->onChar(codepoint);
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	auto instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!instance) return;
	if (instance->glfwWindow != window) return;

	instance->onMouseButton(button, action, mods);
}

void Window::cursorPosCallback(GLFWwindow* window, double currX, double currY)
{
	auto instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!instance) return;
	if (instance->glfwWindow != window) return;

	instance->onCursorPos(currX, currY);
}

void Window::scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	auto instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!instance) return;
	if (instance->glfwWindow != window) return;

	instance->onScroll(xOffset, yOffset);
}
