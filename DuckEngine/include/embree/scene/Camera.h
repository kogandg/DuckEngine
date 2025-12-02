#pragma once

#include "SceneObject.h"
#include <glm/gtc/matrix_transform.hpp>

class Camera : public SceneObject
{
public:
	Camera(glm::vec3 lookFrom, glm::vec3 lookAt, glm::vec3 up, float fovy);
	Camera(glm::vec3 lookFrom, glm::vec3 up, float fovy, float pitch, float yaw);

	void Initialize(int imagePixelWidth, int imagePixelHeight);
	void Initialize(int imagePixelWidth, float aspectRatio);

	std::pair<glm::vec3, glm::vec3> GetRay(int x, int y);

	void UpdateCameraVectors();

	inline int GetPixelWidth() { return imagePixelWidth; }
	inline int GetPixelHeight() { return imagePixelHeight; }

	inline glm::vec3 GetLookFrom() { return lookFrom; }
	inline glm::vec3 GetLookAt() { return lookAt; }
	inline glm::vec3 GetDirection() { return glm::normalize(lookAt - lookFrom); }
	inline glm::vec3 GetUp() { return up; }
	inline glm::vec3 GetRight() { return right; }
	inline float GetPitch() { return pitch; }
	inline float GetYaw() { return yaw; }

	inline void SetLookFrom(glm::vec3 lookFrom) { this->lookFrom = lookFrom; }
	void SetLookAt(glm::vec3 lookAt);
	inline void SetUp(glm::vec3 up) { this->up = up; }
	inline void SetFOVY(float fovY) { this->fovY = fovY; }
	void SetImageSize(int width, int height);
	void SetPitchYaw(float pitch, float yaw);

	inline glm::mat4 GetViewMatrix() { return viewMatrix; }
	inline glm::mat4 GetProjectionMatrix() { return projectionMatrix; }

private:

	void initialize();

	int imagePixelWidth = 0.0f;
	int imagePixelHeight = 0.0f;

	glm::vec3 lookFrom;//center
	glm::vec3 lookAt;
	glm::vec3 up;

	glm::vec3 right;

	float yaw;
	float pitch;

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

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
};

