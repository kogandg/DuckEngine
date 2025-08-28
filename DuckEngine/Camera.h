#pragma once

#include "SceneObject.h"

class Camera : public SceneObject
{
public:
	Camera(glm::vec3 lookFrom, glm::vec3 lookAt, glm::vec3 up, float fovy);

	void Initialize(int imagePixelWidth, int imagePixelHeight);
	void Initialize(int imagePixelWidth, float aspectRatio);

	std::pair<glm::vec3, glm::vec3> GetRay(int x, int y);

	inline int GetPixelWidth() { return imagePixelWidth; }
	inline int GetPixelHeight() { return imagePixelHeight; }

private:

	void initialize();

	int imagePixelWidth = 0.0f;
	int imagePixelHeight = 0.0f;

	glm::vec3 lookFrom;//center
	glm::vec3 lookAt;
	glm::vec3 up;

	float fovY;//this is weird, need to test with real scenes
	float aspectRatio = 0.0f;

	//local space
	glm::vec3 u;
	glm::vec3 v;
	glm::vec3 w;//direction

	float viewportHeight;
	float viewportWidth;

	glm::vec3 viewportU;
	glm::vec3 viewportV;
	glm::vec3 viewportUpperLeft;

	glm::vec3 pixelDeltaU;
	glm::vec3 pixelDeltaV;
	glm::vec3 pixel0Location;
};

