#include "Camera.h"

Camera::Camera(glm::vec3 lookFrom, glm::vec3 lookAt, glm::vec3 up)
{
	this->lookFrom = lookFrom;
	this->lookAt = lookAt;
	this->up = up;
}

void Camera::Initialize(int imagePixelWidth, int imagePixelHeight)
{
	this->imagePixelWidth = imagePixelWidth;
	this->imagePixelHeight = imagePixelHeight;
	aspectRatio = (float)imagePixelWidth / (float)imagePixelHeight;

	initialize();
}

void Camera::Initialize(int imagePixelWidth, float aspectRatio)
{
	this->imagePixelWidth = imagePixelWidth;
	this->aspectRatio = aspectRatio;

	imagePixelHeight = (int)(imagePixelWidth/aspectRatio);
	if (imagePixelHeight < 1)
	{
		imagePixelHeight = 1;
	}
	
	initialize();
}

std::pair<glm::vec3, glm::vec3> Camera::GetRay(int x, int y)
{
	glm::vec3 pixelCenter = pixel0Location + (pixelDeltaU * (float)x) + (pixelDeltaV * (float)y);
	glm::vec3 rayDirection = pixelCenter - lookFrom;

	return std::make_pair(lookFrom, rayDirection);//not sure if it should be from pixelCenter or lookFrom but doesnt matter
}

void Camera::initialize()
{
	float focalLength = 1.0f;
	float theta = fovY / 57.2957795;//degrees to radians
	float h = std::tan(theta / 2.0f);

	viewportHeight = 2.0f * h * focalLength;
	viewportWidth = viewportHeight * aspectRatio;

	w = glm::normalize(lookFrom - lookAt);
	u = glm::cross(up, w);
	v = glm::cross(w, u);

	viewportU = u * viewportWidth;
	viewportV = (-v) * viewportHeight;

	viewportUpperLeft = lookFrom - w - (viewportU / 2.0f) - (viewportV / 2.0f);

	pixelDeltaU = viewportU / (float)imagePixelWidth;
	pixelDeltaV = viewportV / (float)imagePixelHeight;

	pixel0Location = viewportUpperLeft + (0.5f * (pixelDeltaU + pixelDeltaV));
}
