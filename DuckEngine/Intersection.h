#pragma once

#include <glm/glm.hpp>


struct Intersection
{
	glm::vec3 origin = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(0.0f);
	glm::vec3 hit = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f);
	float t = std::numeric_limits<float>::infinity();
	unsigned int id = -1;
};