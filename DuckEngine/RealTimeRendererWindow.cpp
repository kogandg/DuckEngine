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

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	triShader->Use();
	triShader->SetMat4("view", camera.GetViewMatrix());
	triShader->SetMat4("projection", camera.GetProjectionMatrix());

	for (auto triangle : triangles)
	{
		glBindVertexArray(triangle.VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	////cube
	//lightingShader->Use();

	//lightingShader->SetFloat("material.shininess", 32.0f);

	//lightingShader->SetMat4("view", camera.GetViewMatrix());
	//lightingShader->SetMat4("projection", camera.GetProjectionMatrix());
	//lightingShader->SetVec3("viewPos", camera.GetLookFrom());

	////directional light
	//lightingShader->SetVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
	//lightingShader->SetVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
	//lightingShader->SetVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
	//lightingShader->SetVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

	////point lights
	//for (int i = 0; i < 4; i++)
	//{
	//	std::string index = std::to_string(i);
	//	lightingShader->SetVec3(("pointLights[" + index + "].position").c_str(), pointLightPositions[i]);
	//	lightingShader->SetVec3(("pointLights[" + index + "].ambient").c_str(), 0.05f, 0.05f, 0.05f);
	//	lightingShader->SetVec3(("pointLights[" + index + "].diffuse").c_str(), 0.8f, 0.8f, 0.8f);
	//	lightingShader->SetVec3(("pointLights[" + index + "].specular").c_str(), 1.0f, 1.0f, 1.0f);
	//	lightingShader->SetFloat(("pointLights[" + index + "].constant").c_str(), 1.0f);
	//	lightingShader->SetFloat(("pointLights[" + index + "].linear").c_str(), 0.09f);
	//	lightingShader->SetFloat(("pointLights[" + index + "].quadratic").c_str(), 0.032f);
	//}

	////spotlight
	//lightingShader->SetVec3("spotLight.position", camera.GetLookFrom());
	//lightingShader->SetVec3("spotLight.direction", camera.GetDirection());
	//lightingShader->SetVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
	//lightingShader->SetVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
	//lightingShader->SetVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
	//lightingShader->SetFloat("spotLight.constant", 1.0f);
	//lightingShader->SetFloat("spotLight.linear", 0.09f);
	//lightingShader->SetFloat("spotLight.quadratic", 0.032f);
	//lightingShader->SetFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
	//lightingShader->SetFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
	//

	////bind diffuse map
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, diffuseMap);

	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, specularMap);

	//glBindVertexArray(cubeVAO);
	//for (int i = 0; i < 10; i++)
	//{
	//	glm::mat4 model = glm::mat4(1.0f);
	//	model = glm::translate(model, cubePositions[i]);
	//	float angle = 20.0f * i;
	//	model = glm::rotate(model, glm::radians(angle), glm::vec3(1, 0.3f, 0.5f));
	//	lightingShader->SetMat4("model", model);

	//	glm::mat3 transInversemodel = glm::transpose(glm::inverse(model));
	//	lightingShader->SetMat3("transposeInverseModel", transInversemodel);

	//	glDrawArrays(GL_TRIANGLES, 0, 36);
	//}

	////glBindVertexArray(cubeVAO);
	////glDrawArrays(GL_TRIANGLES, 0, 36);

	////light cube
	//lightCubeShader->Use();
	//lightCubeShader->SetMat4("view", camera.GetViewMatrix());
	//lightCubeShader->SetMat4("projection", camera.GetProjectionMatrix());

	//glBindVertexArray(lightVAO);
	//for (int i = 0; i < 4; i++)
	//{
	//	glm::mat4 model = glm::mat4(1.0f);
	//	model = glm::translate(model, pointLightPositions[i]);
	//	model = glm::scale(model, glm::vec3(0.2f));
	//	lightCubeShader->SetMat4("model", model);
	//	glDrawArrays(GL_TRIANGLES, 0, 36);
	//}

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
	ImGui::SetNextWindowSize(ImVec2(225, 250));
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
	camera.SetLookFrom(glm::vec3(1.0f, 1.0f, 4.0f));
	camera.SetPitchYaw(-12.5f, -101.0f);
	camera.SetUp(glm::vec3(0.0f, 1.0f, 0.0f));
	camera.SetFOVY(45.0f);
	camera.SetImageSize(width, height);

	camera.UpdateCameraVectors();
}

void RealTimeRendererWindow::initGLObjects()
{
	//shaders

	SceneLoader sceneLoader = SceneLoader();
	auto sceneLoadingOutput = sceneLoader.ParseSceneFromTestFile("scene7.txt");

	auto scene = sceneLoadingOutput.scene.get();
	auto renderTarget = sceneLoadingOutput.renderTarget.get();
	auto integrator = sceneLoadingOutput.integrator.get();

	camera = *(scene->GetCamera().get());
	auto objects = scene->GetRenderableObjects();

	for (auto object : objects)
	{
		auto triangle = object.get();
		auto geom = static_cast<Triangle*>(triangle->geometry.get());

		tri newTri;
		glGenBuffers(1, &newTri.VBO);
		glBindBuffer(GL_ARRAY_BUFFER, newTri.VBO);

		float verts[]{
			geom->v1.x, geom->v1.y, geom->v1.z, geom->v2.x, geom->v2.y, geom->v2.z, geom->v3.x, geom->v3.y, geom->v3.z
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

		glGenVertexArrays(1, &newTri.VAO);
		glBindVertexArray(newTri.VAO);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		int count = 0;
	}

	triShader = new Shader("tri.vert", "tri.frag");
}

GLuint RealTimeRendererWindow::loadTexture(const char* path)
{
	GLuint textureID;
	glGenTextures(1, &textureID);

	int width;
	int height;
	int numComponents;
	unsigned char* data = stbi_load(path, &width, &height, &numComponents, 0);
	if (data)
	{
		GLenum format;
		if (numComponents == 1) format = GL_RED;
		else if (numComponents == 3) format = GL_RGB;
		else if (numComponents == 4) format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
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


