#pragma once
#include "ImageWindow.h"
#include "OfflineRenderer.h"
#include <thread>
class OfflineRendererWindow : public ImageWindow
{
public:
	OfflineRendererWindow(const char* title, int width, int height);

	virtual void Init() override;
	virtual void CleanUp() override;

	void InitObjects();

	void StartRendering(int width, int height);

	virtual void IdleCallback() override;
	virtual void DisplayCallback() override;

private:
	OfflineRenderer renderer;

	bool joined = false;
	std::thread worker;

};

