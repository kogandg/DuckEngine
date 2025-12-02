#include <integrators/RayTracerIntegrator.h>

glm::vec3 RayTracerIntegrator::RayColor(std::shared_ptr<Scene> scene, Intersection intersection, int depth)
{
	if (depth == maxDepth)
	{
		return glm::vec3(0.0f);
	}
	if (intersection.id == RTC_INVALID_GEOMETRY_ID)
	{
		return background(intersection);
	}
	
	auto sceneObj = scene.get();

	auto hitObject = sceneObj->GetRenderable(intersection.id).get();
	auto material = dynamic_cast<RayTracingMaterial*>(hitObject->material.get());
	auto materialEval = material->Evaluate(intersection);

	glm::vec3 finalIntensity = glm::vec3(0.0f);

	finalIntensity += materialEval.color;

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

			finalIntensity += attenuationTerm * (pointLight.get()->color * (diffuseComponent + specularComponent));
		}
	}

	for (auto directionalLight : sceneObj->GetDirectionalLights())
	{
		auto lightDirection = directionalLight.get()->direction;
		auto occI = sceneObj->CastRay(intersection.hit, lightDirection);
		if (occI.id == RTC_INVALID_GEOMETRY_ID)
		{
			auto direction = glm::normalize(intersection.direction);
			auto normal = materialEval.direction;

			auto halfAngle = glm::normalize(-direction - lightDirection);

			auto specularComponent = material->GetSpecular() * glm::pow(glm::max(glm::dot(normal, halfAngle), 0.0f), material->GetShininess());
			auto diffuseComponent = material->GetDiffuse() * glm::max(glm::dot(normal, lightDirection), 0.0f);

			finalIntensity += directionalLight.get()->color * (diffuseComponent + specularComponent);
		}
	}

	//return glm::vec3(.5, .5, .5);

	auto nextRay = RayColor(scene, scene.get()->CastRay(intersection.hit, glm::reflect(intersection.direction, intersection.normal)), depth + 1);

	return finalIntensity + (material->GetSpecular() * nextRay);
}
