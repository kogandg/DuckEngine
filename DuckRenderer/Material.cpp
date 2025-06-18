#include "Material.h"


Material::Material(glm::vec3 diffuse, glm::vec3 specular, float shininess, glm::vec3 emission)
{
    this->diffuse = diffuse;
    this->specular = specular;
    this->shininess = shininess;
    this->emission = emission;
}


MaterialEvaluation Material::Evaluate(Intersection intersection)
{
    MaterialEvaluation me;
    me.direction = glm::normalize(intersection.normal);
    me.color = diffuse;
    return me;
}

AmbientMaterial::AmbientMaterial(glm::vec3 diffuse, glm::vec3 specular, float shininess, glm::vec3 emission, glm::vec3 ambient) : Material(diffuse, specular, shininess, emission)
{
    this->ambient = ambient;
}

MaterialEvaluation AmbientMaterial::Evaluate(Intersection intersection)
{
    MaterialEvaluation me;
    me.direction = glm::normalize(intersection.normal);
    me.color = diffuse;
    return me;
}
