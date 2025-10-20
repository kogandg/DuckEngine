#pragma once

#include "GPUResourceManager.h"
#include "AssetManager.h"
#include "ECSRegistry.h"
#include "UBOData.h"

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


};