#pragma once
#include <string>
#include <glm/glm.hpp>

class SceneObject
{
public:
	SceneObject(std::string name = "", glm::mat4 transform = glm::mat4(1.0f));

	inline glm::mat4 GetTransform() { return transform; }

	virtual ~SceneObject() = default;

protected:
	std::string name;
	glm::mat4 transform;
	bool isVisible;
};

