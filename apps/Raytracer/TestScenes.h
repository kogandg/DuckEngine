#pragma once

#include <Duck/PathTraceCpu/Camera.h>
#include <Duck/PathTraceCpu/Scene.h>

#include <vector>

struct TestScene
{
    const char* Name = "";
    Duck::PathTrace::Scene Scene;
    Duck::PathTrace::Camera Camera;
};

std::vector<TestScene> MakeTestScenes(float aspectRatio);