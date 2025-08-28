#include "PointLight.h"

PointLight::PointLight(std::string name, glm::vec3 position, glm::vec3 color, glm::vec3 attenuation, glm::mat4 transform) : Light(name, transform)
{
	this->position = position;
	this->color = color;
	this->attenuation = attenuation;
}
