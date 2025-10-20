#pragma once
#include <glm/glm.hpp>

namespace UBOData
{
	constexpr int MAX_LIGHTS = 32;

	struct Camera
	{
		glm::mat4 view;
		glm::mat4 projection;
	};

	struct Light
	{
		glm::vec4 position;//w = 1 for point/spot, w = 0 for directional
		glm::vec4 direction;//w = spotCutoff for spotlights

		glm::vec3 ambient; float _padAmbient;
		glm::vec3 diffuse; float _padDiffuse;
		glm::vec3 specular; float _padSpecular;

		float constant;
		float linear;
		float quadratic;
		float outerCutOff;

		int type; // 0 = directional, 1 = point, 2 = spot
		int _padType0;
		int _padType1;
		int _padType2;
	};

	struct Lights
	{
		int numLights; //x = number of lights
		int _pad0;
		int _pad1;
		int _pad2;

		Light lights[MAX_LIGHTS]; //max 16 lights
	};

	static_assert(sizeof(Light) == 112, "UBO Light must be 112 bytes (std140 layout)");
	static_assert(sizeof(Lights) == (16 + MAX_LIGHTS * sizeof(Light)), "Lights UBO size mismatch");

};