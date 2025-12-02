#pragma once

#include "Scene.h"
#include <core/Tokenizer.h>
#include <graphics/RenderTarget.h>
#include <integrators/Integrator.h>
#include <embree/Triangle.h>
#include <embree/Sphere.h>
#include <integrators/RayTracerIntegrator.h>
#include <embree/lights/PointLight.h>

#include <stack>

struct SceneFileParsingOutput
{
	std::shared_ptr<Scene> scene;
	std::shared_ptr<RenderTarget> renderTarget;
	std::shared_ptr<Integrator> integrator;
};

class SceneLoader
{
public:
	SceneFileParsingOutput ParseSceneFromTestFile(const char* fileName);
};

