#pragma once
#include "Light.h"
class DirectionalLight : public Light
{
public:
	DirectionalLight(std::string name, glm::vec3 direction, glm::vec3 color, glm::mat4 transform = glm::mat4(1.0f));

	glm::vec3 direction;
};

