#pragma once
#include "Light.h"

class PointLight : public Light
{
public:
	PointLight(std::string name, glm::vec3 position, glm::vec3 color, glm::vec3 attenuation = glm::vec3(1, 0, 0), glm::mat4 transform = glm::mat4(1.0f));

	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 attenuation;
private:

};

