#include <embree/scene/SceneObject.h>

SceneObject::SceneObject(std::string name, glm::mat4 transform)
{
	this->name = name;
	this->transform = transform;
	isVisible = true;
}
