#include "RealTimeRendererWindow.h"


RealTimeRendererWindow::RealTimeRendererWindow(const char* title, int width, int height) : Window(title, width, height)//, camera(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), 0.0f)
{

}

void RealTimeRendererWindow::Init()
{
	Window::Init();
	// Setup input callbacks
	glfwSetWindowUserPointer(glfwWindow, this);

	//glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	inputMap.setWindow(glfwWindow);

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

	if (glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

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

	auto lightCubeMat = ECS::MaterialData(ECS::MaterialData::Type::Color, ECS::ColorData(glm::vec3(1.0f)));
	auto lightCubeMaterial = std::make_shared<ECS::MaterialData>(lightCubeMat);
	auto cubeMat = ECS::MaterialData(ECS::MaterialData::Type::Phong, ECS::PhongData("container2.png", "container2_specular.png", 32.0f));
	auto cubeMaterial = std::make_shared<ECS::MaterialData>(cubeMat);


	ECS::Mesh mesh;
	mesh.data = cubeMesh;

	ECS::Material cubeMatComp;
	cubeMatComp.data = cubeMaterial;
	for (int i = 0; i < 10; i++)
	{
		//glm::mat4 model = glm::mat4(1.0f);
		//model = glm::translate(model, cubePositions[i]);
		float angle = 20.0f * i;
		//model = glm::rotate(model, glm::radians(angle), glm::vec3(1, 0.3f, 0.5f));

		ECS::Transform transform;
		transform.position = cubePositions[i];
		transform.rotation = glm::angleAxis(glm::radians(angle), glm::normalize(glm::vec3(1, 0.3, 0.5f)));
		//transform.local = glm::mat4(1.0f);
		//transform.world = model;

		auto cube = registry.CreateEntity();

		registry.addComponent<ECS::Mesh>(cube, mesh);
		registry.addComponent<ECS::Transform>(cube, transform);
		registry.addComponent<ECS::Material>(cube, cubeMatComp);
	}

	ECS::Material lightCubeMatComp;
	lightCubeMatComp.data = lightCubeMaterial;
	for (int i = 0; i < 4; i++)
	{
		//glm::mat4 model = glm::mat4(1.0f);
		//model = glm::translate(model, pointLightPositions[i]);
		//model = glm::scale(model, glm::vec3(0.2f));

		ECS::Transform transform;
		transform.position = pointLightPositions[i];
		transform.scale = glm::vec3(0.2f);
		//transform.local = glm::mat4(1.0f);
		//transform.world = model;

		auto lightCube = registry.CreateEntity();

		registry.addComponent<ECS::Mesh>(lightCube, mesh);
		registry.addComponent<ECS::Transform>(lightCube, transform);
		registry.addComponent<ECS::Material>(lightCube, lightCubeMatComp);
	}


	//lights
	ECS::Transform directionalTransform;
	directionalTransform.rotation = glm::quat(glm::vec3(-0.2f, -1.0f, -0.3f));
	ECS::Light directionalLightComp;
	directionalLightComp.type = ECS::Light::Type::Directional;
	directionalLightComp.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
	directionalLightComp.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
	directionalLightComp.specular = glm::vec3(0.5f, 0.5f, 0.5f);

	directionalLight = registry.CreateEntity();
	registry.addComponent<ECS::Light>(directionalLight, directionalLightComp);
	registry.addComponent<ECS::Transform>(directionalLight, directionalTransform);


	ECS::Transform cameraTransform;
	cameraTransform.position = glm::vec3(0.0f, 0.0f, 3.0f);
	cameraTransform.rotation = glm::angleAxis(glm::radians(-101.0f), glm::vec3(0, 1, 0)) * glm::angleAxis(glm::radians(-12.5f), glm::vec3(1, 0, 0));
	ECS::Camera cameraData;
	cameraData.aspectRatio = width / height;
	cameraData.fovY = 45.0f;
	cameraEntity = registry.CreateEntity();
	registry.addComponent<ECS::Camera>(cameraEntity, cameraData);
	registry.addComponent<ECS::Transform>(cameraEntity, cameraTransform);

	cameraController.pitch = 0.0f;
	cameraController.yaw = -90.0f;
	cameraController.init(registry.getComponent<ECS::Transform>(cameraEntity));
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

	
	if (inputManager.WasMousePressed(MouseButton::Right))
	{
		inputMap.setCursorMode(CursorMode::Locked);
		inputManager.UpdateCursorMode(CursorMode::Locked);
	}
	if (inputManager.WasMouseReleased(MouseButton::Right))
	{
		inputMap.setCursorMode(CursorMode::Normal);
		inputManager.UpdateCursorMode(CursorMode::Normal);
	}

	auto& transform = registry.getComponent<ECS::Transform>(cameraEntity);
	cameraController.update(transform, inputManager, deltaTime);

	transformSystem.update(registry);
	cameraSystem.update(registry);

	inputManager.Update();
}

void RealTimeRendererWindow::DisplayCallback()
{
	if (!Window::Begin()) return;

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	auto& cameraData = registry.getComponent<ECS::Camera>(cameraEntity);
	cameraData.aspectRatio = (float)width / (float)height;

	for (auto& [entity, meshComp] : registry.view<ECS::Mesh>())
	{
		auto& matComp = registry.getComponent<ECS::Material>(entity);
		auto& transform = registry.getComponent<ECS::Transform>(entity);

		auto geom = cache.geomCache[meshComp.data];
		auto gpuMat = cache.materialCache[matComp.data].get();
		auto shader = gpuMat->shader.get();
		GLuint vao = GPUScene::getOrCreateVAO(cache, geom, gpuMat->shader);

		shader->Use();
		glBindVertexArray(vao);

		//uniforms
		switch (gpuMat->type)
		{
		case GPUScene::GPUMaterial::Type::Color:
		{
			/*shader->SetMat4("view", camera.GetViewMatrix());
			shader->SetMat4("projection", camera.GetProjectionMatrix());*/

			shader->SetMat4("view", cameraData.view);
			shader->SetMat4("projection", cameraData.projection);

			shader->SetMat4("model", transform.world);

			shader->SetVec3("color", gpuMat->color);
		}break;
		case GPUScene::GPUMaterial::Type::Phong:
		{
			/*shader->SetMat4("view", camera.GetViewMatrix());
			shader->SetMat4("projection", camera.GetProjectionMatrix());*/

			shader->SetMat4("view", cameraData.view);
			shader->SetMat4("projection", cameraData.projection);

			shader->SetMat4("model", transform.world);
			shader->SetMat3("transposeInverseModel", glm::transpose(glm::inverse(transform.world)));

			shader->SetFloat("material.shininess", gpuMat->shininess);
			shader->SetTexture("material.diffuse", 0, *gpuMat->diffuseTexture.get());
			shader->SetTexture("material.specular", 1, *gpuMat->specularTexture.get());

			auto dirLightData = registry.getComponent<ECS::Light>(directionalLight);
			auto quatRotation = registry.getComponent<ECS::Transform>(directionalLight).rotation;
			shader->SetVec3("dirLight.direction", glm::eulerAngles(quatRotation));
			shader->SetVec3("dirLight.ambient", dirLightData.ambient);
			shader->SetVec3("dirLight.diffuse", dirLightData.diffuse);
			shader->SetVec3("dirLight.specular", dirLightData.specular);

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

			////spotlight
			//shader->SetVec3("spotLight.position", camera.GetLookFrom());
			//shader->SetVec3("spotLight.direction", camera.GetDirection());
			//shader->SetVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
			//shader->SetVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
			//shader->SetVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
			//shader->SetFloat("spotLight.constant", 1.0f);
			//shader->SetFloat("spotLight.linear", 0.09f);
			//shader->SetFloat("spotLight.quadratic", 0.032f);
			//shader->SetFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
			//shader->SetFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
		}break;
		}

		glDrawArrays(GL_TRIANGLES, 0, geom.get()->vertexCount);
	}


	ImGui::SetNextWindowPos(ImVec2(0, 0));// , ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(225, 250));
	ImGui::Begin("Camera info", nullptr);

	/*auto camPos = camera.GetLookFrom();
	auto camDir = camera.GetDirection();
	auto camRight = camera.GetRight();
	auto camPitch = camera.GetPitch();
	auto camYaw = camera.GetYaw();

	auto camTestTrans = registry.getComponent<ECS::Transform>(cameraEntity);
	auto camTestPos = camTestTrans.position;
	auto camTestDir = glm::eulerAngles(camTestTrans.rotation);

	ImGui::Text("Pos = (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);
	ImGui::Text("Dir = (%.2f, %.2f, %.2f)", camDir.x, camDir.y, camDir.z);
	ImGui::Text("Right = (%.2f, %.2f, %.2f)", camRight.x, camRight.y, camRight.z);
	ImGui::Text("Pitch = %.2f", camPitch);
	ImGui::Text("Yaw = %.2f", camYaw);*/


	/*ImGui::Text("Test Pos = (%.2f, %.2f, %.2f)", camTestPos.x, camTestPos.y, camTestPos.z);
	ImGui::Text("Test Dir = (%.2f, %.2f, %.2f)", camTestDir.x, camTestDir.y, camTestDir.z);
	ImGui::Text("Test Dir = (%.2f, %.2f, %.2f)", front.x, front.y, front.z);*/
	//ImGui::Text("Test Dir = (%.2f, %.2f, %.2f)", test3.x, test3.y, test3.z);

	auto mousePos = inputManager.GetMousePos();
	auto mouseDelta = inputManager.GetMouseDelta();
	ImGui::Text("Mouse Pos = (%.2f, %.2f)", mousePos.x, mousePos.y);
	ImGui::Text("Mouse Delta = (%.2f, %.2f)", mouseDelta.x, mouseDelta.y);
	ImGui::Text("Cursor mode = %s", inputManager.GetCursorMode() == CursorMode::Normal ? "normal" : "locked");

	glm::vec3 forward = registry.getComponent<ECS::Transform>(cameraEntity).rotation * glm::vec3(0, 0, -1);
	glm::vec3 right = registry.getComponent<ECS::Transform>(cameraEntity).rotation * glm::vec3(1, 0, 0);

	ImGui::Text("Dir = (%.2f, %.2f, %.2f)", forward.x, forward.y, forward.z);

	ImGui::End();

	debugSystem.update(registry);

	Window::End();
}

void RealTimeRendererWindow::InitObjects()
{
	initGLObjects();
	//camera.SetImageSize(width, height);
}

void RealTimeRendererWindow::initGLObjects()
{
	GPUScene::preloadGPU(registry, cache);
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
	Window::onCursorPos(currX, currY);

	inputManager.onMouseMove(currX, currY);
}

void RealTimeRendererWindow::onKey(int key, int scancode, int action, int mods)
{
	Window::onKey(key, scancode, action, mods);

	inputManager.onKeyEvent(inputMap.translateKey(key), inputMap.translateButtonAction(action));
}

void RealTimeRendererWindow::onMouseButton(int button, int action, int mods)
{
	Window::onMouseButton(button, action, mods);

	inputManager.onMouseButtonEvent(inputMap.translateMouseButton(button), inputMap.translateButtonAction(action));
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

std::shared_ptr<Shader> GPUScene::getOrCreateShader(GPUCache& c, GPUMaterial::Type type, const char* vertexPath, const char* fragmentPath)
{
	auto iter = c.shaderCache.find(type);
	if (iter != c.shaderCache.end())
	{
		return c.shaderCache[type];
	}

	auto shader = std::make_shared<Shader>(vertexPath, fragmentPath);

	switch (type)
	{
	case GPUMaterial::Type::Color:
	{
		shader.get()->CacheUniform("view");
		shader.get()->CacheUniform("projection");
		shader.get()->CacheUniform("model");
		shader.get()->CacheUniform("color");
	} break;
	case GPUMaterial::Type::Phong:
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
	switch (md.get()->type)
	{
	case ECS::MaterialData::Type::Color:
	{
		material.get()->type = GPUMaterial::Type::Color;
		material.get()->shader = getOrCreateShader(c, GPUMaterial::Type::Color, "color.vert", "color.frag");
		auto& d = std::get<ECS::ColorData>(md.get()->data);
		material.get()->color = d.color;
	} break;
	case ECS::MaterialData::Type::Phong:
	{
		material.get()->type = GPUMaterial::Type::Phong;
		material.get()->shader = getOrCreateShader(c, GPUMaterial::Type::Phong, "lighting.vert", "lighting.frag");
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

void GPUScene::preloadGPU(ECS::ECSRegistry& registry, GPUCache& cache)
{
	for (auto& [entity, meshComp] : registry.view<ECS::Mesh>())
	{
		auto& matComp = registry.getComponent<ECS::Material>(entity);

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
