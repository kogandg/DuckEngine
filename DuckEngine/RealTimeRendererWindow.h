#pragma once

#include "Window.h"

#include "stb/stb_image.h"

#include "Shader.h"
#include <array>
#include "Camera.h"
#include "Scene.h"
#include "ECSRegistry.h"
#include "GLFWInputMap.h"

#include "GPUCache.h"

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
	ECS::ECSRegistry registry;
	ECS::TransformSystem transformSystem;
	ECS::CameraSystem cameraSystem;
	GPUCache cache;

	ECS::Entity directionalLight;


	ECS::Entity cameraEntity;
	ECS::CameraController cameraController;

	InputManager inputManager;
	GLFWInputMap inputMap;

	GLuint VBO;
	GLuint cubeVAO;
	GLuint lightVAO;

	Shader* lightingShader;
	Shader* lightCubeShader;
	
	GLuint diffuseMap;
	GLuint specularMap;

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

	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	void initGLObjects();

	void drawDebugUI(ECS::ECSRegistry& registry, GPUCache& cache);
	void drawEntityNode(ECS::ECSRegistry& registry, ECS::Entity e);
	void drawTransform(ECS::Transform& t);
	void drawMaterial(ECS::Material& m);
	void drawLight(ECS::Light& l);
	void drawCamera(ECS::Camera& c);

	virtual void onCursorPos(double currX, double currY) override;
	virtual void onKey(int key, int scancode, int action, int mods) override;
	virtual void onMouseButton(int button, int action, int mods) override;
};

