#pragma once

#include <iostream>

#include <embree4/rtcore.h>
#include <glm/glm.hpp>

#include "Intersection.h"
#include "Triangle.h"
#include "Scene.h"
#include "RenderableObject.h"

#include <thread>
#include <chrono>


class OfflineRenderer
{
public:
	OfflineRenderer(int maxDepth = 5);

	void Render(int imageWidth, int imageHeight);

	inline bool DoneRendering() { return started && !running; }
	inline unsigned char* GetImage() { return image; }
	inline void Cancel() { cancel = true; }
	inline bool Started() { return started; }

	inline int GetPixelWidth() { return scene.GetCamera().get()->GetPixelWidth(); }
	inline int GetPixelHeight() { return scene.GetCamera().get()->GetPixelHeight(); }

private:

	std::shared_ptr<RenderableObject> tri;

	unsigned char* image;

	bool started = false;
	bool running = false;
	bool cancel = false;

	int maxDepth;

	//void errorFunction(void* userPtr, enum RTCError error, const char* str);
	RTCDevice initializeDevice();
	void initializeScene(RTCDevice device);
	
	Scene scene;
	//RTCScene scene;

	//Intersection castRay(glm::vec3 origin, glm::vec3 direction);
	glm::vec3 rayColor(Intersection intersection);

	void writePixel(glm::vec3 color, int x, int y, int width);
};

