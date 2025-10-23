#include "RealTimeRendererWindow.h"


RealTimeRendererWindow::RealTimeRendererWindow(const char* title, int width, int height) : Window(title, width, height), renderer(assetManager, registry)//, camera(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), 0.0f)
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

	glDisable(GL_CULL_FACE);

	if (glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	//auto cubeMesh = std::make_shared<ECS::MeshData>(ECS::MeshData({
	//	// positions          // normals           // texture coords
	//	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
	//	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
	//	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	//	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	//	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
	//	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

	//	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
	//	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
	//	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	//	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	//	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
	//	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

	//	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	//	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	//	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	//	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	//	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	//	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	//	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	//	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	//	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	//	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	//	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	//	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	//	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
	//	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
	//	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	//	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	//	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
	//	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

	//	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
	//	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
	//	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	//	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	//	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
	//	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
	//	}));

	/*auto lightCubeMat = ECS::MaterialData(ECS::MaterialData::Type::Color, ECS::ColorData(glm::vec3(1.0f)));
	auto lightCubeMaterial = std::make_shared<ECS::MaterialData>(lightCubeMat);
	auto cubeMat = ECS::MaterialData(ECS::MaterialData::Type::Phong, ECS::PhongData("container2.png", "container2_specular.png", 32.0f));
	auto cubeMaterial = std::make_shared<ECS::MaterialData>(cubeMat);

	ECS::Mesh mesh;
	mesh.data = cubeMesh;

	ECS::Material cubeMatComp;
	cubeMatComp.data = cubeMaterial;
	for (int i = 0; i < 10; i++)
	{
		float angle = 20.0f * i;

		ECS::Transform transform;
		transform.position = cubePositions[i];
		transform.rotation = glm::angleAxis(glm::radians(angle), glm::normalize(glm::vec3(1, 0.3, 0.5f)));

		auto cube = registry.CreateEntity();

		registry.addComponent<ECS::Mesh>(cube, mesh);
		registry.addComponent<ECS::Transform>(cube, transform);
		registry.addComponent<ECS::Material>(cube, cubeMatComp);
	}

	ECS::Material lightCubeMatComp;
	lightCubeMatComp.data = lightCubeMaterial;
	for (int i = 0; i < pointLightPositions.size(); i++)
	{
		ECS::Transform transform;
		transform.position = pointLightPositions[i];
		transform.scale = glm::vec3(0.2f);

		ECS::Light light;
		light.type = ECS::Light::Type::Point;
		light.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
		light.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
		light.specular = glm::vec3(1.0f, 1.0f, 1.0f);
		ECS::PointData data;
		data.constant = 1.0f;
		data.linear = 0.09f;
		data.quadratic = 0.032f;
		light.data = data;

		auto lightCube = registry.CreateEntity();

		registry.addComponent<ECS::Mesh>(lightCube, mesh);
		registry.addComponent<ECS::Transform>(lightCube, transform);
		registry.addComponent<ECS::Material>(lightCube, lightCubeMatComp);
		registry.addComponent<ECS::Light>(lightCube, light);
	}*/

	auto cubeModel = assetManager.LoadModel("resources/backpack/backpack.obj");
	for (MeshID meshID : cubeModel)
	{
		auto entity = registry.CreateEntity();
		ECS::Mesh meshComp;
		meshComp.meshID = meshID;
		registry.addComponent<ECS::Mesh>(entity, meshComp);
		MaterialID matID = assetManager.GetMeshMaterial(meshID);
		auto matData = assetManager.GetMaterial(matID);

		//matData->type = "unlit";
		ECS::Material matComp;
		matComp.materialID = matID;
		registry.addComponent<ECS::Material>(entity, matComp);
	}

	ECS::Transform cameraTransform;
	cameraTransform.position = glm::vec3(0.0f, 0.0f, 3.0f);
	ECS::Camera cameraData;
	cameraData.aspectRatio = width / height;
	cameraData.fovY = 45.0f;
	cameraEntity = registry.CreateEntity();
	registry.addComponent<ECS::Camera>(cameraEntity, cameraData);
	registry.addComponent<ECS::Transform>(cameraEntity, cameraTransform);
	registry.addComponent<ECS::Hierarchy>(cameraEntity, ECS::Hierarchy());
	cameraController.pitch = 0.0f;
	cameraController.yaw = -90.0f;
	cameraController.init(cameraTransform);

	//spotlight
	/*auto sle = registry.CreateEntity();
	{
		ECS::Light s;
		s.type = ECS::Light::Type::Spot;
		s.ambient = glm::vec3(0.0f);
		s.diffuse = glm::vec3(1.0f);
		s.specular = glm::vec3(1.0f);
		ECS::SpotData sd;
		sd.constant = 1.0f;
		sd.linear = 0.09f;
		sd.quadratic = 0.032f;
		sd.cutOff = glm::cos(glm::radians(12.5f));
		sd.outerCutOff = glm::cos(glm::radians(15.0f));
		s.data = sd;

		registry.addComponent<ECS::Light>(sle, s);

		ECS::Hierarchy sh;
		sh.parent = cameraEntity;
		registry.addComponent<ECS::Hierarchy>(sle, sh);

		auto& ch = registry.getComponent<ECS::Hierarchy>(cameraEntity);
		ch.children.push_back(sle);
	}*/
	
	//directional light
	auto dle = registry.CreateEntity();
	{
		ECS::Transform directionalTransform;
		directionalTransform.rotation = glm::quat(glm::vec3(-0.2f, -1.0f, -0.3f));
		ECS::Light directionalLightComp;
		directionalLightComp.type = ECS::Light::Type::Directional;
		directionalLightComp.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
		directionalLightComp.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
		directionalLightComp.specular = glm::vec3(0.5f, 0.5f, 0.5f);

		registry.addComponent<ECS::Light>(dle, directionalLightComp);
		registry.addComponent<ECS::Transform>(dle, directionalTransform);
	}

	//point lights
	for (int i = 0; i < pointLightPositions.size(); i++)
	{
		ECS::Transform transform;
		transform.position = pointLightPositions[i];

		ECS::Light light;
		light.type = ECS::Light::Type::Point;
		light.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
		light.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
		light.specular = glm::vec3(1.0f, 1.0f, 1.0f);
		ECS::PointData data;
		data.constant = 1.0f;
		data.linear = 0.09f;
		data.quadratic = 0.032f;
		light.data = data;

		auto lightEnt = registry.CreateEntity();
		registry.addComponent<ECS::Transform>(lightEnt, transform);
		registry.addComponent<ECS::Light>(lightEnt, light);
	}
}

void RealTimeRendererWindow::CleanUp()
{
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

	//cache.SyncMaterials(registry);
}

void RealTimeRendererWindow::DisplayCallback()
{
	if (!Window::Begin()) return;

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	auto& cameraData = registry.getComponent<ECS::Camera>(cameraEntity);
	cameraData.aspectRatio = (float)width / (float)height;

	auto& camTransform = registry.getComponent<ECS::Transform>(cameraEntity);

	renderer.SetCamera(cameraData.view, cameraData.projection);
	renderer.UpdateLightsUBO();

	renderer.RenderScene(camTransform.position);

	/*
	//ubos
	CameraUBO camUBO;
	camUBO.view = cameraData.view;
	camUBO.projection = cameraData.projection;
	cache.UpdateUBO("Camera", &camUBO, sizeof(CameraUBO));

	LightsUBO lightsUBO;
	auto lights = registry.view<ECS::Light>();
	lightsUBO.numLights = glm::vec4(lights.size());
	int lightsCount = 0;
	for (auto& [entity, light] : lights)
	{
		GPULight gpuLight;
		auto& transform = registry.getComponent<ECS::Transform>(entity);

		switch (light.type)
		{
		case ECS::Light::Type::Directional:
		{
			gpuLight.type = glm::vec4(0);
			gpuLight.position = glm::vec4(0.0f);
			gpuLight.direction = glm::vec4(transform.worldDirection, 0.0f);

			gpuLight.params = glm::vec4(0.0f);
		}break;
		case ECS::Light::Type::Point:
		{
			const auto& p = std::get<ECS::PointData>(light.data);
			gpuLight.type = glm::vec4(1);
			gpuLight.position = glm::vec4(transform.worldPosition, 1.0f);
			gpuLight.direction = glm::vec4(0.0f);

			gpuLight.params = glm::vec4(p.constant, p.linear, p.quadratic, 0.0f);
		}break;
		case ECS::Light::Type::Spot:
		{
			const auto& s = std::get<ECS::SpotData>(light.data);
			gpuLight.type = glm::vec4(2);
			gpuLight.position = glm::vec4(transform.worldPosition, 1.0f);
			gpuLight.direction = glm::vec4(transform.worldDirection, s.cutOff);

			gpuLight.params = glm::vec4(s.constant, s.linear, s.quadratic, s.outerCutOff);
		}break;
		default:
			break;
		}

		gpuLight.ambient = glm::vec4(light.ambient, 0.0f);
		gpuLight.diffuse = glm::vec4(light.diffuse, 0.0f);
		gpuLight.specular = glm::vec4(light.specular, 0.0f);

		lightsUBO.lights[lightsCount] = gpuLight;
		lightsCount++;
	}
	cache.UpdateUBO("Lights", &lightsUBO, sizeof(LightsUBO));


	for (auto& [entity, meshComp] : registry.view<ECS::Mesh>())
	{
		auto& matComp = registry.getComponent<ECS::Material>(entity);
		auto& transform = registry.getComponent<ECS::Transform>(entity);

		auto geom = cache.GetGeometry(meshComp.data);
		auto gpuMat = cache.GetMaterial(matComp.data).get();
		auto shaderSig = cache.GetRequiredSignature(*gpuMat);
		auto shader = gpuMat->shader.get();

		GLuint vao = *cache.GetVAO(*geom.get(), shaderSig);

		shader->Use();
		glBindVertexArray(vao);

		//uniforms
		switch (gpuMat->type)
		{
		case GPUMaterial::Type::Color:
		{
			shader->SetMat4("model", transform.world);
			shader->SetVec3("color", gpuMat->vectors["color"]);
		}break;
		case GPUMaterial::Type::Phong:
		{
			shader->SetMat4("model", transform.world);
			shader->SetMat3("transposeInverseModel", glm::transpose(glm::inverse(transform.world)));

			shader->SetFloat("material.shininess", gpuMat->scalars["shininess"]);
			shader->SetTexture("material.diffuse", 0, *gpuMat->textures["diffuse"]);
			shader->SetTexture("material.specular", 1, *gpuMat->textures["specular"]);
		}break;
		}

		glDrawElements(GL_TRIANGLES, geom->indexCount, GL_UNSIGNED_INT, 0);
		//glDrawArrays(GL_TRIANGLES, 0, geom.get()->vertexCount);

		glBindVertexArray(0);
		glActiveTexture(GL_TEXTURE0);
	}

	*/


	ImGui::SetNextWindowPos(ImVec2(width - 225, 0), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(225, 250), ImGuiCond_Once);
	ImGui::Begin("Mouse info", nullptr);

	auto mousePos = inputManager.GetMousePos();
	auto mouseDelta = inputManager.GetMouseDelta();
	ImGui::Text("Mouse Pos = (%.2f, %.2f)", mousePos.x, mousePos.y);
	ImGui::Text("Mouse Delta = (%.2f, %.2f)", mouseDelta.x, mouseDelta.y);
	ImGui::Text("Cursor mode = %s", inputManager.GetCursorMode() == CursorMode::Normal ? "normal" : "locked");

	glm::vec3 forward = registry.getComponent<ECS::Transform>(cameraEntity).rotation * glm::vec3(0, 0, -1);
	glm::vec3 right = registry.getComponent<ECS::Transform>(cameraEntity).rotation * glm::vec3(1, 0, 0);

	ImGui::Text("Dir = (%.2f, %.2f, %.2f)", forward.x, forward.y, forward.z);

	ImGui::End();

	//drawDebugUI(registry, cache);

	Window::End();
}

void RealTimeRendererWindow::InitObjects()
{
	initGLObjects();
	//camera.SetImageSize(width, height);
}

void RealTimeRendererWindow::initGLObjects()
{
	renderer.Initialize();
	//cache.PreloadGPU(registry);
}


//void RealTimeRendererWindow::drawDebugUI(ECS::ECSRegistry& registry, GPUCache& cache)
//{
//	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
//	ImGui::Begin("Scene");
//
//	for (auto& e : registry.getEntities())
//	{
//		if (registry.hasComponent<ECS::Hierarchy>(e))
//		{
//			auto& h = registry.getComponent<ECS::Hierarchy>(e);
//			if (h.parent != ECS::INVALID_ENTITY) continue;
//		}
//		drawEntityNode(registry, e, cache);
//	}
//
//	ImGui::End();
//}
//
//void RealTimeRendererWindow::drawEntityNode(ECS::ECSRegistry& registry, ECS::Entity e, GPUCache& cache)
//{
//	ImGui::PushID(e);
//
//	bool open = ImGui::TreeNodeEx(("Entity " + std::to_string(e)).c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth);
//
//	if (open)
//	{
//		if (registry.hasComponent<ECS::Transform>(e))
//		{
//			auto& t = registry.getComponent<ECS::Transform>(e);
//			if (ImGui::TreeNode("Transform"))
//			{
//				drawTransform(t);
//				ImGui::TreePop();
//			}
//		}
//
//		if (registry.hasComponent<ECS::Material>(e))
//		{
//			auto& m = registry.getComponent<ECS::Material>(e);
//			if (ImGui::TreeNode("Material"))
//			{
//				drawMaterial(m, registry, cache);
//				ImGui::TreePop();
//			}
//		}
//
//		if (registry.hasComponent<ECS::Light>(e))
//		{
//			auto& l = registry.getComponent<ECS::Light>(e);
//			if (ImGui::TreeNode("Light"))
//			{
//				drawLight(l);
//				ImGui::TreePop();
//			}
//		}
//
//		if (registry.hasComponent<ECS::Hierarchy>(e))
//		{
//			auto& h = registry.getComponent<ECS::Hierarchy>(e);
//			for (auto child : h.children)
//			{
//				drawEntityNode(registry, child, cache);
//			}
//		}
//
//		ImGui::TreePop();
//	}
//
//	ImGui::PopID();
//}
//
//void RealTimeRendererWindow::drawTransform(ECS::Transform& t)
//{
//	bool dirty = false;
//	if (ImGui::DragFloat3("Position", &t.position[0], 0.1f)) t.dirty = true;
//
//	glm::vec3 euler = glm::degrees(glm::eulerAngles(t.rotation));
//	if (ImGui::DragFloat3("Rotation", &euler[0], 1.0f))
//	{
//		t.rotation = glm::quat(glm::radians(euler));
//		t.dirty = true;
//	}
//
//	if (ImGui::DragFloat3("Scale", &t.scale[0], 0.1f)) t.dirty = true;
//}
//
//void RealTimeRendererWindow::drawMaterial(ECS::Material& m, ECS::ECSRegistry& registry, GPUCache& cache)
//{
//	auto mat = m.data.get();
//	if (ImGui::Button("Make Unique"))
//	{
//		//make unique
//		m.makeMaterialUnique();
//		mat = m.data.get();
//	}
//
//	switch (mat->type)
//	{
//	case ECS::MaterialData::Type::Color:
//	{
//		/*auto& c = std::get<ECS::ColorData>(mat->data);
//		if (ImGui::ColorEdit3("Color", &c.color[0]))
//		{
//			mat->dirty = true;
//		}*/
//	}
//	break;
//	case ECS::MaterialData::Type::Phong:
//	{
//		auto d = m.data.get();
//
//		for (auto& [name, texture] : d->textures)
//		{
//			auto textureId = cache.GetTexture(texture.path);
//			ImGui::Image((ImTextureID)*textureId, ImVec2(256, 256));
//		}
//
//		//auto diffuseID = *cache.GetTexture(p.diffuseTexturePath).get();
//		//auto specularID = *cache.GetTexture(p.specularTexturePath).get();
//
//		//ImGui::Text("Diffuse texture: %s", p.diffuseTexturePath.c_str());
//		//ImGui::Image((ImTextureID)diffuseID, ImVec2(256, 256));//256 is the draw image size, not actaully the resolution of the texture
//		//ImGui::Text("Specular texture: %s", p.specularTexturePath.c_str());
//		//ImGui::Image((ImTextureID)specularID, ImVec2(256, 256));
//		//if (ImGui::DragFloat("Shininess", &p.shininess, 0.1f, 0.0f, 128.0f))
//		//{
//		//	mat->dirty = true;
//		//}
//	}
//	break;
//	}
//}
//
//void RealTimeRendererWindow::drawLight(ECS::Light& l)
//{
//
//}

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