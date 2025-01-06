#include "OfflineRendererWindow.h"

OfflineRendererWindow::OfflineRendererWindow(const char* title, int width, int height) : ImageWindow(title, width, height)
{
}

void OfflineRendererWindow::Init()
{
	ImageWindow::Init();
}

void OfflineRendererWindow::CleanUp()
{
	renderer.Cancel();
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
}

void OfflineRendererWindow::StartRendering(int width, int height)
{
	worker = std::thread(&OfflineRenderer::Render, &renderer, width, height);
	saveEnable = true;
}

void OfflineRendererWindow::IdleCallback()
{
	//Window::IdleCallback();

	if (!initialized) return;

	if (renderer.Started())
	{
		loadTexture(renderer.GetImage(), renderer.GetPixelWidth(), renderer.GetPixelHeight());
	}

	if (worker.joinable() && renderer.DoneRendering())
	{
		worker.join();
		joined = true;
	}

	if (savePressed)
	{
		saveImage(renderer.GetImage(), width, height);
	}

	if (cancelPressed)
	{
		renderer.Cancel();
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

	if (ImGui::Button("Cancel rendering"))
	{
		cancelPressed = true;
	}

	if (saveEnable)
	{
		imguiSaveDialog();
	}

	ImGui::End();

	Window::End();
}


