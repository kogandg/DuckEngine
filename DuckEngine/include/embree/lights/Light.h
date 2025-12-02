#pragma once
#include <embree/scene/SceneObject.h>
class Light : public SceneObject
{
public:
	glm::vec3 color;

protected:
	Light(std::string name = "light", glm::mat4 transform = glm::mat4(1.0f)) : SceneObject(name, transform) { }
};

