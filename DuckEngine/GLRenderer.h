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
    void RenderScene();
    //void Resize(int width, int height);
    void SetCamera(const glm::mat4& view, const glm::mat4& projection);

private:
    AssetManager& assetManager;
    ECS::ECSRegistry& ecs;
    GPUResourceManager gpuResourceManager;

    void RenderEntity(ECS::Entity entity);
};