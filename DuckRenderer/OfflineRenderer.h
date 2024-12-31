#pragma once

#include <iostream>

#include <embree4/rtcore.h>
#include <glm/glm.hpp>

struct Intersection
{
	glm::vec3 origin = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(0.0f);
	glm::vec3 hit = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f);
	float t = std::numeric_limits<float>::infinity();
	unsigned int id = RTC_INVALID_GEOMETRY_ID;
};

class OfflineRenderer
{
public:
	void Render(int width, int height);

	inline bool DoneRendering() { return started && !running; }

	inline unsigned char* GetImage() { return image; }
	
	inline void Cancel() { cancel = true; }

	inline bool Started() { return started; }

private:

	unsigned char* image;

	bool started = false;
	bool running = false;
	bool cancel = false;

	//void errorFunction(void* userPtr, enum RTCError error, const char* str);
	RTCDevice initializeDevice();
	RTCScene initializeScene(RTCDevice device);

	RTCScene scene;

	Intersection castRay(glm::vec3 origin, glm::vec3 direction);
	glm::vec3 rayColor(Intersection intersection);

	void writePixel(glm::vec3 color, int x, int y, int width);
};

