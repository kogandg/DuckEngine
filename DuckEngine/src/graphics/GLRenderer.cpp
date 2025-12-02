#include <graphics/GLRenderer.h>

GLRenderer::GLRenderer(AssetManager& assetManager, ECS::ECSRegistry& ecs) : assetManager(assetManager), ecs(ecs)
{
}

void GLRenderer::Initialize()
{
	gpuResourceManager.LoadDefaultShaders();

	gpuResourceManager.LoadDefaultResources(assetManager);
	gpuResourceManager.LoadFromAssetManager(assetManager);
	
	gpuResourceManager.CreateUBO("CameraData", sizeof(UBOData::Camera), 0);//binding point 0
	gpuResourceManager.CreateUBO("LightsData", sizeof(UBOData::Lights), 1);//binding point 1
}

void GLRenderer::RenderScene(const glm::vec3& camPos)
{
	//maybe clear here, but also clearing in the window

	std::vector<RenderCommand> opaqueCommands;
	std::vector<RenderCommand> transparentCommands;

	for (auto entity : ecs.getEntities())
	{
		if (ecs.hasComponent<ECS::Mesh>(entity) && ecs.hasComponent<ECS::Material>(entity) && ecs.hasComponent<ECS::Transform>(entity))
		{
			RenderCommand cmd;
			cmd.mesh = gpuResourceManager.GetMesh(ecs.getComponent<ECS::Mesh>(entity).meshID);
			cmd.material = gpuResourceManager.GetMaterial(ecs.getComponent<ECS::Material>(entity).materialID);
			cmd.modelMatrix = ecs.getComponent<ECS::Transform>(entity).world;
			cmd.worldPosition = ecs.getComponent<ECS::Transform>(entity).worldPosition;

			if (cmd.material->IsTransparent())
			{
				transparentCommands.push_back(cmd);
			}
			else
			{
				opaqueCommands.push_back(cmd);
			}
		}
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	for (auto& cmd : opaqueCommands)
	{
		drawCommand(cmd, camPos);
	}

	for (auto& cmd : transparentCommands)
	{
		float d2 = glm::length2(camPos - cmd.worldPosition);
		cmd.sortKey = d2;
	}
	std::sort(transparentCommands.begin(), transparentCommands.end(),
		[](const RenderCommand& a, const RenderCommand& b)
		{
			return a.sortKey > b.sortKey;
		});

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	for (auto& cmd : transparentCommands)
	{
		drawCommand(cmd, camPos);
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void GLRenderer::SetCamera(const glm::mat4& view, const glm::mat4& projection)
{
	UBOData::Camera camData = { view, projection };
	gpuResourceManager.UpdateUBO("CameraData", &camData, sizeof(UBOData::Camera));
}

void GLRenderer::UpdateLightsUBO()
{
	UBOData::Lights lights{};
	int count = 0;

	for (const auto& [entity, lightComp] : ecs.view<ECS::Light>())
	{
		auto& transformComp = ecs.getComponent<ECS::Transform>(entity);

		if (count >= UBOData::MAX_LIGHTS) break;
		UBOData::Light& gpuLight = lights.lights[count];

		switch (lightComp.type)
		{
		case ECS::Light::Type::Directional:
		{
			gpuLight.type = 0;
			glm::vec3 dir = glm::normalize(transformComp.worldDirection);
			gpuLight.position = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
			gpuLight.direction = glm::vec4(dir, 0.0f);
		}
		break;
		case ECS::Light::Type::Point:
		{
			gpuLight.type = 1;
			glm::vec3 pos = transformComp.worldPosition;
			glm::vec3 dir = glm::normalize(transformComp.worldDirection);
			gpuLight.position = glm::vec4(pos, 1.0f);
			gpuLight.direction = glm::vec4(dir, 0.0f);

			auto pointData = std::get<ECS::PointData>(lightComp.data);
			gpuLight.constant = pointData.constant;
			gpuLight.linear = pointData.linear;
			gpuLight.quadratic = pointData.quadratic;
		}
		break;
		case ECS::Light::Type::Spot:
		{
			gpuLight.type = 2;
			glm::vec3 pos = transformComp.worldPosition;
			glm::vec3 dir = glm::normalize(transformComp.worldDirection);
			gpuLight.position = glm::vec4(pos, 1.0f);
			gpuLight.direction = glm::vec4(dir, 0.0f);

			auto spotData = std::get<ECS::SpotData>(lightComp.data);
			gpuLight.direction.w = spotData.cutOff;
			gpuLight.constant = spotData.constant;
			gpuLight.linear = spotData.linear;
			gpuLight.quadratic = spotData.quadratic;
			gpuLight.outerCutOff = spotData.outerCutOff;
		}
		break;
		}

		gpuLight.ambient = lightComp.ambient;
		gpuLight.diffuse = lightComp.diffuse;
		gpuLight.specular = lightComp.specular;

		count++;
	}

	lights.numLights = count;

	gpuResourceManager.UpdateUBO("LightsData", &lights, sizeof(UBOData::Lights));
}

void GLRenderer::drawCommand(const RenderCommand& cmd, const glm::vec3& camPos)
{
	auto shader = gpuResourceManager.GetShader(cmd.material->type);

	shader->Use();

	gpuResourceManager.BindUBOToShader("CameraData", shader);
	gpuResourceManager.BindUBOToShader("LightsData", shader);

	shader->SetMat4("model", cmd.modelMatrix);

	if (shader->ContainsUniform("viewPos"))
	{
		shader->SetVec3("viewPos", camPos);
	}

	//bind colors/shininess/textures
	bindMaterial(shader, *cmd.material);

	glBindVertexArray(cmd.mesh->vao);
	glDrawElements(GL_TRIANGLES, cmd.mesh->indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void GLRenderer::renderEntity(ECS::Entity entity, const glm::vec3& camPos)
{
	auto& meshComp = ecs.getComponent<ECS::Mesh>(entity);
	auto& matComp = ecs.getComponent<ECS::Material>(entity);
	auto& transformComp = ecs.getComponent<ECS::Transform>(entity);

	auto gpuMesh = gpuResourceManager.GetMesh(meshComp.meshID);
	auto gpuMat = gpuResourceManager.GetMaterial(assetManager.GetMeshMaterial(meshComp.meshID));
	gpuMat = gpuResourceManager.GetMaterial(matComp.materialID);
	auto shader = gpuResourceManager.GetShader(gpuMat->type);

	if (!gpuMesh)
	{
		std::cerr << "Warning: Mesh not found in GPUResourceManager for MeshID " << meshComp.meshID << std::endl;
		return;
	}
	if (!gpuMat)
	{
		std::cerr << "Warning: Material not found in GPUResourceManager for MaterialID " << assetManager.GetMeshMaterial(meshComp.meshID) << std::endl;
		return;
	}
	if (!shader)
	{
		std::cerr << "Warning: Shader not found in GPUResourceManager for type " << gpuMat->type << std::endl;
		return;
	}

	shader->Use();

	gpuResourceManager.BindUBOToShader("CameraData", shader);
	gpuResourceManager.BindUBOToShader("LightsData", shader);

	shader->SetMat4("model", transformComp.world);

	if (shader->ContainsUniform("viewPos"))
	{
		shader->SetVec3("viewPos", camPos);
	}

	//bind colors/shininess/textures
	bindMaterial(shader, *gpuMat);

	glBindVertexArray(gpuMesh->vao);
	glDrawElements(GL_TRIANGLES, gpuMesh->indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	//should unbind textures
}

void GLRenderer::bindMaterial(std::shared_ptr<Shader> shader, const GPUMaterial& material)
{
	for (const auto& [name, value] : material.scalars)
	{
		std::string uniformName = "material." + name;
		if (shader->ContainsUniform(uniformName))
		{
			shader->SetFloat(uniformName, value);
		}
	}
	for (const auto& [name, value] : material.vectors)
	{
		std::string uniformName = "material." + name;
		if (shader->ContainsUniform(uniformName))
		{
			shader->SetVec3(uniformName, value);
		}
	}

	int textureUnit = 0;
	std::unordered_set <std::string> boundTextures;
	for (const auto& [name, texture] : material.textures)
	{
		std::string uniformName = "material." + name;
		if (shader->ContainsUniform(uniformName))
		{
			shader->SetTexture(uniformName, textureUnit, texture->id);
			boundTextures.insert(uniformName);
			textureUnit++;
		}
	}

	auto samplers = shader->GetActiveSamplerUniforms();
	auto defaultTextureID = gpuResourceManager.GetDefaultTextureID();
	for (const auto& samplerName : samplers)
	{
		if (boundTextures.contains(samplerName)) continue;

		shader->SetTexture(samplerName, textureUnit, defaultTextureID);
		textureUnit++;
	}
}
