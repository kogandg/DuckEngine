#pragma once

#include "Geometry.h"

#include <embree4/rtcore.h>

class EmbreeGeometry : public Geometry
{
public: 
	virtual void InitGeometry(RTCDevice device, RTCScene scene, glm::mat4 transform);
};

