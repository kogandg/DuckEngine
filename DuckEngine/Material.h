#pragma once

#include <glm/glm.hpp>
#include "Intersection.h"

struct MaterialEvaluation
{
	glm::vec3 direction;
	glm::vec3 color;
};

class Material
{
public:
	virtual ~Material() = default;

	virtual MaterialEvaluation Evaluate(Intersection intersection);
};

class RayTracingMaterial : public Material
{
public:
	RayTracingMaterial(glm::vec3 diffuse = glm::vec3(0.0f), glm::vec3 specular = glm::vec3(0.0f), float shininess = 0, glm::vec3 emission = glm::vec3(0.0f), glm::vec3 ambient = glm::vec3(0.2f));
	
	MaterialEvaluation Evaluate(Intersection intersection);

	inline glm::vec3 GetDiffuse() { return diffuse; }
	inline glm::vec3 GetSpecular() { return specular; }
	inline float GetShininess() { return shininess; }
	inline glm::vec3 GetEmission() { return emission; }
	inline glm::vec3 GetAmbient() { return ambient; }

	inline void SetDiffuse(glm::vec3 diffuse) { this->diffuse = diffuse; }
	inline void SetSpecular(glm::vec3 specular) { this->specular = specular; }
	inline void SetShininess(float shininess) { this->shininess = shininess; }
	inline void SetEmission(glm::vec3 emission) { this->emission = emission; }
	inline void SetAmbient(glm::vec3 ambient) { this->ambient = ambient; }

protected:
	glm::vec3 diffuse = glm::vec3(0.0f);
	glm::vec3 specular = glm::vec3(0.0f);
	float shininess = 0.0f;
	glm::vec3 emission = glm::vec3(0.0f);
	glm::vec3 ambient;
};