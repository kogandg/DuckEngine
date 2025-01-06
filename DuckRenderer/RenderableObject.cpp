#include "RenderableObject.h"

RenderableObject::RenderableObject(std::string name, std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material) : SceneObject(name)
{
	this->geometry = geometry;
	this->material = material;
}
