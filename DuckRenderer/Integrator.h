#pragma once

#include "Scene.h"

enum BackgroundType
{
	None,
	Color,
	SimpleSky
};

class Integrator
{
public:
	virtual glm::vec3 RayColor(std::shared_ptr<Scene> scene, Intersection intersection, int depth) = 0;

	inline void SetBackgroundType(BackgroundType type, glm::vec3 color = glm::vec3(0.0f)) { backgroundType = type; backgroundColor = color; }

	inline void SetMaxDepth(int maxDepth) { this->maxDepth = maxDepth; }
	inline int GetMaxDepth() { return maxDepth; }

protected:
	BackgroundType backgroundType = None;
	glm::vec3 backgroundColor = glm::vec3(0.0f);

	virtual glm::vec3 background(Intersection intersection);

	int maxDepth;
};