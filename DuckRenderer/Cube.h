#pragma once

#include "EmbreeGeometry.h"
#include "Triangle.h"
#include <vector>

class Cube : public EmbreeGeometry
{
public:
	Cube();

	void InitGeometry(RTCDevice device, RTCScene scene, glm::mat4 transform) override;

private:
	glm::vec3 cubeMin = glm::vec3(-1, -1, -1);
	glm::vec3 cubeMax = glm::vec3(1, 1, 1);
};

