#include <embree/scene/RenderableObject.h>

RenderableObject::RenderableObject(std::string name, std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material, glm::mat4 transform) : SceneObject(name, transform)
{
	this->geometry = geometry;
	this->material = material;

	inverseTransposeTransfrom = glm::inverse(glm::transpose(transform));
}
