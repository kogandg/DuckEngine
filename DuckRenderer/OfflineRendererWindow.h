#pragma once
#include "ImageWindow.h"
#include "OfflineRenderer.h"
#include <thread>
#include "RenderTarget.h"
class OfflineRendererWindow : public ImageWindow
{
public:
	OfflineRendererWindow(const char* title, int width, int height, std::shared_ptr<OfflineRenderer> renderer, std::shared_ptr<RenderTarget> renderTarget);
	OfflineRendererWindow(const char* title, std::shared_ptr<OfflineRenderer> renderer, std::shared_ptr<RenderTarget> renderTarget);

	virtual void Init() override;
	virtual void CleanUp() override;

	void InitObjects();

	void StartRendering();

	virtual void IdleCallback() override;
	virtual void DisplayCallback() override;

private:
	std::shared_ptr<OfflineRenderer> renderer;
	std::shared_ptr<RenderTarget> renderTarget;

	bool joined = false;
	std::thread worker;

	bool cancelPressed = false;
};

