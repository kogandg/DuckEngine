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



unsigned char* image;

//void testThread()
//{
//	for (int i = 0; i < 10; i++)
//	{
//		bufferMutex.lock();
//		buffer += "test";
//		bufferMutex.unlock();
//		std::this_thread::sleep_for(std::chrono::seconds(2));
//	}
//
//	std::cout << "done" << std::endl;
//	running = false;
//}
//
//// Generate a checkerboard pattern for the texture
//void generateCheckerboard(unsigned char* image, int width, int height)
//{
//	running = true;
//	started = true;
//
//	for (int y = 0; y < height; y++)
//	{
//		for (int x = 0; x < width; x++)
//		{
//			int index = (y * width + x) * 3; // 3 channels (RGB) per pixel
//			bool isWhite = ((x / 16) % 2 == (y / 16) % 2); // 16-pixel checkerboard pattern
//			//bufferMutex.lock();
//			image[index + 0] = isWhite ? 255 : 0;  // Red
//			image[index + 1] = isWhite ? 255 : 0;  // Green
//			image[index + 2] = isWhite ? 255 : 0;  // Blue
//			//bufferMutex.unlock();
//
//			if (cancelThread)
//			{
//				return;
//			}
//		}
//		//std::this_thread::sleep_for(std::chrono::milliseconds(10));
//	}
//
//	std::cout << "Done Rendering!" << std::endl;
//	running = false;
//}
//
//void generateGradient(unsigned char* image, int width, int height)
//{
//	running = true;
//	started = true;
//
//	for (int y = 0; y < height; y++)
//	{
//		for (int x = 0; x < width; x++)
//		{
//			int index = (y * width + x) * 3; // 3 channels (RGB) per pixel
//			//bool isWhite = ((x / 16) % 2 == (y / 16) % 2); // 16-pixel checkerboard pattern
//			//bufferMutex.lock();
//
//			auto r = double(x) / (width - 1);
//			auto g = double(y) / (height - 1);
//			auto b = 0.0;
//
//			image[index + 0] = int(255.99 * r);  // Red
//			image[index + 1] = int(255.99 * g);  // Green
//			image[index + 2] = int(255.99 * b);  // Blue
//			//bufferMutex.unlock();
//
//			if (cancelThread)
//			{
//				return;
//			}
//		}
//		//std::this_thread::sleep_for(std::chrono::milliseconds(10));
//	}
//
//	std::cout << "Done Rendering!" << std::endl;
//	running = false;
//}

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

int main()
{
	initGLFW();

	int testWidth = 256;
	int testHeight = 256;
	image = new unsigned char[testWidth * testHeight * 3];
	for (int i = 0; i < testWidth * testHeight * 3; i++)
	{
		image[i] = 0;
	}

	SelectionWindow* selWindow = new SelectionWindow("Selection", 400, 400);
	//ImageWindow* imageWindow = new ImageWindow("Renderer", testWidth * 2, testHeight * 2);
	OfflineRendererWindow* rendererWindow = new OfflineRendererWindow("Offline Renderer", testWidth*2, testHeight*2);

	//selWindow->Init(ImVec4(0.2f, 0.3f, 0.3f, 1.0f));
	selWindow->Init(ImVec4(0, 0, 0, 1));

	initGLEW();

	//std::thread worker;

	//OfflineRenderer renderer = OfflineRenderer();

	WindowManager manager = WindowManager();
	manager.AddWindow(selWindow);
	//manager.AddWindow(imageWindow);
	manager.AddWindow(rendererWindow);

	while (!manager.SomeClosed())
	{
		// Poll for and process events
		glfwPollEvents();

		if (selWindow->offlinePressed && !rendererWindow->Initialized())
		{
			manager.CleanRemoveWindow(selWindow);

			rendererWindow->Init();
			rendererWindow->InitObjects();
			rendererWindow->StartRendering(testWidth, testHeight);

			//worker = std::thread(&OfflineRenderer::Render, &renderer, image, testWidth, testHeight);
		}


		selWindow->IdleCallback();
		//imageWindow->IdleCallback(image, testWidth, testHeight);
		rendererWindow->IdleCallback();

		manager.DisplayCallback();
	}

	/*if (worker.joinable())
	{
		renderer.Cancel();
		worker.detach();
	}*/

	manager.CleanUp();
	manager.DestroyWindows();

	glfwTerminate();
	return 0;
}