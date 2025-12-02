#include <embree/scene/Camera.h>

Camera::Camera(glm::vec3 lookFrom, glm::vec3 lookAt, glm::vec3 up, float fovy)
{
	this->lookFrom = lookFrom;
	this->lookAt = lookAt;
	this->up = up;
	this->fovY = fovy;
}

Camera::Camera(glm::vec3 lookFrom, glm::vec3 up, float fovy, float pitch, float yaw)
{
	this->lookFrom = lookFrom;
	this->up = up;
	this->fovY = fovy;
	this->pitch = pitch;
	this->yaw = yaw;
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
	glm::vec3 pixelCenter = pixel0Location + ((pixelDeltaU * (float)x) + (pixelDeltaV * (float)y));
	glm::vec3 rayDirection = pixelCenter - lookFrom;

	return std::make_pair(pixelCenter, rayDirection);//not sure if it should be from pixelCenter or lookFrom but doesnt matter
}

void Camera::UpdateCameraVectors()
{
	glm::vec3 front;

	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(front);
	lookAt = lookFrom + front;

	right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));//worldUp
	up = glm::normalize(glm::cross(right, front));


	viewMatrix = glm::lookAt(lookFrom, lookAt, up);
	projectionMatrix = glm::perspective(glm::radians(fovY), aspectRatio, 0.01f, 100.0f);
}

void Camera::SetLookAt(glm::vec3 lookAt)
{
	this->lookAt = lookAt;

	glm::vec3 direction = lookAt - lookFrom;
	pitch = asin(direction.y);
	yaw = atan2(direction.z, direction.x);
}

void Camera::SetImageSize(int width, int height)
{
	imagePixelWidth = width;
	imagePixelHeight = height;
	aspectRatio = (float)imagePixelWidth / (float)imagePixelHeight;
}

void Camera::SetPitchYaw(float pitch, float yaw)
{
	this->pitch = pitch;
	this->yaw = yaw;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction = glm::normalize(direction);

	lookAt = lookFrom + direction;
}

void Camera::initialize()
{
	float focalLength = 1.0f;
	float theta = fovY / 57.2957795;//degrees to radians
	float h = std::tan(theta / 2.0f);

	viewportHeight = 2.0f * h * focalLength;
	viewportWidth = viewportHeight * aspectRatio;

	w = glm::normalize(lookFrom - lookAt);
	u = glm::normalize(glm::cross(up, w));
	v = glm::normalize(glm::cross(w, u));
	
	viewportU = u * viewportWidth;
	viewportV = (-v) * viewportHeight;

	viewportUpperLeft = lookFrom - w - (viewportU / 2.0f) - (viewportV / 2.0f);

	pixelDeltaU = viewportU / (float)imagePixelWidth;
	pixelDeltaV = viewportV / (float)imagePixelHeight;

	pixel0Location = viewportUpperLeft + (0.5f * (pixelDeltaU + pixelDeltaV));

	viewMatrix = glm::lookAt(lookFrom, lookAt, up);
	projectionMatrix = glm::perspective(glm::radians(fovY), aspectRatio, 0.01f, 100.0f);
}
