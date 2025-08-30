#include "RealTimeRendererWindow.h"


RealTimeRendererWindow::RealTimeRendererWindow(const char* title, int width, int height) : Window(title, width, height), camera(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), 0.0f)
{
	
}

void RealTimeRendererWindow::Init()
{
	Window::Init();
	// Setup input callbacks
	glfwSetWindowUserPointer(glfwWindow, this);

	glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
}

void RealTimeRendererWindow::CleanUp()
{
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);
	//glDeleteProgram(shaderProgram);

	Window::CleanUp();
}

void RealTimeRendererWindow::IdleCallback()
{
	if (!initialized) return;
	
	float currentTime = static_cast<float>(glfwGetTime());
	deltaTime = currentTime - lastTime;
	lastTime = currentTime;

	float velocity = movementSpeed * deltaTime;
	if (glfwGetKey(glfwWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		camera.SetLookFrom(camera.GetLookFrom() + (camera.GetDirection() * velocity));
	}
	if (glfwGetKey(glfwWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera.SetLookFrom(camera.GetLookFrom() - (camera.GetDirection() * velocity));
	}
	if (glfwGetKey(glfwWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		auto oldLookFrom = camera.GetLookFrom();
		camera.SetLookFrom(camera.GetLookFrom() + (camera.GetRight() * velocity));
		auto newLookFrom = camera.GetLookFrom();

		int count = 0;
	}
	if (glfwGetKey(glfwWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		camera.SetLookFrom(camera.GetLookFrom() - (camera.GetRight() * velocity));
	}
	

	//camera.SetLookFrom(glm::vec3(0.0f, 0.0f, 3.0f));
	//camera.SetPitchYaw(0.f, -90.0f);
	//camera.SetUp(glm::vec3(0.0f, 1.0f, 0.0f));
	//camera.SetFOVY(45.0f);
	camera.SetImageSize(width, height);
	camera.UpdateCameraVectors();

}

void RealTimeRendererWindow::DisplayCallback()
{
	if (!Window::Begin()) return;

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	//cube
	lightingShader->Use();
	lightingShader->SetMat4("view", camera.GetViewMatrix());
	lightingShader->SetMat4("projection", camera.GetProjectionMatrix());

	lightingShader->SetVec3("objectColor", 1.0f, 0.5f, 0.31f);
	lightingShader->SetVec3("lightColor", 1.0f, 1.0f, 1.0f);

	glm::mat4 model = glm::mat4(1.0f);
	lightingShader->SetMat4("model", model);

	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	//light cube
	lightCubeShader->Use();
	lightCubeShader->SetMat4("view", camera.GetViewMatrix());
	lightCubeShader->SetMat4("projection", camera.GetProjectionMatrix());

	model = glm::mat4(1.0f);
	model = glm::translate(model, lightPosition);
	model = glm::scale(model, glm::vec3(0.2f));
	lightCubeShader->SetMat4("model", model);

	glBindVertexArray(lightVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	// bind textures on corresponding texture units
	/*glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2);*/

	/*glBindVertexArray(VAO);
	for (int i = 0; i < 10; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, cubePositions[i]);
		float angle = 20.0f * i;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1, 0.3f, 0.5f));
		shader->SetMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);
	}*/

	
	ImGui::SetNextWindowPos(ImVec2(0, 0));// , ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(300, 400));
	ImGui::Begin("Camera info", nullptr);

	auto camPos = camera.GetLookFrom();
	auto camDir = camera.GetDirection();
	auto camRight = camera.GetRight();
	auto camPitch = camera.GetPitch();
	auto camYaw = camera.GetYaw();
	
	ImGui::Text("Pos = (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);
	ImGui::Text("Dir = (%.2f, %.2f, %.2f)", camDir.x, camDir.y, camDir.z);
	ImGui::Text("Right = (%.2f, %.2f, %.2f)", camRight.x, camRight.y, camRight.z);
	ImGui::Text("Pitch = %.2f", camPitch);
	ImGui::Text("Yaw = %.2f", camYaw);

	ImGui::End();

	Window::End();
}

void RealTimeRendererWindow::InitObjects()
{
	initGLObjects();
	//camera.SetImageSize(width, height);
	camera.SetLookFrom(glm::vec3(0.0f, 0.0f, 3.0f));
	camera.SetPitchYaw(0.f, -90.0f);
	camera.SetUp(glm::vec3(0.0f, 1.0f, 0.0f));
	camera.SetFOVY(45.0f);
	camera.SetImageSize(width, height);

	camera.UpdateCameraVectors();
}

void RealTimeRendererWindow::initGLObjects()
{
	//shaders

	lightCubeShader = new Shader("lightCube.vert", "lightCube.frag");
	lightingShader = new Shader("lighting.vert", "lighting.frag");

	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};

	//verts setup
	glGenBuffers(1, &VBO);
	//glGenBuffers(1, &EBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//cube setup
	glGenVertexArrays(1, &cubeVAO);
	glBindVertexArray(cubeVAO);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	int stride = 3 + 2;
	//position attrib
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//texture cord attrib
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	//lightcube setup
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//position attrib
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//texture

	//stbi_set_flip_vertically_on_load(true);

	//int texWidth;
	//int texHeight;
	//int texNumChannels;
	//unsigned char* texData = stbi_load("container.jpg", &texWidth, &texHeight, &texNumChannels, 0);

	//glGenTextures(1, &texture1);
	//glBindTexture(GL_TEXTURE_2D, texture1);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//if (texData)
	//{
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
	//	glGenerateMipmap(GL_TEXTURE_2D);
	//}
	//else
	//{
	//	std::cout << "Failed to load texture" << std::endl;
	//}

	//stbi_image_free(texData);


	////texture2
	//texData = stbi_load("out12.png", &texWidth, &texHeight, &texNumChannels, 0);

	//glGenTextures(1, &texture2);
	//glBindTexture(GL_TEXTURE_2D, texture2);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//if (texData)
	//{
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
	//	glGenerateMipmap(GL_TEXTURE_2D);
	//}
	//else
	//{
	//	std::cout << "Failed to load texture" << std::endl;
	//}

	//stbi_image_free(texData);

	////shader->Use();
	////shader->SetInt("texture1", 0);
	////shader->SetInt("texture2", 1);

	//stbi_set_flip_vertically_on_load(false);//just in case anywhere else using loading
}

void RealTimeRendererWindow::onCursorPos(double currX, double currY)
{
	ImGui::SetCurrentContext(imguiContext);
	ImGui_ImplGlfw_CursorPosCallback(glfwWindow, currX, currY);

	if (firstMouse)
	{
		lastX = currX;
		lastY = currY;
		firstMouse = false;
	}

	float xOffset = currX - lastX;
	float yOffset = lastY - currY;

	lastX = currX;
	lastY = currY;

	//mouse movement
	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	float newPitch = camera.GetPitch() + yOffset;
	if (newPitch > 89.0f)
	{
		newPitch = 89.0f;
	}
	if (newPitch < -89.0f)
	{
		newPitch = -89.0f;
	}

	camera.SetPitchYaw(newPitch, camera.GetYaw()+xOffset);

	camera.UpdateCameraVectors();
}


