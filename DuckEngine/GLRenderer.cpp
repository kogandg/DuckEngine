#include "GLRenderer.h"

GLRenderer::GLRenderer(AssetManager& assetManager, ECS::ECSRegistry& ecs) : assetManager(assetManager), ecs(ecs)
{
}

void GLRenderer::Initialize()
{
	gpuResourceManager.LoadDefaultShaders();

	gpuResourceManager.LoadFromAssetManager(assetManager);
	gpuResourceManager.CreateUBO("CameraData", sizeof(UBOData::Camera), 0);//binding point 0
	gpuResourceManager.CreateUBO("LightsData", sizeof(UBOData::Lights), 1);//binding point 1
}

void GLRenderer::RenderScene(const glm::vec3& camPos)
{
	//maybe clear here, but also clearing in the window

	for (auto entity : ecs.getEntities())
	{
		if (ecs.hasComponent<ECS::Mesh>(entity) && ecs.hasComponent<ECS::Material>(entity) && ecs.hasComponent<ECS::Transform>(entity))
		{
			renderEntity(entity, camPos);
		}
	}
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

void GLRenderer::renderEntity(ECS::Entity entity, const glm::vec3& camPos)
{
	auto& meshComp = ecs.getComponent<ECS::Mesh>(entity);
	auto& matComp = ecs.getComponent<ECS::Material>(entity);
	auto& transformComp = ecs.getComponent<ECS::Transform>(entity);

	auto gpuMesh = gpuResourceManager.GetMesh(meshComp.meshID);
	auto gpuMat = gpuResourceManager.GetMaterial(assetManager.GetMeshMaterial(meshComp.meshID));
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

	shader->SetVec3("viewPos", camPos);

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
