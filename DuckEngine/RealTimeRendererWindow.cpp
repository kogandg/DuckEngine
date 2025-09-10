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

	auto cubeMesh = std::make_shared<ECS::MeshData>(ECS::MeshData({
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
		}));

	auto lightCubeMat = ECS::MaterialData(ECS::MaterialType::Color, ECS::ColorData(glm::vec3(1.0f)));
	auto lightCubeMaterial = std::make_shared<ECS::MaterialData>(lightCubeMat);
	auto cubeMat = ECS::MaterialData(ECS::MaterialType::Phong, ECS::PhongData("container2.png", "container2_specular.png"));
	auto cubeMaterial = std::make_shared<ECS::MaterialData>(cubeMat);


	ECS::Mesh mesh;
	mesh.data = cubeMesh;

	ECS::Material cubeMatComp;
	cubeMatComp.data = cubeMaterial;
	for (int i = 0; i < 10; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, cubePositions[i]);
		float angle = 20.0f * i;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1, 0.3f, 0.5f));

		ECS::Transform transform;
		transform.local = glm::mat4(1.0f);
		transform.world = model;

		auto cube = ecs.CreateEntity();

		ecs.addComponent<ECS::Mesh>(cube, mesh);
		ecs.addComponent<ECS::Transform>(cube, transform);
		ecs.addComponent<ECS::Material>(cube, cubeMatComp);
	}

	ECS::Material lightCubeMatComp;
	lightCubeMatComp.data = lightCubeMaterial;
	for (int i = 0; i < 4; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, pointLightPositions[i]);
		model = glm::scale(model, glm::vec3(0.2f));

		ECS::Transform transform;
		transform.local = glm::mat4(1.0f);
		transform.world = model;

		auto lightCube = ecs.CreateEntity();

		ecs.addComponent<ECS::Mesh>(lightCube, mesh);
		ecs.addComponent<ECS::Transform>(lightCube, transform);
		ecs.addComponent<ECS::Material>(lightCube, lightCubeMatComp);
	}
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

	for (auto& [entity, meshComp] : ecs.view<ECS::Mesh>())
	{
		auto& matComp = ecs.getComponent<ECS::Material>(entity);
		auto& transform = ecs.getComponent<ECS::Transform>(entity);

		auto geom = cache.geomCache[meshComp.data];
		auto gpuMat = cache.materialCache[matComp.data];
		auto shader = gpuMat.get()->shader.get();
		GLuint vao = GPUScene::getOrCreateVAO(cache, geom, gpuMat.get()->shader);

		shader->Use();
		glBindVertexArray(vao);

		//uniforms
		switch (gpuMat.get()->type)
		{
		case GPUScene::MaterialType::Color:
		{
			shader->SetMat4("view", camera.GetViewMatrix());
			shader->SetMat4("projection", camera.GetProjectionMatrix());

			shader->SetMat4("model", transform.world);

			shader->SetVec3("color", glm::vec3(1.0f));
		}break;
		case GPUScene::MaterialType::Phong:
		{
			shader->SetMat4("view", camera.GetViewMatrix());
			shader->SetMat4("projection", camera.GetProjectionMatrix());

			shader->SetMat4("model", transform.world);
			shader->SetMat3("transposeInverseModel", glm::transpose(glm::inverse(transform.world)));

			shader->SetFloat("material.shininess", 32.0f);
			shader->SetInt("material.diffuse", 0);
			shader->SetInt("material.specular", 1);

			//bind textures
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseMap);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, specularMap);

			shader->SetVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
			shader->SetVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
			shader->SetVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
			shader->SetVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

			//point lights
			for (int i = 0; i < 4; i++)
			{
				std::string index = std::to_string(i);
				shader->SetVec3(("pointLights[" + index + "].position").c_str(), pointLightPositions[i]);
				shader->SetVec3(("pointLights[" + index + "].ambient").c_str(), 0.05f, 0.05f, 0.05f);
				shader->SetVec3(("pointLights[" + index + "].diffuse").c_str(), 0.8f, 0.8f, 0.8f);
				shader->SetVec3(("pointLights[" + index + "].specular").c_str(), 1.0f, 1.0f, 1.0f);
				shader->SetFloat(("pointLights[" + index + "].constant").c_str(), 1.0f);
				shader->SetFloat(("pointLights[" + index + "].linear").c_str(), 0.09f);
				shader->SetFloat(("pointLights[" + index + "].quadratic").c_str(), 0.032f);
			}

			//spotlight
			shader->SetVec3("spotLight.position", camera.GetLookFrom());
			shader->SetVec3("spotLight.direction", camera.GetDirection());
			shader->SetVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
			shader->SetVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
			shader->SetVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
			shader->SetFloat("spotLight.constant", 1.0f);
			shader->SetFloat("spotLight.linear", 0.09f);
			shader->SetFloat("spotLight.quadratic", 0.032f);
			shader->SetFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
			shader->SetFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
		}break;
		}

		glDrawArrays(GL_TRIANGLES, 0, geom.get()->vertexCount);
	}

	////cube
	//lightingShader->Use();

	//lightingShader->SetFloat("material.shininess", 32.0f);
	//lightingShader->SetInt("material.diffuse", 0);
	//lightingShader->SetInt("material.specular", 1);

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

	GPUScene::preloadGPU(ecs, cache);

	//shaders

	lightCubeShader = new Shader("lightCube.vert", "lightCube.frag");
	lightingShader = new Shader("lighting.vert", "lighting.frag");

	float vertices[] = {
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
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

	int stride = 3 + 3 + 2;
	//position attrib
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//normal attrib
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//tex coords attrib
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);


	//lightcube setup
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//position attrib
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//texture
	diffuseMap = loadTexture("container2.png");
	specularMap = loadTexture("container2_specular.png");

	//lightingShader->Use();
	//lightingShader->SetInt("material.diffuse", 0);
	//lightingShader->SetInt("material.specular", 1);

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

	camera.SetPitchYaw(newPitch, camera.GetYaw() + xOffset);

	camera.UpdateCameraVectors();
}

std::shared_ptr<GPUScene::GPUGeometry> GPUScene::getOrCreateGeometry(GPUCache& c, const std::shared_ptr<ECS::MeshData>& md, const VertexLayoutKey& layout)
{
	auto iter = c.geomCache.find(md);
	if (iter != c.geomCache.end())
	{
		return c.geomCache[md];
	}

	auto geom = GPUGeometry();
	geom.layout = layout;

	glGenBuffers(1, &(geom.VBO));

	glBindBuffer(GL_ARRAY_BUFFER, geom.VBO);
	glBufferData(GL_ARRAY_BUFFER, md.get()->verts.size() * sizeof(float), md.get()->verts.data(), GL_STATIC_DRAW);

	geom.vertexCount = (GLsizei)(md.get()->verts.size() * sizeof(float) / geom.layout.stride);

	auto geomPointer = std::make_shared<GPUGeometry>(geom);
	return c.geomCache[md] = geomPointer;
}

std::shared_ptr<Shader> GPUScene::getOrCreateShader(GPUCache& c, MaterialType type, const char* vertexPath, const char* fragmentPath)
{
	auto iter = c.shaderCache.find(type);
	if (iter != c.shaderCache.end())
	{
		return c.shaderCache[type];
	}

	auto shader = std::make_shared<Shader>(vertexPath, fragmentPath);

	switch (type)
	{
	case MaterialType::Color:
	{
		shader.get()->CacheUniform("view");
		shader.get()->CacheUniform("projection");
		shader.get()->CacheUniform("model");
		shader.get()->CacheUniform("color");
	} break;
	case MaterialType::Phong:
	{
		shader.get()->CacheUniform("view");
		shader.get()->CacheUniform("projection");
		shader.get()->CacheUniform("model");
		shader.get()->CacheUniform("transposeInverseModel");
		shader.get()->CacheUniform("viewPos");

		//can set once but since this same shader can be used for lots of objects with different vals its better to set each frame
		shader.get()->CacheUniform("material.shininess");
		shader.get()->CacheUniform("material.diffuse");
		shader.get()->CacheUniform("material.specular");

		//directional light
		shader.get()->CacheUniform("dirLight.direction");
		shader.get()->CacheUniform("dirLight.ambient");
		shader.get()->CacheUniform("dirLight.diffuse");
		shader.get()->CacheUniform("dirLight.specular");

		//point lights
		for (int i = 0; i < 4; i++)
		{
			std::string index = std::to_string(i);
			shader.get()->CacheUniform(("pointLights[" + index + "].position").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].ambient").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].diffuse").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].specular").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].constant").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].linear").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].quadratic").c_str());
		}

		//spotlight
		shader.get()->CacheUniform("spotLight.position");
		shader.get()->CacheUniform("spotLight.direction");
		shader.get()->CacheUniform("spotLight.ambient");
		shader.get()->CacheUniform("spotLight.diffuse");
		shader.get()->CacheUniform("spotLight.specular");
		shader.get()->CacheUniform("spotLight.constant");
		shader.get()->CacheUniform("spotLight.linear");
		shader.get()->CacheUniform("spotLight.quadratic");
		shader.get()->CacheUniform("spotLight.cutOff");
		shader.get()->CacheUniform("spotLight.outerCutOff");
	} break;
	}

	return c.shaderCache[type] = shader;
}

std::shared_ptr<GLuint> GPUScene::getOrCreateTexture(GPUCache& c, const std::string& path)
{
	if (path.empty()) return 0;
	auto iter = c.textureCache.find(path);
	if (iter != c.textureCache.end())
	{
		return c.textureCache[path];
	}

	GLuint texture;
	glGenTextures(1, &texture);

	int width;
	int height;
	int numComponents;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &numComponents, 0);
	if (data)
	{
		GLenum format;
		if (numComponents == 1) format = GL_RED;
		else if (numComponents == 3) format = GL_RGB;
		else if (numComponents == 4) format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, texture);
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

	auto texturePointer = std::make_shared<GLuint>(texture);
	c.textureCache[path] = texturePointer;

	return texturePointer;
}

std::shared_ptr<GPUScene::GPUMaterial> GPUScene::getOrCreateMaterial(GPUCache& c, const std::shared_ptr<ECS::MaterialData>& md)
{
	auto iter = c.materialCache.find(md);
	if (iter != c.materialCache.end())
	{
		return c.materialCache[md];
	}

	auto material = std::make_shared<GPUMaterial>();
	switch (md.get()->materialType)
	{
	case ECS::MaterialType::Color:
	{
		material.get()->type = MaterialType::Color;
		material.get()->shader = getOrCreateShader(c, MaterialType::Color, "color.vert", "color.frag");
		auto& d = std::get<ECS::ColorData>(md.get()->data);
		material.get()->color = d.color;
	} break;
	case ECS::MaterialType::Phong:
	{
		material.get()->type = MaterialType::Phong;
		material.get()->shader = getOrCreateShader(c, MaterialType::Phong, "lighting.vert", "lighting.frag");
		auto& d = std::get<ECS::PhongData>(md.get()->data);
		material.get()->diffuseTexture = getOrCreateTexture(c, d.diffuseTexturePath);
		material.get()->specularTexture = getOrCreateTexture(c, d.specularTexturePath);
		material.get()->shininess = d.shininess;
	}break;
	}

	return c.materialCache[md] = material;
}

GLuint GPUScene::buildVAO(const GPUGeometry& g, const Shader& s)
{
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, g.VBO);
	//if (g.ebo) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.ebo); //later for ebos

	for (const auto& attr : g.layout.attrs)
	{
		glEnableVertexAttribArray(attr.location);
		glVertexAttribPointer(attr.location, attr.size, attr.type, GL_FALSE, g.layout.stride, (void*)(attr.offset));
	}

	return vao;
}

GLuint GPUScene::getOrCreateVAO(GPUCache& c, const std::shared_ptr<GPUGeometry>& g, const std::shared_ptr<Shader>& s)
{
	VAOKey key{ g.get()->VBO, g.get()->layout };

	auto iter = c.vaoCache.find(key);
	if (iter != c.vaoCache.end())
	{
		c.vaoCache[key];
	}

	VAOEntry entry{ buildVAO(*g, *s) };
	c.vaoCache[key] = entry;

	return entry.VAO;
}

void GPUScene::preloadGPU(ECS::ECS& ecs, GPUCache& cache)
{
	for (auto& [entity, meshComp] : ecs.view<ECS::Mesh>())
	{
		auto& matComp = ecs.getComponent<ECS::Material>(entity);

		VertexLayoutKey layout
		{
			sizeof(float) * 8, //stride = pos(3) + norm(3) + text coords(2)
			{
			{0, 3, GL_FLOAT, 0},
			{1, 3, GL_FLOAT, sizeof(float) * 3},
			{2, 2, GL_FLOAT, sizeof(float) * 6}
			}
		};
		auto geom = getOrCreateGeometry(cache, meshComp.data, layout);

		auto gpuMat = getOrCreateMaterial(cache, matComp.data);

		GLuint vao = getOrCreateVAO(cache, geom, gpuMat.get()->shader);
	}
}
