#include "GLRenderer.h"

GLRenderer::GLRenderer(AssetManager& assetManager, ECS::ECSRegistry& ecs) : assetManager(assetManager), ecs(ecs)
{
}

void GLRenderer::Initialize()
{
	gpuResourceManager.LoadDefaultShaders();

	gpuResourceManager.LoadFromAssetManager(assetManager);
	gpuResourceManager.CreateUBO("CameraData", sizeof(UBOData::Camera), 0);//binding point 0
}

void GLRenderer::RenderScene()
{
	//maybe clear here, but also clearing in the window

	for (auto entity : ecs.getEntities())
	{
		if (ecs.hasComponent<ECS::Mesh>(entity) && ecs.hasComponent<ECS::Material>(entity) && ecs.hasComponent<ECS::Transform>(entity))
		{
			RenderEntity(entity);
		}
	}
}

void GLRenderer::SetCamera(const glm::mat4& view, const glm::mat4& projection)
{
	UBOData::Camera camData = { view, projection };
	gpuResourceManager.UpdateUBO("CameraData", &camData, sizeof(UBOData::Camera));
}

void GLRenderer::RenderEntity(ECS::Entity entity)
{
	auto& meshComp = ecs.getComponent<ECS::Mesh>(entity);
	auto& matComp = ecs.getComponent<ECS::Material>(entity);
	auto& transformComp = ecs.getComponent<ECS::Transform>(entity);

	auto gpuMesh = gpuResourceManager.GetMesh(meshComp.meshID);
	auto gpuMat = gpuResourceManager.GetMaterial(assetManager.GetMeshMaterial(meshComp.meshID));
	auto shader = gpuResourceManager.GetShader(gpuMat->type);

	if (!gpuMesh || !gpuMat || !shader) return;

	shader->Use();

	gpuResourceManager.BindUBOToShader("CameraData", shader);

	shader->SetMat4("model", transformComp.world);

	//bind colors/shininess/textures


	glBindVertexArray(gpuMesh->vao);
	glDrawElements(GL_TRIANGLES, gpuMesh->indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	//should unbind textures
}
