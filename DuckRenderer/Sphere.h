#pragma once

#include "EmbreeGeometry.h"

class Sphere : public EmbreeGeometry
{
public:

	Sphere(glm::vec3 center, float radius);

	void InitGeometry(RTCDevice device, RTCScene scene, glm::mat4 transform) override;

private:
	glm::vec3 center;
	float radius;

	static void boundsFunc(const struct RTCBoundsFunctionArguments* args);
	static void intersectFunc(const RTCIntersectFunctionNArguments* args);
	static void occludedFunc(const RTCOccludedFunctionNArguments* args);
};

