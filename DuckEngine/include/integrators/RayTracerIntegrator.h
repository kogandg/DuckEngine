#pragma once
#include "Integrator.h"
class RayTracerIntegrator : public Integrator
{
public:
	// Inherited via Integrator
	glm::vec3 RayColor(std::shared_ptr<Scene> scene, Intersection intersection, int depth) override;
};

