#pragma once

#include "Window.h"

#include "stb/stb_image.h"

#include "Shader.h"
#include <array>
#include "Camera.h"
#include "SceneLoader.h"

struct tri
{
	GLuint VBO;
	GLuint VAO;
};

class RealTimeRendererWindow : public Window
{
public:
	RealTimeRendererWindow(const char* title, int width, int height);

	virtual void Init() override;
	virtual void CleanUp() override;

	virtual void IdleCallback() override;
	virtual void DisplayCallback() override;

	virtual void InitObjects();

private:
	GLuint VBO;
	GLuint cubeVAO;
	GLuint lightVAO;
	//GLuint EBO;
	//GLuint shaderProgram;

	Shader* triShader;
	
	GLuint diffuseMap;
	GLuint specularMap;

	std::vector<tri> triangles;

	std::array<glm::vec3, 10> cubePositions = { {
		glm::vec3(0.0f,  0.0f,  0.0f),
		glm::vec3(2.0f,  5.0f, -15.0f),
		glm::vec3(-1.5f, -2.2f, -2.5f),
		glm::vec3(-3.8f, -2.0f, -12.3f),
		glm::vec3(2.4f, -0.4f, -3.5f),
		glm::vec3(-1.7f,  3.0f, -7.5f),
		glm::vec3(1.3f, -2.0f, -2.5f),
		glm::vec3(1.5f,  2.0f, -2.5f),
		glm::vec3(1.5f,  0.2f, -1.5f),
		glm::vec3(-1.3f,  1.0f, -1.5f)
	} };

	std::array<glm::vec3, 4> pointLightPositions = { {
		glm::vec3(0.7f,  0.2f,  2.0f),
		glm::vec3(2.3f, -3.3f, -4.0f),
		glm::vec3(-4.0f,  2.0f, -12.0f),
		glm::vec3(0.0f,  0.0f, -3.0f)
	} };

	glm::vec3 lightPosition = glm::vec3(1.2f, 1.0f, 2.0f);

	Camera camera;

	float movementSpeed = 2.5f;
	float mouseSensitivity = 0.1f;

	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	void initGLObjects();

	GLuint loadTexture(const char* path);

	bool firstMouse = true;
	float lastX;
	float lastY;
	virtual void onCursorPos(double currX, double currY) override;
};

