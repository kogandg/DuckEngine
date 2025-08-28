#include "SelectionWindow.h"

SelectionWindow::SelectionWindow(const char* title, int width, int height) : Window(title, width, height)
{

}

void SelectionWindow::Init(ImVec4 color)
{
	Window::Init();

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("fonts/DroidSans.ttf", 22);

	this->glBackgroundColor = color;
	imguiBackgroundColor = ImVec4(0, 0, 0, 0);

	fullWindowFlags = ImGuiWindowFlags_NoDecoration | 
					ImGuiWindowFlags_NoMove | 
					ImGuiWindowFlags_NoResize | 
					ImGuiWindowFlags_NoSavedSettings;
}

void SelectionWindow::IdleCallback()
{
	Window::IdleCallback();
}

void SelectionWindow::DisplayCallback()
{
	if (!Window::Begin()) return;

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));

	

	// Create window-specific ImGui content
	ImGui::Begin("Selection window", nullptr, fullWindowFlags);//("Window: " + std::string(title)).c_str());

	//// Color picker button
	ImGui::Text("Change Background Color:");
	ImGui::ColorEdit3(("GL Background Color Picker##" + std::string(title)).c_str(), (float*)&glBackgroundColor);

	ImGui::ColorEdit4("ImGui Background Color Picker", (float*)&imguiBackgroundColor);

	ImGui::Text("Window width: %d", width);
	ImGui::Text("Window height: %d", height);

	if (ImGui::Button("Offline Renderer"))
	{
		offlinePressed = true;
	}
	if (ImGui::Button("Realtime Renderer"))
	{
		realTimePressed = true;
	}

	ImGui::End();

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg] = imguiBackgroundColor;

	// Clear the screen with the selected color
	glClearColor(
		glBackgroundColor.x,
		glBackgroundColor.y,
		glBackgroundColor.z,
		glBackgroundColor.w
	);
	glClear(GL_COLOR_BUFFER_BIT);

	Window::End();
}