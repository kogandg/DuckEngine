#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(std::string name, glm::vec3 direction, glm::vec3 color, glm::mat4 transform) : Light(name, transform)
{
	this->direction = direction;
	this->color = color;
}
