#include <embree/Material.h>

MaterialEvaluation Material::Evaluate(Intersection intersection)
{
    MaterialEvaluation me;
    me.direction = glm::normalize(intersection.normal);
    me.color = glm::vec3(0.0f);
    return me;
}


RayTracingMaterial::RayTracingMaterial(glm::vec3 diffuse, glm::vec3 specular, float shininess, glm::vec3 emission, glm::vec3 ambient) : Material()
{
    this->diffuse = diffuse;
    this->specular = specular;
    this->shininess = shininess;
    this->emission = emission;
    this->ambient = ambient;
}

MaterialEvaluation RayTracingMaterial::Evaluate(Intersection intersection)
{
    MaterialEvaluation me;
    me.direction = intersection.normal;
    me.color = emission + ambient;
    return me;
}
