#include "OfflineRendererWindow.h"

OfflineRendererWindow::OfflineRendererWindow(const char* title, int width, int height, std::shared_ptr<OfflineRenderer> renderer, std::shared_ptr<RenderTarget> renderTarget) : ImageWindow(title, width, height)
{
	this->renderer = renderer;
	this->renderTarget = renderTarget;
}

OfflineRendererWindow::OfflineRendererWindow(const char* title, std::shared_ptr<OfflineRenderer> renderer, std::shared_ptr<RenderTarget> renderTarget) : ImageWindow(title, renderTarget.get()->GetWidth(), renderTarget.get()->GetHeight())
{
	this->renderer = renderer;
	this->renderTarget = renderTarget;
}

void OfflineRendererWindow::Init()
{
	ImageWindow::Init();
}

void OfflineRendererWindow::CleanUp()
{
	renderer.get()->Cancel();
	if (!joined)
	{
		if (worker.joinable())
		{
			worker.join();
		}
		else
		{
			worker.detach();
		}
	}

	ImageWindow::CleanUp();
}

void OfflineRendererWindow::InitObjects()
{
	initGLObjects();
	renderer.get()->Initialize();
}

void OfflineRendererWindow::StartRendering()
{
	worker = std::thread(&OfflineRenderer::Render, renderer.get(), renderTarget);
	saveEnable = true;
}

void OfflineRendererWindow::IdleCallback()
{
	//Window::IdleCallback();

	if (!initialized) return;

	if (renderer.get()->GetProgressState() != ProgressState::NotStarted)
	{
		loadTexture(renderTarget.get()->GetImage(), renderTarget.get()->GetWidth(), renderTarget.get()->GetHeight());
	}

	if (worker.joinable() && (renderer.get()->GetProgressState() == ProgressState::Done || renderer.get()->GetProgressState() == ProgressState::Canceled))
	{
		worker.join();
		joined = true;
	}

	if (savePressed)
	{
		saveImage(renderTarget.get()->GetImage(), renderTarget.get()->GetWidth(), renderTarget.get()->GetHeight());
	}

	if (cancelPressed)
	{
		renderer.get()->Cancel();
	}

	if (pausePressed)
	{
		renderer.get()->TogglePaused();
		pausePressed = false;
	}
}

void OfflineRendererWindow::DisplayCallback()
{
	if (!Window::Begin()) return;
	savePressed = false;
	cancelPressed = false;

	drawQuad();

	// Create window-specific ImGui content
	ImGui::Begin("Render info: ", nullptr, 0);

	ImGui::Text("Window width: %d", width);
	ImGui::Text("Window height: %d", height);

	auto imageSize = glm::ivec2(renderTarget.get()->GetWidth(), renderTarget.get()->GetHeight());
	ImGui::Text("Image Size: %d, %d", imageSize.x, imageSize.y);

	int totalPixels = imageSize.x * imageSize.y;
	ImGui::Text("Total Pixels: %d", totalPixels);
	int pixelsRendered = renderer.get()->GetPixelsRendered();
	ImGui::Text("Rendered pixels: %d", pixelsRendered);

	float progress = (float)pixelsRendered / totalPixels;

	char progressLabel[32];
	snprintf(progressLabel, sizeof(progressLabel), "%.1f%%", progress * 100.0f);
	ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), progressLabel);

	auto progressState = renderer.get()->GetProgressState();
	if (progressState != ProgressState::Done && progressState != ProgressState::Canceled)
	{
		if (ImGui::Button(progressState != ProgressState::Paused ? "Pause rendering" : "Resume rendering"))
		{
			pausePressed = true;
		}
		if (ImGui::Button("Cancel rendering"))
		{
			cancelPressed = true;
		}
	}

	if (saveEnable)
	{
		imguiSaveDialog();
	}

	if (ImGui::Button("Debug"))
	{
		bool debug = true;
	}

	

	ImGui::End();

	int windowPosX = 0;
	int windowPosY = 0;
	glfwGetWindowPos(glfwWindow, &windowPosX, &windowPosY);
	ImGui::SetNextWindowPos(ImVec2(windowPosX, windowPosY));
	ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));
	
	auto windowFlags = ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBackground;

	ImGui::Begin("Overlay", nullptr,
		windowFlags);

	ImVec2 imagePos = ImGui::GetWindowPos();
	auto tiles = renderer.get()->GetTiles();
	auto drawList = ImGui::GetWindowDrawList();
	for (auto tile : tiles)
	{
		if (tile->state == TileState::InProgress)
		{
			float lineWidth = 2.0f;
			ImVec2 p0 = ImVec2(imagePos.x + tile->x, imagePos.y + tile->y);
			ImVec2 p1 = ImVec2(p0.x + tile->width, p0.y + tile->height);
			drawList->AddRect(p0, p1, IM_COL32(255, 0, 0, 128), 0.0f, 0, lineWidth);
		}
	}

	ImGui::End();

	Window::End();
}