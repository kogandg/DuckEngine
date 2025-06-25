#pragma once

#include <iostream>

#include <embree4/rtcore.h>
#include <glm/glm.hpp>

#include "Intersection.h"
#include "Triangle.h"
#include "Sphere.h"
#include "Scene.h"
#include "RenderableObject.h"
#include "Integrator.h"
#include "RenderTarget.h"

#include <thread>
#include <chrono>
#include <algorithm>
#include <condition_variable>


enum ProgressState
{
	NotStarted,
	Running,
	Done,
	Canceled,
	Paused
};

class OfflineRenderer
{
public:
	OfflineRenderer(std::shared_ptr<Integrator> integrator, std::shared_ptr<Scene> scene);

	void Initialize();
	void Render(std::shared_ptr<RenderTarget> target);
	
	inline ProgressState GetProgressState() { return progressState; }
	//inline bool DoneRendering() { return started && !running; }
	//inline unsigned char* GetImage() { return image; }
	inline void Cancel() { progressState = ProgressState::Canceled; pauseCV.notify_all(); }

	void TogglePaused();

	//inline bool Started() { return started; }

	inline int GetPixelWidth() { return scene.get()->GetCamera().get()->GetPixelWidth(); }
	inline int GetPixelHeight() { return scene.get()->GetCamera().get()->GetPixelHeight(); }
	inline glm::ivec2 GetCurrentPixel() { return currentPixel; }

private:

	RTCDevice embreeDevice;
	std::shared_ptr<RenderableObject> tri;
	std::shared_ptr<RenderableObject> sphere;

	//bool started = false;
	//bool running = false;
	//bool cancel = false;

	ProgressState progressState = ProgressState::NotStarted;
	glm::ivec2 currentPixel = glm::ivec2(0, 0);

	std::mutex pauseMutex;
	std::condition_variable pauseCV;

	int maxDepth;

	////void errorFunction(void* userPtr, enum RTCError error, const char* str);
	//RTCDevice initializeDevice();
	//void initializeScene(RTCDevice device);
	
	std::shared_ptr<Scene> scene;
	std::shared_ptr<Integrator> integrator;
	//RTCScene scene;

	//Intersection castRay(glm::vec3 origin, glm::vec3 direction);
	//glm::vec3 rayColor(Intersection intersection);

	//void writePixel(glm::vec3 color, int x, int y, int width);
};

