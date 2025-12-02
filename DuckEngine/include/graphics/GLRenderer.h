#pragma once

#include <graphics\buffers\GPUResourceManager.h>
#include <scene\AssetManager.h>
#include <scene\ECSRegistry.h>
#include <graphics\buffers\UBOData.h>

struct RenderCommand
{
	std::shared_ptr<GPUMesh> mesh;
	std::shared_ptr<GPUMaterial> material;
    glm::mat4 modelMatrix;
    glm::vec3 worldPosition;
    float sortKey;
};

class GLRenderer
{
public:
    GLRenderer(AssetManager& assetManager, ECS::ECSRegistry& ecs);
    //~GLRenderer();

    void Initialize();
    void RenderScene(const glm::vec3& camPos);
    //void Resize(int width, int height);
    void SetCamera(const glm::mat4& view, const glm::mat4& projection);

    void UpdateLightsUBO();

private:
    AssetManager& assetManager;
    ECS::ECSRegistry& ecs;
    GPUResourceManager gpuResourceManager;

    void renderEntity(ECS::Entity entity, const glm::vec3& camPos);
    void bindMaterial(std::shared_ptr<Shader> shader, const GPUMaterial& material);
	void drawCommand(const RenderCommand& cmd, const glm::vec3& camPos);
};