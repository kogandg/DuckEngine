#include "RayTracerIntegrator.h"

glm::vec3 RayTracerIntegrator::RayColor(std::shared_ptr<Scene> scene, Intersection intersection, int depth)
{
	if (intersection.id == RTC_INVALID_GEOMETRY_ID)
	{
		return background(intersection);
	}

	auto sceneObj = scene.get();

	auto hitObject = sceneObj->GetRenderable(intersection.id).get();
	auto material = hitObject->material.get();
	auto materialEval = material->Evaluate(intersection);

	glm::vec3 finalIntensity = glm::vec3(0.0f);

	if (std::shared_ptr<AmbientMaterial> ambientMat = std::dynamic_pointer_cast<AmbientMaterial>(sceneObj->GetRenderable(intersection.id).get()->material))
	{
		finalIntensity += ambientMat.get()->GetAmbient();
	}

	finalIntensity += material->GetEmission();

	for (auto pointLight : sceneObj->GetPointLights())
	{
		auto pointLightDifference = pointLight.get()->position - intersection.hit;
		auto dirToPointLight = glm::normalize(pointLightDifference);
		auto distToPointLight = glm::length(pointLightDifference);

		auto occI = sceneObj->CastRay(intersection.hit, dirToPointLight);

		if (occI.id == RTC_INVALID_GEOMETRY_ID || distToPointLight < occI.t)
		{
			//return glm::vec3(1, 1, 1);
			auto direction = glm::normalize(intersection.direction);
			auto normal = materialEval.direction;

			auto halfAngle = glm::normalize(-direction + dirToPointLight);

			auto specularComponent = material->GetSpecular() * glm::pow(glm::max(glm::dot(normal, halfAngle), 0.0f), material->GetShininess());
			auto diffuseComponent = material->GetDiffuse() * glm::max(glm::dot(normal, dirToPointLight), 0.0f);
			
			auto attenuation = pointLight.get()->attenuation;
			auto attenuationTerm = 1.0f / (attenuation.x + (attenuation.y * distToPointLight) + (attenuation.z * distToPointLight * distToPointLight));

			finalIntensity += pointLight.get()->color * attenuationTerm * (diffuseComponent + specularComponent);
		}
	}

	//return glm::vec3(.5, .5, .5);
	return finalIntensity;
}
