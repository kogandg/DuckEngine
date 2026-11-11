#pragma once

#include <Duck/RayTraceCpu/Camera.h>
#include <Duck/RayTraceCpu/Scene.h>

#include <vector>

struct TestScene
{
    const char* Name = "";
    Duck::RayTraceCpu::Scene Scene;
    Duck::RayTraceCpu::Camera Camera;
};

std::vector<TestScene> MakeTestScenes(float aspectRatio);