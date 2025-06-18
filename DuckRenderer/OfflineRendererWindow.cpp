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

	if (renderer.get()->GetProgressState() != ProgressState::Done && ImGui::Button("Cancel rendering"))
	{
		cancelPressed = true;
	}

	if (saveEnable)
	{
		imguiSaveDialog();
	}

	if (ImGui::Button("Debug"))
	{
		int count = 0;
	}

	ImGui::End();

	Window::End();
}


