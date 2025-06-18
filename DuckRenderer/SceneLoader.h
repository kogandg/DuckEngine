#pragma once

#include "Scene.h"
#include "Tokenizer.h"
#include "RenderTarget.h"
#include "Integrator.h"
#include "Triangle.h"
#include "Sphere.h"
#include "RayTracerIntegrator.h"

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

