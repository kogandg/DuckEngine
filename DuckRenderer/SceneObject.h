#pragma once
#include <string>
#include <glm/glm.hpp>

class SceneObject
{
public:
	SceneObject(std::string name = "");

protected:
	std::string name;
	bool isVisible;

	glm::mat4 transform = glm::mat4(1.0f);
};

