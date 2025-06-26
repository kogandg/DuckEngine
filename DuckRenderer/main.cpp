#include <GL/glew.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#define GLFW_DLL
#include <GLFW/glfw3.h>


#include "iostream"
#include <stdio.h>

#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "Window.h"
#include "SelectionWindow.h"
#include "ImageWindow.h"

#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include "OfflineRenderer.h"
#include "WindowManager.h"
#include "OfflineRendererWindow.h"
#include "RayTracerIntegrator.h"
#include "Cube.h"
#include "SceneLoader.h"
#include "RenderTarget.h"

static void errorCallback(int error, const char* description)
{
	std::cerr << "GLFW: " << description << std::endl;
}

void initGLFW()
{
	glfwSetErrorCallback(errorCallback);

	if (!glfwInit())
	{
		throw std::runtime_error("Failed to initialize GLFW");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void initGLEW()
{
	// start GLEW extension handler
	//should be done after windows are created but before any gl objects created
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);
}

//Scene makeScene()
//{
//	Scene scene = Scene();
//	//scene.SetCamera(std::make_shared<Camera>(Camera(glm::vec3(0, -4, 4), glm::vec3(0, -1, 0), glm::vec3(0, 1, 1))));//fov 45
//	scene.SetCamera(std::make_shared<Camera>(Camera(glm::vec3(0, 0, 2), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0), 45)));
//
//	auto triGeom = Triangle(glm::vec3(0, 0, -1.25), glm::vec3(1, 0, -1.25), glm::vec3(0, 1, -1.25));
//	auto sphereGeom = Sphere(glm::vec3(0, 0, -2), 1);
//	auto cubeGeom = Cube();
//
//	Material mat1 = Material(glm::vec3(1, 0, 0), glm::vec3(1, 0, 0), 0, glm::vec3(0));
//	Material mat2 = Material(glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), 0, glm::vec3(0));
//
//	auto tri = std::make_shared<RenderableObject>(RenderableObject("tri", std::make_shared<Triangle>(triGeom), std::make_shared<Material>(mat1)));
//	auto sphere = std::make_shared<RenderableObject>(RenderableObject("sphere", std::make_shared<Sphere>(sphereGeom), std::make_shared<Material>(mat2)));
//	auto cube = std::make_shared<RenderableObject>(RenderableObject("cube", std::make_shared<Cube>(cubeGeom), std::make_shared<Material>(mat2)));
//
//	//scene.AddObject(tri);
//	//scene.AddObject(sphere);
//	scene.AddObject(cube);
//
//	return scene;
//}

int main()
{
	initGLFW();

	SelectionWindow* selWindow = new SelectionWindow("Selection", 600, 400);
	OfflineRendererWindow* rendererWindow = nullptr;

	selWindow->Init(ImVec4(0, 0, 0, 1));

	initGLEW();

	//OfflineRenderer renderer = OfflineRenderer();

	WindowManager manager = WindowManager();
	manager.AddWindow(selWindow);

	while (!manager.SomeClosed())
	{
		// Poll for and process events
		glfwPollEvents();

		if (selWindow->offlinePressed && (rendererWindow == nullptr || !rendererWindow->Initialized()))
		{
			manager.CleanRemoveWindow(selWindow);

			//auto scene = std::make_shared<Scene>(makeScene());
			
			SceneLoader sceneLoader = SceneLoader();
			auto sceneLoadingOutput = sceneLoader.ParseSceneFromTestFile("scene7.txt");
			/*auto scene = std::make_shared<Scene>(sceneLoadingOutput.scene);
			auto target = std::make_shared<RenderTarget>(sceneLoadingOutput.renderTarget);
			auto integrator = std::make_shared<Integrator>(sceneLoadingOutput.integrator);*/

			auto renderer = std::make_shared<OfflineRenderer>(sceneLoadingOutput.integrator, sceneLoadingOutput.scene);

			rendererWindow = new OfflineRendererWindow("Offline Renderer", renderer, sceneLoadingOutput.renderTarget);
			manager.AddWindow(rendererWindow);
			rendererWindow->Init();
			rendererWindow->InitObjects();
			rendererWindow->StartRendering();

			//worker = std::thread(&OfflineRenderer::Render, &renderer, image, testWidth, testHeight);
		}


		manager.IdleCallback();

		manager.DisplayCallback();
	}

	manager.CleanUp();
	manager.DestroyWindows();

	glfwTerminate();
	return 0;
}