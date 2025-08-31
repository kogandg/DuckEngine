#pragma once

#include "EmbreeGeometry.h"

#include <glm/gtc/type_ptr.hpp>

class Triangle : public EmbreeGeometry
{
public:
	Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3);

	void InitGeometry(RTCDevice device, RTCScene scene, glm::mat4 transform) override;

	//need to add getters for verts or something
//private:
	glm::vec3 v1;
	glm::vec3 v2;
	glm::vec3 v3;
};

