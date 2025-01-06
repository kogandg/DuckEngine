#pragma once

#include <glm/glm.hpp>

#include "Geometry.h"
#include "EmbreeGeometry.h"

class Triangle : public Geometry
{
public:
	Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3);

	void InitGeometry(RTCDevice device, RTCScene scene) override;

private:
	glm::vec3 v1;
	glm::vec3 v2;
	glm::vec3 v3;
};

