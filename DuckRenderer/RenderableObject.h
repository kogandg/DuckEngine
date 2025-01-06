#pragma once

#include <memory>
#include <glm/glm.hpp>

#include "SceneObject.h"
#include "Geometry.h"

struct Material
{
	glm::vec3 diffuse = glm::vec3(0.0f);
	glm::vec3 specular = glm::vec3(0.0f);
	float shininess = 1.0f;
	float roughness = 1.0f;
	glm::vec3 emission = glm::vec3(0.0f);
};

class RenderableObject : public SceneObject
{
public:
	RenderableObject(std::string name, std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material);

	std::shared_ptr<Geometry> geometry;
	std::shared_ptr<Material> material;

private:
	
};

