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
	Material(glm::vec3 diffuse, glm::vec3 specular, float shininess, glm::vec3 emission);

	virtual MaterialEvaluation Evaluate(Intersection intersection);

	inline glm::vec3 GetDiffuse() { return diffuse; }
	inline glm::vec3 GetSpecular() { return specular; }
	inline float GetShininess() { return shininess; }
	inline glm::vec3 GetEmission() { return emission; }

	inline void SetDiffuse(glm::vec3 diffuse) { this->diffuse = diffuse; }
	inline void SetSpecular(glm::vec3 specular) { this->specular = specular; }
	inline void SetShininess(float shininess) { this->shininess = shininess; }
	inline void SetEmission(glm::vec3 emission) { this->emission = emission; }

protected:
	glm::vec3 diffuse = glm::vec3(0.0f);
	glm::vec3 specular = glm::vec3(0.0f);
	float shininess = 0.0f;
	glm::vec3 emission = glm::vec3(0.0f);
};

class AmbientMaterial : public Material
{
public:
	AmbientMaterial(glm::vec3 diffuse, glm::vec3 specular, float shininess, glm::vec3 emission, glm::vec3 ambient);

	MaterialEvaluation Evaluate(Intersection intersection);

	inline glm::vec3 GetAmbient() { return ambient; }

	inline void SetAmbient(glm::vec3 ambient) { this->ambient = ambient; }

private:
	glm::vec3 ambient;
};
