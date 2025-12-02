#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Geometry
{
public:
	inline unsigned int GetGeometryID() { return geometryID; }

protected:
	unsigned int geometryID = -1;
};

